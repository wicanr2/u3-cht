/* plat_resource.c — Resource Manager / 檔案 fork / 別名 / 資料夾 / Movie stub
 *
 * 移植期一律 stub:資源管理回 nil/noErr。
 * ⚠ 之後做「可玩」的 save/load (ROSTER/PARTY) 需把 FSReadFork/FSWriteFork/GetEOF/
 *    FSpOpenResFile 接成真正讀寫檔 (見 UltimaMisc.c InitRoster / UltimaNew.c prefs)。
 *
 * 注意:FSGetCatalogInfo / FSMakeFSSpec 已在 plat_image.c 真實作 (影像鏈用),此處不重複。 */
#include "mac_shim.h"
#include <stdint.h>

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
    gResErr = noErr;
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
void   ReleaseResource(Handle h)               { if (h) DisposeHandle(h); }
void   ChangedResource(Handle h)               { (void)h; }
void   WriteResource(Handle h)                 { (void)h; }
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
