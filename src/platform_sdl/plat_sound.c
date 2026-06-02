/* plat_sound.c — 音效 / 音樂 / 語音 全 no-op stub
 *
 * 目前無音訊後端 (gSoundIncapable/gMusicIncapable = TRUE)。
 * 之後做「可玩」若要音效,於此接 SDL_mixer (PlaySoundFile/MusicUpdate)。 */
#include "mac_shim.h"

/* ===== Sound Manager ===== */
OSErr OpenChannel(void)  { return noErr; }   /* 簽名不重要 (回傳忽略) */
void  CloseChannel(SndChannelPtr chan) { (void)chan; }
OSErr SndDoImmediate(SndChannelPtr chan, const SndCommand *cmd) {
    (void)chan; (void)cmd; return noErr;
}

/* ===== 音效播放 (CocoaBridge / UltimaSound) ===== */
void PlaySoundFile(CFStringRef name, Boolean async) { (void)name; (void)async; }
void ErrorTone(void) {}
void ApplyVolumePreferences(void) {}

/* ===== 音樂 ===== */
void SetUpMusic(void) {}
void MusicUpdate(void) {}
void EndSong(void) {}
void CloseMusic(void) {}
void SetMusicPortAndDevice(void) {}

/* ===== 語音 ===== */
void SetUpSpeech(void) {}
void SpeakMessages(Boolean on) { (void)on; }
void Speech(StringPtr s) { (void)s; }
