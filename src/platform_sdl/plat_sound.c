/* plat_sound.c — 音效 (SDL_mixer) / 音樂 / 語音
 *
 * 音效:assets/Sounds/<name>.wav|.mp3,由 PlaySoundFile(CFSTR("Hit")…) 觸發。
 *       Mix_OpenAudio 初始化失敗 (如 headless 無音訊裝置) 則靜默停用,不崩潰。
 *       測試可設 SDL_AUDIODRIVER=dummy 走虛擬裝置,或 U3_NOSOUND 完全關閉。
 * 音樂:原始為 .mov (QuickTime),SDL_mixer 不支援,需 ffmpeg 轉 ogg (容器暫無);
 *       MusicUpdate 等保持安全 stub,待轉檔後接 Mix_Music。 */
#include "mac_shim.h"
#include <SDL.h>
#include <SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int gAudioReady = 0;
static int gAudioTried = 0;

#define MAX_SND 64
static struct { char name[32]; Mix_Chunk *chunk; } gCache[MAX_SND];
static int gCacheN = 0;

static void audio_init(void) {
    gAudioTried = 1;
    if (getenv("U3_NOSOUND")) return;
    if (!SDL_WasInit(SDL_INIT_AUDIO) && SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "[U3] 音訊子系統初始化失敗:%s\n", SDL_GetError());
        return;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
        fprintf(stderr, "[U3] Mix_OpenAudio 失敗 (無音訊裝置?):%s\n", Mix_GetError());
        return;
    }
    Mix_AllocateChannels(16);
    gAudioReady = 1;
    fprintf(stderr, "[U3] 音效就緒 (SDL_mixer)\n");
}

static Mix_Chunk *load_sound(const char *name) {
    for (int i = 0; i < gCacheN; i++)
        if (strcmp(gCache[i].name, name) == 0) return gCache[i].chunk;
    Mix_Chunk *c = NULL;
    const char *ext[] = { "wav", "mp3" };
    char path[256];
    for (int e = 0; e < 2 && !c; e++) {
        snprintf(path, sizeof(path), "assets/Sounds/%s.%s", name, ext[e]);
        c = Mix_LoadWAV(path);
    }
    if (gCacheN < MAX_SND) {
        strncpy(gCache[gCacheN].name, name, sizeof(gCache[0].name) - 1);
        gCache[gCacheN].chunk = c;
        gCacheN++;
    }
    return c;
}

/* ===== Sound Manager (殘留;音效改走 PlaySoundFile) ===== */
OSErr OpenChannel(void)  { return noErr; }
void  CloseChannel(SndChannelPtr chan) { (void)chan; }
OSErr SndDoImmediate(SndChannelPtr chan, const SndCommand *cmd) {
    (void)chan; (void)cmd; return noErr;
}

/* ===== 音效播放 ===== */
void PlaySoundFile(CFStringRef name, Boolean async) {
    (void)async;
    if (!gAudioTried) audio_init();
    if (!gAudioReady || !name) return;
    Mix_Chunk *c = load_sound((const char *)name);
    if (getenv("U3_DBG_SOUND")) fprintf(stderr, "[SND] %s -> %s\n", (const char *)name, c ? "play" : "MISS");
    if (c) Mix_PlayChannel(-1, c, 0);
}
void ErrorTone(void) { PlaySoundFile((CFStringRef)"Error1", true); }
void ApplyVolumePreferences(void) {}

/* ===== 音樂 (QTMA→MIDI→ogg,見 tools/qtma2midi.py) =====
 * 原始 Song_*.mov 是 QuickTime Music ('musi' 音符序列),已離線轉成
 * assets/Music/Song_<N>.ogg (1-8 / A=10 / B=11)。gSongNext 為遊戲要求的曲目,
 * MusicUpdate 在其變動時載入對應 ogg 並迴圈播放。 */
extern short gSongCurrent, gSongNext, gSongPlaying;
static Mix_Music *gMusic = NULL;
static int gPlaying = -1;

static const char *song_suffix(int n) {
    static char buf[4];
    if (n == 10) return "A";
    if (n == 11) return "B";
    snprintf(buf, sizeof(buf), "%d", n);
    return buf;
}
void SetUpMusic(void) { if (!gAudioTried) audio_init(); }
void MusicUpdate(void) {
    if (!gAudioTried) audio_init();
    if (!gAudioReady) return;
    int want = gSongNext;
    if (want == gPlaying) return;     /* 只在要求曲目變動時切換 */
    gPlaying = want;
    if (gMusic) { Mix_HaltMusic(); Mix_FreeMusic(gMusic); gMusic = NULL; }
    if (want <= 0) return;            /* 0 = 停止 */
    char path[256];
    snprintf(path, sizeof(path), "assets/Music/Song_%s.ogg", song_suffix(want));
    gMusic = Mix_LoadMUS(path);
    if (gMusic) {
        Mix_VolumeMusic(MIX_MAX_VOLUME * 3 / 4);
        Mix_PlayMusic(gMusic, -1);   /* 迴圈 */
        if (getenv("U3_DBG_SOUND")) fprintf(stderr, "[MUS] play Song_%s.ogg\n", song_suffix(want));
    } else if (getenv("U3_DBG_SOUND")) {
        fprintf(stderr, "[MUS] MISS %s\n", path);
    }
}
void EndSong(void) {
    if (gMusic) { Mix_HaltMusic(); Mix_FreeMusic(gMusic); gMusic = NULL; }
    gPlaying = -1;
}
void CloseMusic(void) { EndSong(); }
void SetMusicPortAndDevice(void) {}

/* ===== 語音 (Mac TTS;移植期停用) ===== */
void SetUpSpeech(void) {}
void SpeakMessages(Boolean on) { (void)on; }
void Speech(StringPtr s) { (void)s; }
