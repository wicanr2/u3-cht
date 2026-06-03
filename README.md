# Ultima III: Exodus 繁體中文化 (u3-cht)

把官方授權的 [`beastie/ultima3`](https://github.com/beastie/ultima3)(LairWare Ultima III,
MIT C 源碼)從 Mac(Carbon / Cocoa / QuickDraw)移植到 **SDL2**,並以 **UTF-8 + SDL_ttf
(Noto Sans CJK)** 全面繁體中文化,可在 **Linux(AppImage)** 與 **Windows** 執行遊玩。

本 repo 同時是一份 **Ultima III 繁體中文參考入口**:除了可玩的移植版,也整理了
世界觀、人物、發行史、台灣流通查證、在地化筆記與入門導覽(見下方
[Ultima III 中文知識庫](#-ultima-iii-中文知識庫))。

> 《Ultima III: Exodus》(EA 官方繁中名 **《創世紀3:魔胎》**)由 Richard Garriott 設計、
> 1983 年 Origin Systems 於 Apple II 發行,是「黑暗時代三部曲」終章。玩家組成**四人隊伍**,
> 在 **Sosaria(索薩利亞)** 追查並終結 Mondain 與 Minax 的造物 **Exodus**。
> 完整背景見 [世界觀導讀](docs/ultima-iii-overview.md)。

---

## 📸 截圖(繁中化後)

| 主選單 / 標題 | 調整選項 |
|---|---|
| ![主選單](docs/screenshots/01-title-menu-zh.png) | ![選項](docs/screenshots/02-options-zh.png) |

| 世界地圖 + 隊伍 sidebar | 進入城堡(內部) |
|---|---|
| ![世界與隊伍](docs/screenshots/03-world-party-zh.png) | ![城堡](docs/screenshots/04-castle-zh.png) |

| 創角 + 種族/職業說明(中文) | 種族屬性 / 職業說明圖(中文) |
|---|---|
| ![創角](docs/screenshots/05-create-char-zh.png) | ![種族職業說明](docs/screenshots/06-raceclass-info-zh.png) |

右側隊伍面板顯示**種族+職業短碼**(精靈賊 / 人類俠 / 哈比牧 / 毛絨巫)、等級「**級**」、
食物「**糧**」、無法力職業標「**不可施法**」;底部訊息(如「**等待**」)、選單、創角、
城堡/城鎮、種族職業說明圖均為中文。

---

## ⬇️ 下載與執行(Release)

預先打包的可執行檔請至 **[Releases 頁面](../../releases/latest)** 下載
(大型二進位不入 git,改以 GitHub Release 發布)。

### Linux — AppImage
```bash
chmod +x Ultima3-CHT-x86_64.AppImage
./Ultima3-CHT-x86_64.AppImage
```
- 自含 SDL 相依與遊戲資料(含中文字型與繁中 RaceClassInfo 圖),於 Ubuntu 22.04+ 可執行。
- 無音訊裝置時只警告、停用音效,遊戲仍可玩。

### Windows — zip
```text
解壓 Ultima3-CHT-windows-x64.zip,執行其中的 .exe。
```

> 也可依下節「**從原始碼建置**」自行產生 `dist/` 內的可執行檔。

---

## 🛠️ 從原始碼建置(Docker first)

一律於容器內 build/test,不污染宿主環境。

```bash
# 1) 建映像
docker build -t u3cht docker/

# 2) 編譯遊戲 → build/u3 (HOSTUID/HOSTGID 讓產物 chown 回宿主)
docker run --rm -e HOSTUID=$(id -u) -e HOSTGID=$(id -g) -v "$PWD":/work \
  u3cht bash /work/u3-cht/tools/build_game.sh

# 3) 打包 AppImage (Ubuntu 22.04 容器,自動重生繁中 RaceClassInfo.gif)
docker run --rm -e HOSTUID=$(id -u) -e HOSTGID=$(id -g) -v "$(dirname $PWD)":/work \
  ubuntu:22.04 bash /work/u3-cht/tools/package_appimage.sh
```

> 路徑說明:本 repo 位於 `.../u3-cht/u3-cht`;打包腳本預期掛載其上一層為 `/work`。
> 依你的目錄結構調整 `-v` 來源即可。

---

## ✅ 測試 / 驗證

```bash
# 單元測試 (cmake + ctest)。host 缺 sdl2 pkg-config 時請於容器內跑:
docker run --rm -v "$PWD":/work u3cht bash /work/u3-cht/tools/run_tests.sh

# 整合 smoke (容器內 build + 腳本驅動截圖,斷言不崩潰且持續渲染)
docker run --rm -e HOSTUID=$(id -u) -e HOSTGID=$(id -g) -v "$PWD":/work -w /work/u3-cht \
  u3cht bash tools/build_and_verify.sh tests/scripts/smoke.txt 45 15

# AppImage release smoke (可從 repo 外任意 cwd 跑,需 host 有 xvfb-run)。
# 以 [SCENE] 標記斷言:主選單 / 選項 / 編組 / 世界 / 城堡 五場景皆抵達。
cd /tmp && /path/to/u3-cht/tools/smoke_appimage.sh
```

> `run_tests.sh` 需 SDL2 開發環境;**宿主缺 `sdl2` pkg-config 時請改用上面的 `u3cht` 容器**,
> 本機直接跑失敗屬環境缺依賴,非專案問題。
> 測試掛勾(僅測試用,正式版不啟用):`U3_SKIPINTRO`、`U3_DBG_SCENE`、`U3_TELEPORT` 等,
> 細節見 [在地化筆記 · 測試與截圖](docs/localization-notes.md#測試與截圖注意事項)。

---

## 📚 Ultima III 中文知識庫

以原創中文整理、附參考來源;不收錄受版權保護的原始手冊全文。

| 文件 | 內容 |
|---|---|
| [世界觀與故事背景](docs/ultima-iii-overview.md) | Sosaria、黑暗時代三部曲、Exodus、火焰之島、為何重要 |
| [人物與勢力](docs/characters-and-factions.md) | Lord British、Mondain、Minax、Exodus、自建四人隊伍、種族/職業 |
| [發行與移植史](docs/release-history.md) | Garriott / Origin、1983 Apple II 原版、多平台移植、在地化 |
| [台灣發行/流通查證](docs/taiwan-history.md) | 官方中文名;精訊/第三波 屬**收藏者紀錄(非官方代理證明)**;**未查到可靠發行年份** |
| [在地化筆記](docs/localization-notes.md) | 翻譯原則、名詞對照、字型/SDL/AppImage、踩雷與測試掛勾 |
| [入門玩法導覽](docs/gameplay-guide.md) | 主選單、創角、種族職業、操作、初期目標 |

> 台灣發行年份/授權狀態目前**未能查到可靠公開資料**證實,相關段落採保守措辭並列出查證方法,
> 詳見 [taiwan-history.md](docs/taiwan-history.md)。

---

## 🧩 專案技術重點

- **compat shim**(`src/compat/`):以 SDL2 重建 QuickDraw / Mac Toolbox,上游 C 遊戲邏輯
  幾乎不改即可編譯(見 `docs/adr/`)。
- **文字單一出口**:全遊戲繪字收斂於 `UDrawThemePascalString` → SDL_ttf CJK,一處替換即全中文。
- **字串外部化**:`.u3s`(UTF-8)字串表,由 `tools/extract_strings.py` 從 `translations/*.json` 產生;
  未翻譯自動回退英文。
- **美術中文化**:`tools/make_raceclass_gif.py` 重繪繁中種族/職業說明圖,打包時自動重生。

開發歷程與可玩性狀態見 [`docs/GAMEPLAY-STATUS.md`](docs/GAMEPLAY-STATUS.md);
編譯移植細節見 [`docs/P3-compat-compile-status.md`](docs/P3-compat-compile-status.md)。

---

## ⚠️ 已知限制

- **角色預設名**:範例隊伍角色名(Tatiana/Roderic/…)保留原西式專有名詞;種族/職業/狀態/
  數值欄位皆已繁中。
- **隊伍面板寬度**:右側 sidebar 空間有限,種族+職業以短碼呈現(完整名見「編組隊伍」名冊)。
- **音樂**:原始 `.mov`(QuickTime Music,類 MIDI)非取樣音訊,SDL_mixer 不直接支援,
  目前以安全 stub 處理(音效已可用)。
- **台灣發行考證**:見上,年份/授權待第一手資料補充。

---

## 📄 授權

- **程式碼**:MIT(沿用上游 © Leon McNeill,移植與中文化部分同 MIT)。
- **非程式資產**(美術/音樂/字串)源自原作,著作權屬原權利人(現 IP 屬 EA);
  本專案採「引擎與資料分離」,以自用與技術研究為主。
- 知識庫文件為原創中文整理,引用之事實均附參考來源連結。
