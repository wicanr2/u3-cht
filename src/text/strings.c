/* strings.c — 執行期 UTF-8 字串表讀取器 */
#include "strings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TABLES 32

typedef struct {
    char     name[32];
    int      count;
    unsigned char *data;   /* 整個 .u3s 檔內容 */
    const unsigned char *offsets; /* 指向檔中 offset 表 (count × uint32 LE) */
    const char *blob;      /* UTF-8 blob 起點 */
} Table;

static Table gTables[MAX_TABLES];
static int   gTableN = 0;
static char  gRoot[512] = "assets/strings";
static char  gLang[32]  = "zh-Hant";

void Strings_SetRoot(const char *root) {
    if (root && *root) { strncpy(gRoot, root, sizeof(gRoot)-1); gRoot[sizeof(gRoot)-1]=0; }
}
void Strings_SetLang(const char *lang) {
    if (lang && *lang) { strncpy(gLang, lang, sizeof(gLang)-1); gLang[sizeof(gLang)-1]=0; }
}

static uint32_t rd32(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}

static Table *find_loaded(const char *table) {
    for (int i = 0; i < gTableN; i++)
        if (strcmp(gTables[i].name, table) == 0) return &gTables[i];
    return NULL;
}

/* 嘗試從某語言載入;成功回 1 */
static int try_load(Table *t, const char *table, const char *lang) {
    char path[800];
    snprintf(path, sizeof(path), "%s/%s/%s.u3s", gRoot, lang, table);
    FILE *fp = fopen(path, "rb");
    if (!fp) return 0;
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (sz < 8) { fclose(fp); return 0; }
    unsigned char *buf = (unsigned char *)malloc(sz);
    if (!buf) { fclose(fp); return 0; }
    if (fread(buf, 1, sz, fp) != (size_t)sz) { free(buf); fclose(fp); return 0; }
    fclose(fp);
    if (memcmp(buf, "U3S1", 4) != 0) { free(buf); return 0; }
    t->count   = (int)rd32(buf + 4);
    t->offsets = buf + 8;
    t->blob    = (const char *)(buf + 8 + (size_t)t->count * 4);
    t->data    = buf;
    strncpy(t->name, table, sizeof(t->name)-1);
    t->name[sizeof(t->name)-1] = 0;
    return 1;
}

static Table *load_table(const char *table) {
    Table *t = find_loaded(table);
    if (t) return t;
    if (gTableN >= MAX_TABLES) return NULL;
    t = &gTables[gTableN];
    memset(t, 0, sizeof(*t));
    /* 先試設定語言,缺則回退 en */
    if (!try_load(t, table, gLang)) {
        if (strcmp(gLang, "en") == 0 || !try_load(t, table, "en")) return NULL;
    }
    gTableN++;
    return t;
}

const char *Strings_GetUTF8(const char *table, int index) {
    Table *t = load_table(table);
    if (!t || index < 0 || index >= t->count) return "";
    uint32_t off = rd32(t->offsets + (size_t)index * 4);
    return t->blob + off;
}

int Strings_Count(const char *table) {
    Table *t = load_table(table);
    return t ? t->count : 0;
}

void Strings_GetPascal(StringPtr pstringPtr, const char *table, int index) {
    const char *s = Strings_GetUTF8(table, index);
    size_t n = strlen(s);
    if (n > 255) n = 255;   /* Str255 上限;長中文走 UTF-8 路徑 */
    pstringPtr[0] = (unsigned char)n;
    memcpy(pstringPtr + 1, s, n);
}

void Strings_Shutdown(void) {
    for (int i = 0; i < gTableN; i++)
        if (gTables[i].data) free(gTables[i].data);
    gTableN = 0;
}
