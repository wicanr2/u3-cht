# Ultima III: Exodus 中文化 (u3-cht)

把官方授權的 [`beastie/ultima3`](https://github.com/beastie/ultima3)(LairWare Ultima III,MIT C 源碼)
從 Mac (Carbon/Cocoa/QuickDraw) 移植到 **SDL2**,並以 **UTF-8 + SDL_ttf(優質中文 TTF)** 全面中文化,
目標可在 Linux 執行遊玩。

> 詳細工程計畫見 [`PLAN.md`](PLAN.md);可行性分析見工作區的 `ultima3-中文化評估.md`。

## 建置 (Docker first)

```bash
# 建映像
docker build -t u3cht docker/

# 編譯 (容器內,不污染系統)
docker run --rm -v "$PWD":/work u3cht \
  bash -c "cmake -S . -B build && cmake --build build -j"

# 跑 smoke 與 PoC (headless)
docker run --rm -v "$PWD":/work u3cht \
  bash -c "./build/hello_sdl && xvfb-run -a ./build/poc"
```

PoC 會輸出 `poc_out.png`:左側 tile 拼圖 + 右側中文文字,一次驗證
「繪圖換得掉 / 字型載得到 / 中文畫得出」三個最高風險假設。

## 狀態

| Phase | 內容 | 狀態 |
|---|---|---|
| P0 | repo 骨架 + Docker + smoke build | 進行中 |
| P1 | 垂直切片 PoC | 進行中 |
| P2+ | 平台層移植 / 繪圖 / 文字 / 遊玩迴圈 | 待開始 |

## 授權

* 程式碼:MIT(沿用上游 © Leon McNeill,移植與中文化部分同 MIT)。
* 非程式資產(美術/音樂/字串)源自原作,著作權屬原權利人;本專案採引擎與資料分離,自用為主。
