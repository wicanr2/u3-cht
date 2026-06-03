/* plat_resource.c — Resource Manager / 檔案 fork / 別名 / 資料夾 / Movie stub
 *
 * 移植期一律 stub:資源管理回 nil/noErr。
 * ⚠ 之後做「可玩」的 save/load (ROSTER/PARTY) 需把 FSReadFork/FSWriteFork/GetEOF/
 *    FSpOpenResFile 接成真正讀寫檔 (見 UltimaMisc.c InitRoster / UltimaNew.c prefs)。
 *
 * 注意:FSGetCatalogInfo / FSMakeFSSpec 已在 plat_image.c 真實作 (影像鏈用),此處不重複。 */
#include "mac_shim.h"
#include <stdint.h>
#include <stdlib.h>   /* malloc/realloc/free (save overlay) */

/* mac_types.h 未定義 SInt64 (64-bit 檔案位移),此處補上 (僅本檔用)。 */
typedef int64_t SInt64;

/* ===== Resource Manager — 真正讀取 MainResources.rsrc =====
 * U3 的地圖(MAPS)、怪物(MONS)、對話(TLKS)、月相/職業表(MISC)、
 * 角色/隊伍範本(ROST/PRTY)等遊戲資料都在 Mac 資源 fork 內。
 * 解析資源 map,GetResource(type,id) 回傳資料的副本 Handle(經 NewHandle,
 * 與記憶體管理一致,支援 GetHandleSize/DisposeHandle)。 */
#include <stdio.h>
extern Handle NewHandle(Size);   /* plat_memory.c */
extern Handle NewHandleClear(Size);
extern void   DisposeHandle(Handle);

static unsigned char *gRsrc = NULL;
static long gRsrcLen = 0;
static int  gRsrcTried = 0;
static OSErr gResErr = noErr;

typedef struct { uint32_t type; int id; long dataPos; } ResEnt;
static ResEnt *gEnt = NULL;
static int gEntN = 0;

/* ===== 存檔 overlay =====
 * WriteResource 寫入的資源以 (type,id) 存進 overlay,並持久化到存檔檔。
 * GetResource 優先回 overlay → 角色名冊(ROST)、隊伍(PRTY)、目前世界(MAPS/MONS 419)
 * 等存檔資料可跨重啟保留。原始 MainResources.rsrc 維持唯讀。 */
typedef struct { uint32_t type; int id; long len; unsigned char *data; } SaveEnt;
#define MAX_SAVE 96
static SaveEnt gSave[MAX_SAVE];
static int gSaveN = 0;
static int gSaveLoaded = 0;

/* live handle → (type,id,len) 追蹤,供 WriteResource 反查資源身分 */
typedef struct { Handle h; uint32_t type; int id; long len; } TrkEnt;
#define MAX_TRK 512
static TrkEnt gTrk[MAX_TRK];
static int gTrkN = 0;

#define U3_SAVE_MAGIC 0x55335356u  /* 'U3SV' (LE) */
static const char *save_path(void) {
    const char *p = getenv("U3_SAVE");
    return (p && *p) ? p : "u3save.dat";
}
static SaveEnt *save_find(uint32_t type, int id) {
    for (int i = 0; i < gSaveN; i++)
        if (gSave[i].type == type && gSave[i].id == id) return &gSave[i];
    return NULL;
}
static void save_persist(void) {
    FILE *f = fopen(save_path(), "wb");
    if (!f) return;
    uint32_t magic = U3_SAVE_MAGIC, n = (uint32_t)gSaveN;
    fwrite(&magic, 4, 1, f); fwrite(&n, 4, 1, f);
    for (int i = 0; i < gSaveN; i++) {
        uint32_t id = (uint32_t)gSave[i].id, len = (uint32_t)gSave[i].len;
        fwrite(&gSave[i].type, 4, 1, f); fwrite(&id, 4, 1, f); fwrite(&len, 4, 1, f);
        if (len) fwrite(gSave[i].data, 1, len, f);
    }
    fclose(f);
}
static void save_load(void) {
    if (gSaveLoaded) return;
    gSaveLoaded = 1;
    FILE *f = fopen(save_path(), "rb");
    if (!f) return;
    uint32_t magic = 0, n = 0;
    if (fread(&magic, 4, 1, f) != 1 || magic != U3_SAVE_MAGIC || fread(&n, 4, 1, f) != 1) { fclose(f); return; }
    for (uint32_t i = 0; i < n && gSaveN < MAX_SAVE; i++) {
        uint32_t type, id, len;
        if (fread(&type, 4, 1, f) != 1 || fread(&id, 4, 1, f) != 1 || fread(&len, 4, 1, f) != 1) break;
        unsigned char *d = (unsigned char *)malloc(len ? len : 1);
        if (!d) break;
        if (len && fread(d, 1, len, f) != len) { free(d); break; }
        gSave[gSaveN].type = type; gSave[gSaveN].id = (int)id;
        gSave[gSaveN].len = (long)len; gSave[gSaveN].data = d; gSaveN++;
    }
    fclose(f);
    fprintf(stderr, "[U3] 存檔載入:%d 筆 (%s)\n", gSaveN, save_path());
}
static void save_upsert(uint32_t type, int id, const unsigned char *data, long len) {
    SaveEnt *e = save_find(type, id);
    if (!e) { if (gSaveN >= MAX_SAVE) return; e = &gSave[gSaveN++]; e->type = type; e->id = id; e->data = NULL; }
    unsigned char *nd = (unsigned char *)realloc(e->data, len ? len : 1);
    if (!nd) return;
    if (len) memcpy(nd, data, len);
    e->data = nd; e->len = len;
}
static void trk_add(Handle h, uint32_t type, int id, long len) {
    for (int i = 0; i < gTrkN; i++)
        if (gTrk[i].h == h) { gTrk[i].type = type; gTrk[i].id = id; gTrk[i].len = len; return; }
    int s = (gTrkN < MAX_TRK) ? gTrkN++ : 0;   /* live set 通常很小;滿則覆蓋第 0 筆 */
    gTrk[s].h = h; gTrk[s].type = type; gTrk[s].id = id; gTrk[s].len = len;
}
static TrkEnt *trk_find(Handle h) {
    for (int i = 0; i < gTrkN; i++) if (gTrk[i].h == h) return &gTrk[i];
    return NULL;
}
static void trk_remove(Handle h) {
    for (int i = 0; i < gTrkN; i++) if (gTrk[i].h == h) { gTrk[i] = gTrk[--gTrkN]; return; }
}

static uint32_t be32(const unsigned char *p){ return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3]; }
static uint16_t be16(const unsigned char *p){ return (uint16_t)((p[0]<<8)|p[1]); }
static int16_t  be16s(const unsigned char *p){ return (int16_t)be16(p); }

static void rsrc_load(void) {
    gRsrcTried = 1;
    FILE *f = fopen("assets/MainResources.rsrc", "rb");
    if (!f) { fprintf(stderr,"[U3] 找不到 assets/MainResources.rsrc\n"); return; }
    fseek(f, 0, SEEK_END); gRsrcLen = ftell(f); fseek(f, 0, SEEK_SET);
    gRsrc = (unsigned char*)malloc(gRsrcLen);
    if (fread(gRsrc, 1, gRsrcLen, f) != (size_t)gRsrcLen) { fclose(f); gRsrc=NULL; return; }
    fclose(f);

    long dataOff = be32(gRsrc), mapOff = be32(gRsrc+4);
    const unsigned char *m = gRsrc + mapOff;
    int typeListOff = be16(m+24);
    const unsigned char *tlist = m + typeListOff;
    int numTypes = be16s(tlist) + 1;
    /* 先數總數 */
    int total = 0;
    for (int i=0;i<numTypes;i++){ total += be16s(tlist+2+i*8+4)+1; }
    gEnt = (ResEnt*)malloc(sizeof(ResEnt)*total);
    for (int i=0;i<numTypes;i++){
        const unsigned char *e = tlist + 2 + i*8;
        uint32_t rtype = be32(e);
        int count = be16s(e+4)+1;
        int refOff = be16(e+6);
        const unsigned char *rl = tlist + refOff;
        for (int j=0;j<count;j++){
            const unsigned char *r = rl + j*12;
            int rid = be16s(r);
            long dpos = dataOff + (((long)r[5]<<16)|((long)r[6]<<8)|r[7]);
            gEnt[gEntN].type = rtype; gEnt[gEntN].id = rid; gEnt[gEntN].dataPos = dpos;
            gEntN++;
        }
    }
    fprintf(stderr,"[U3] MainResources.rsrc 載入:%d 個資源\n", gEntN);
}

Handle GetResource(ResType type, SInt16 id) {
    if (!gRsrcTried) rsrc_load();
    if (!gSaveLoaded) save_load();
    gResErr = noErr;
    /* 存檔 overlay 優先:已存檔的資源覆蓋 bundle (含 MAPS/MONS 419 目前世界) */
    SaveEnt *sv = save_find((uint32_t)type, (int)id);
    if (sv) {
        Handle h = NewHandleClear((Size)sv->len + 32);
        if (h && *h && sv->len) memcpy(*h, sv->data, sv->len);
        if (h) trk_add(h, (uint32_t)type, (int)id, sv->len);
        return h;
    }
    if (!gRsrc) { gResErr = -192 /*resNotFound*/; return nil; }
    /* 本發行版的「目前 Sosaria」MAPS/MONS 419 不在資源檔中;Mac 版會由重設/寫檔建立。
     * 移植期資源寫入是 no-op,所以讀 419 時回退到原始 Sosaria 420,讓 Journey 可進世界。
     * 其他類型保留舊後備:ROST/PRTY 等可能與 BASERES 慣例差 ±100。 */
    int delta[4] = { 0, -100, +100, 0 };
    int deltaN = 3;
    if (((uint32_t)type == (uint32_t)'MAPS' || (uint32_t)type == (uint32_t)'MONS') && id == 419) {
        delta[0] = 1; delta[1] = 0; delta[2] = -100; delta[3] = +100; deltaN = 4;
    }
    for (int pass = 0; pass < deltaN; pass++) {
        int want = (int)id + delta[pass];
        for (int i=0;i<gEntN;i++){
            if (gEnt[i].type == (uint32_t)type && gEnt[i].id == want) {
                long dpos = gEnt[i].dataPos;
                uint32_t len = be32(gRsrc + dpos);
                /* +32 零化 slop:部分原碼讀取略超出資源長度 (依賴 Mac handle slop) */
                Handle h = NewHandleClear((Size)len + 32);
                if (h && *h && len) memcpy(*h, gRsrc + dpos + 4, len);
                /* 以「請求的」(type,id) 追蹤 (非後備 want),使存檔對應正確
                 * (例:MAPS 419 後備讀 420,但存檔須以 419 寫回)。 */
                if (h) trk_add(h, (uint32_t)type, (int)id, (long)len);
                return h;
            }
        }
    }
    gResErr = -192;
    return nil;
}
Handle Get1Resource(ResType type, SInt16 id)  { return GetResource(type, id); }
void   LoadResource(Handle h)                 { (void)h; }
OSErr  ResError(void)                          { return gResErr; }
void   ReleaseResource(Handle h)               { if (h) { trk_remove(h); DisposeHandle(h); } }
void   ChangedResource(Handle h)               { (void)h; }
/* WriteResource:反查 handle 的 (type,id,len),寫入 overlay 並持久化存檔。 */
void   WriteResource(Handle h) {
    if (!h || !*h) return;
    TrkEnt *t = trk_find(h);
    if (!t) return;   /* 非由 GetResource 取得 (無身分),略過 */
    save_upsert(t->type, t->id, (const unsigned char *)*h, t->len);
    save_persist();
}
void   AddResource(Handle h, ResType t, SInt16 id, ConstStr255Param name) {
    (void)h; (void)t; (void)id; (void)name;
}
void   DetachResource(Handle h)                { (void)h; }
SInt16 UseResFile(SInt16 refNum)               { (void)refNum; return noErr; }
void   UpdateResFile(SInt16 refNum)            { (void)refNum; }
SInt16 FSpOpenResFile(const FSSpec *spec, SInt8 perm) { (void)spec; (void)perm; return -1; }
void   FSpCreateResFile(const FSSpec *spec, OSType creator, OSType type, SInt16 script) {
    (void)spec; (void)creator; (void)type; (void)script;
}

/* 顏色表 / 圖片 / 游標資源 (回 nil → game 走預設/略過) */
CTabHandle GetCTable(SInt16 id)  { (void)id; return nil; }
PicHandle  GetPicture(SInt16 id) { (void)id; return nil; }
/* 回非 NULL 假 cursor handle:ToolBoxInit 會 *watchCurs 解參且不檢查 nil。 */
static char gDummyCursorData[68];
static Ptr  gDummyCursorPtr = gDummyCursorData;
CursHandle GetCursor(SInt16 id)  { (void)id; return (CursHandle)&gDummyCursorPtr; }

/* ===== 檔案 fork / EOF (save/load 用;移植期 stub) ===== */
OSErr FSClose(SInt16 refNum) { (void)refNum; return noErr; }
OSErr GetEOF(SInt16 refNum, SInt32 *logEOF) {
    (void)refNum; if (logEOF) *logEOF = 0; return noErr;
}
OSErr FSReadFork(SInt16 refNum, UInt16 positionMode, SInt64 positionOffset,
                 SInt32 requestCount, void *buffer, SInt32 *actualCount) {
    (void)refNum; (void)positionMode; (void)positionOffset; (void)requestCount; (void)buffer;
    if (actualCount) *actualCount = 0;
    return noErr;
}
OSErr FSWriteFork(SInt16 refNum, UInt16 positionMode, SInt64 positionOffset,
                  SInt32 requestCount, const void *buffer, SInt32 *actualCount) {
    (void)refNum; (void)positionMode; (void)positionOffset; (void)requestCount; (void)buffer;
    if (actualCount) *actualCount = requestCount;
    return noErr;
}
OSErr FSpDelete(const FSSpec *spec) { (void)spec; return noErr; }

/* ===== 資料夾 / 別名 ===== */
OSErr FindFolder(SInt16 vRefNum, OSType folderType, Boolean createFolder,
                 SInt16 *foundVRefNum, SInt32 *foundDirID) {
    (void)vRefNum; (void)folderType; (void)createFolder;
    if (foundVRefNum) *foundVRefNum = 0;
    if (foundDirID)   *foundDirID = 0;
    return noErr;
}
OSErr IsAliasFile(const FSSpec *spec, Boolean *aliasFileFlag, Boolean *folderFlag) {
    (void)spec;
    if (aliasFileFlag) *aliasFileFlag = false;
    if (folderFlag)    *folderFlag = false;
    return noErr;
}
OSErr ResolveAliasFile(FSSpec *spec, Boolean resolveAliasChains,
                       Boolean *targetIsFolder, Boolean *wasAliased) {
    (void)spec; (void)resolveAliasChains;
    if (targetIsFolder) *targetIsFolder = false;
    if (wasAliased)     *wasAliased = false;
    return noErr;
}

/* ===== QuickTime Movie (片頭;停用) ===== */
void MoviesTask(void *movie, SInt32 maxMs) { (void)movie; (void)maxMs; }
void ExitMovies(void) {}
