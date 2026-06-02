/* plat_globals.c — 平台層全域變數定義
 *
 * 這些變數原定義在被排除的 Cocoa/.m / MacIF 平台檔,各遊戲檔以 extern 參用。
 * 在此給出單一定義 (型別對齊上游 extern 宣告)。 */
#include "mac_shim.h"

/* ===== QuickDraw 全域 (qd.thePort / qd.randSeed) ===== */
QDGlobals qd;

/* ===== U3 偏好設定 key (CFStringRef = const char*,值即 key 名稱字串) ===== */
CFStringRef U3PrefAsyncSound              = CFSTR("AsyncSound");
CFStringRef U3PrefAutoSave                = CFSTR("AutoSave");
CFStringRef U3PrefCheckedResourcesDate    = CFSTR("CheckedResourcesDate");
CFStringRef U3PrefClassicAppearance       = CFSTR("ClassicAppearance");
CFStringRef U3PrefCurWindowX              = CFSTR("CurWindowX");
CFStringRef U3PrefCurWindowY              = CFSTR("CurWindowY");
CFStringRef U3PrefDontAskDisplayMode      = CFSTR("DontAskDisplayMode");
CFStringRef U3PrefFullScreen              = CFSTR("FullScreen");
CFStringRef U3PrefFullScreenResChange     = CFSTR("FullScreenResChange");
CFStringRef U3PrefGameFont                = CFSTR("GameFont");
CFStringRef U3PrefHealThreshold           = CFSTR("HealThreshold");
CFStringRef U3PrefIncludeWind             = CFSTR("IncludeWind");
CFStringRef U3PrefInformDayInterval       = CFSTR("InformDayInterval");
CFStringRef U3PrefInformedNewVersionDate  = CFSTR("InformedNewVersionDate");
CFStringRef U3PrefLatestReleaseNote       = CFSTR("LatestReleaseNote");
CFStringRef U3PrefLatestReleaseNumber     = CFSTR("LatestReleaseNumber");
CFStringRef U3PrefManualCombat            = CFSTR("ManualCombat");
CFStringRef U3PrefMusicInactive           = CFSTR("MusicInactive");
CFStringRef U3PrefMusicVolume             = CFSTR("MusicVolume");
CFStringRef U3PrefNoAutoHeal              = CFSTR("NoAutoHeal");
CFStringRef U3PrefNoDiagonals             = CFSTR("NoDiagonals");
CFStringRef U3PrefNoEducateAboutFullScreen= CFSTR("NoEducateAboutFullScreen");
CFStringRef U3PrefOriginalSize            = CFSTR("OriginalSize");
CFStringRef U3PrefRegCode                 = CFSTR("RegCode");
CFStringRef U3PrefSaveWindowX             = CFSTR("SaveWindowX");
CFStringRef U3PrefSaveWindowY             = CFSTR("SaveWindowY");
CFStringRef U3PrefSoundInactive           = CFSTR("SoundInactive");
CFStringRef U3PrefSoundVolume             = CFSTR("SoundVolume");
CFStringRef U3PrefSpeechInactive          = CFSTR("SpeechInactive");
CFStringRef U3PrefSpeedUnconstrain        = CFSTR("SpeedUnconstrain");
CFStringRef U3PrefTileSet                 = CFSTR("TileSet");
CFStringRef U3PrefUserName                = CFSTR("UserName");

/* ===== 音效 / 音樂 全域 (型別對齊上游 extern) ===== */
short          gSongCurrent = 0, gSongNext = 0, gSongPlaying = 0;
short          gCurChan = 0, gMaxChan = 0;
SndChannelPtr  gSampChan[5] = {0};
SndChannelPtr  gToneChan = 0;
Boolean        gSoundIncapable = TRUE;   /* 無音效後端 → 標記不可用,避免 game 嘗試播放 */
Boolean        gMusicIncapable = TRUE;

/* ===== Movie (QuickTime 片頭;停用) ===== */
CGrafPtr       gMoviesPort = 0;
