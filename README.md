# teat — 三燈視覺倒數計時器（泡茶 / 爐火提醒器）

> 本 README 依課程「README 範本：牛奶瓶版」的 10 節結構撰寫。
> 第 1–5、9 節以本專案程式碼與腳位為準整理；第 6–8、10 節為補充。
> 說明文字以繁體中文撰寫，與程式碼註解一致。

---

## 1. 情境故事

你想在泡茶、煮蛋或關注爐火的時候，不必一直盯著手機計時器。
現實中你會買一台帶燈的廚房計時器；但對原型來說，我們先用開發板上的
**三顆 LED** 當作「時間還剩多少」的視覺指標：

- 你先用電腦透過序列埠告訴板子「要計時幾分鐘」（`SET <分鐘>`）。
- 再送出 `START`，三顆燈立刻**全亮**。
- 隨著時間流逝，三顆燈**依序漸暗**（第一顆 → 第二顆 → 第三顆）。
- 三顆全部熄滅時，板子回報 `TIME_UP`，表示時間到。
- 過程中按一下外接按鈕，可以**暫停 / 繼續**。

這讓「還剩多少時間」一眼就能看穿，不必看數字。

---

## 2. 替身對照（故事裡的模組 → 板上替身）

因為還沒有做外殼與真實顯示器，我們用開發板上的資源當作**替身**：

| 故事裡的物件 | 板上的替身 | 說明 |
| --- | --- | --- |
| 廚房計時器的「時間顯示」 | PA0 / PA1 / PB10 三顆 LED（TIM2 PWM） | 用亮度表示剩餘時間，依序漸暗 |
| 設定鈕 / 手機設定介面 | 電腦序列埠（LPUART1） | 傳 `SET` / `START` 指令 |
| 暫停鈕 | PC9 外接按鈕（外部中斷） | 切換暫停 |
| 「時間到」提醒 | 序列埠 `TIME_UP` 字串 | 替代真實蜂鳴器 / 螢幕 |
| 真實外殼 + 電源 | NUCLEO-G474RE 開發板 + ST-LINK 供電 | 原型階段直接用評估板 |

> 換成真實裝置時：把 LED 替身換成段顯 / 螢幕，把序列埠換成實體旋鈕，
> 把 `TIME_UP` 換成蜂鳴器即可（見第 10 節）。

---

## 3. 功能與驗收條件

| 動作 | 預期結果 | 實測結果（依程式邏輯） |
| --- | --- | --- |
| 上電、開啟終端機 | 板子等待指令，三燈全暗 | ✅ state=0，主迴圈持續將 compare 設 0 |
| 終端機送 `SET 1` | 回 `收到指令: [SET 1]`，進入就緒 | ✅ `countdown_time=1`、`state=1` |
| 終端機送 `START` | 三燈瞬間全亮，開始依序漸暗 | ✅ `state=2`，PA0→PA1→PB10 各 200 步漸暗 |
| 等待至三燈全暗 | 回 `TIME_UP`，回到等待 | ✅ 送 `TIME_UP\r\n`，`state=0` |
| 執行中按 PC9 | 暫停 / 繼續（忙等） | ✅ `pause_flag` 切換，200ms 軟體消抖 |
| `START` 但未 `SET` | 不開始（需 `countdown_time>0`） | ✅ 僅在 `countdown_time>0` 才 `state=2` |

---

## 4. 用到的技術（第 3～7 週勾選表）

- [x] **GPIO 輸出**：TIM2 PWM 驅動三顆 LED（PA0/PA1/PB10）
- [x] **外部中斷（EXTI）**：PC9 下降緣觸發暫停（EXTI9_5，priority 0）
- [x] **TIM 定時器 / PWM**：TIM2 CH1/2/3，Prescaler=169、Period=999
- [x] **UART 非同步串列**：LPUART1 115200 8N1，RX 中斷逐 byte
- [x] **中斷服務程式（ISR）**：`HAL_UART_RxCpltCallback`、`HAL_GPIO_EXTI_Callback`
- [x] **狀態機**：state 0/1/2（等待 / 就緒 / 執行中）
- [x] **HAL 函式庫 + STM32CubeMX**：程式碼由 CubeMX 6.18.1 生成框架
- [x] **STM32CubeIDE 建置 / 燒錄**：專案為 CubeIDE 格式（含 `Debug/` 輸出）
- [x] **序列終端機**：透過 ST-LINK 虛擬 COM 埠下指令
- [ ] 低功耗（Stop/Sleep）模式
- [ ] RTC 即時時鐘 / 斷電保持
- [ ] FreeRTOS 多工

---

## 5. 腳位表

| 腳位 | 訊號 / 標籤 | 用途 | 方向 | 備註 |
| --- | --- | --- | --- | --- |
| PA0 | TIM2_CH1 | LED 1（第一顆，先暗） | 輸出 (PWM) | 外接 LED + 限流電阻 |
| PA1 | TIM2_CH2 | LED 2（第二顆） | 輸出 (PWM) | 外接 LED + 限流電阻 |
| PB10 | TIM2_CH3 | LED 3（第三顆，最後暗） | 輸出 (PWM) | 外接 LED + 限流電阻 |
| PA2 | LPUART1_TX [STLINKV3E_VCP_RX] | 序列傳送 | 輸出 | 接 ST-LINK VCP |
| PA3 | LPUART1_RX [STLINKV3E_VCP_TX] | 序列接收 | 輸入 | 接 ST-LINK VCP，RX 中斷 |
| PC9 | GPXTI9 | 暫停按鈕 | 輸入 | 下降緣 + 內部上拉，外接按鈕 |
| SWDIO / SWCLK | PA13 / PA14 | 燒錄與除錯 | — | ST-LINK 內建 |

> 注意：NUCLEO-G474RE 板載使用者 LED（LD2）在 PA5、使用者按鈕（B1）在 PC13，
> 本專案**未使用**板載 LD2 / B1，三顆 LED 與暫停鈕皆為**外接**。

---

## 6. 建置與執行

### 開發環境
- 開發板：**NUCLEO-G474RE**（MCU = STM32G474RET6）
- 工具：**STM32CubeIDE**（專案由 STM32CubeMX 6.18.1 生成）
- 燒錄／供電：板載 ST-LINK（USB 連電腦即可）
- 序列終端機：Tera Term / PuTTY / CubeIDE 內建 Serial，選 ST-LINK VCP 的 COM 埠

### 燒錄步驟
1. 用 STM32CubeIDE 開啟本倉庫的專案資料夾 `weeeeeeeeeeee/we1`。
2. 接上 NUCLEO-G474RE（USB 連 ST-LINK）。
3. 在 CubeIDE 點 **Run → Run**（或 Debug），會自動編譯並經 ST-LINK 燒錄。
4. （倉庫已附 `Debug/` 建置輸出，亦可直接燒錄該 `.elf` / `.bin`。）

### 終端機設定
- 埠：ST-LINK Virtual COM Port（裝置管理員查看）
- 鮑率：**115200**，資料位 8、停止位 1、無奇偶校驗、無硬體流控
- 換行：指令以 `\r` 或 `\n` 結尾分句

### 命令
```
SET <分鐘>    例如：SET 1   設定倒數 1 分鐘，板子回「收到指令: [SET 1]」
START                      開始計時（需先 SET 且分鐘 > 0）
```
結束時板子主動送出 `TIME_UP\r\n`。

---

## 7. 程式結構

| 檔案 | 負責什麼 |
| --- | --- |
| `Core/Src/main.c` | 主程式：系統時脈、周邊初始化、主迴圈狀態機、PWM 漸暗邏輯、UART/EXTI 回呼 |
| `Core/Src/stm32g4xx_it.c` | 中斷向量入口：`EXTI9_5_IRQHandler`、`TIM2_IRQHandler`、`LPUART1_IRQHandler` 等 |
| `Core/Src/stm32g4xx_hal_msp.c` | HAL MSP：周邊時脈與腳位底層初始化（`HAL_TIM_MspPostInit` 等） |
| `Core/Src/system_stm32g4xx.c` | 啟動與向量表、系統時脈常數 |
| `Core/Inc/main.h` | 腳位巨集（`LPUART1_TX_Pin` 等）、`HAL_TIM_MspPostInit` 宣告 |
| `Core/Startup/startup_stm32g474retx.s` | 啟動組語、堆疊/堆積與 Reset 向量 |
| `we1.ioc` | STM32CubeMX 設定（腳位、TIM2、LPUART1、時脈 170MHz） |

### ISR 與主迴圈分工
- **主迴圈 `while(1)`**：
  - state ≠ 2 時把三通道 compare 設 0（全暗）並 `HAL_Delay(100)` 空轉。
  - state = 2 時執行漸暗：三燈瞬亮 → PA0→PA1→PB10 各 200 步（每次 −5），
    每步以 `HAL_Delay(5)` 累積延遲；每步與每小步都檢查 `pause_flag`。
  - 三燈全暗後送 `TIME_UP`，`state` 歸 0、`countdown_time` 歸 0。
- **`HAL_UART_RxCpltCallback`（LPUART1）**：逐 byte 收集，遇 `\r`/`\n` 分句；
  解析 `SET %lu`（設 `countdown_time`，`state=1`）與 `START`（`state=2`）；
  回 echo `收到指令: [...]`；收完一定重新呼叫 `HAL_UART_Receive_IT`。
- **`HAL_GPIO_EXTI_Callback`（PC9）**：200ms 軟體消抖後切換 `pause_flag`（暫停/繼續）。

### 時間校正
- `total_ms = 分鐘 × 60 × 1000`；`calibrated_ms = total_ms × 88 / 100`。
- 600 步（3 燈 × 200），`step_delay = calibrated_ms / 600`（最小 1ms）。
- 用 `× 0.88` 硬校正 `HAL_Delay` 累積誤差（見第 10 節已知限制）。

---

## 8. 實作照片

> 本倉庫尚未包含實作照片。請拍攝下列畫面並 **commit 到 `docs/`**，
> 再以上方相對路徑引用（不要放個資、學號、手機、密碼）。

| 圖片 | 圖說 |
| --- | --- |
| `docs/01_board.jpg` | NUCLEO-G474RE 接上三顆外接 LED 與 PC9 暫停按鈕的實體接線 |
| `docs/02_terminal_set_start.png` | 終端機送 `SET 1`、`START` 與板子回 `收到指令: [...]` 的畫面 |
| `docs/03_leds_dimming.jpg` | 三燈依序漸暗進行中的樣子 |
| `docs/04_time_up.png` | 三燈全暗、終端機顯示 `TIME_UP` 的畫面 |

---

## 9. AI 使用說明

| 項目 | 說明 |
| --- | --- |
| 哪些段落讓 AI 做 | 依本倉庫程式碼反推腳位表、時脈與 PWM 參數；協助整理本 README 的 10 節結構與表格 |
| 你怎麼查核 | 直接對照 `we1.ioc`（PA0/PA1/PB10=TIM2 CH1/2/3、PA2/PA3=LPUART1、PC9=GPXTI9）與 `Core/Src/main.c`（狀態機、回呼、漸暗迴圈）；親自算一次 `SET 1` 的步數與延遲是否吻合程式註解 |
| 未讓 AI 決定的部分 | 實體接線、腳位對應、燒錄指令以實際硬體與 CubeIDE 為準；AI 僅整理，不替你改腳位 |

---

## 10. 已知限制（哪些是替身、換真實要改哪裡）

1. **暫停用忙等（busy-wait）**：`while(pause_flag){...}` 會卡住主迴圈，
   暫停期間無法做其他工作；若要併行，應改用 RTOS 或計時器中斷。
2. **指令為單向**：只有「設定 / 開始」，沒有查詢剩餘時間、取消、暫停的指令；
   想加取消可新增 `CANCEL` 解析並 `state=0`。
3. **無 RTC / 斷電不保持**：重開機設定消失；換真實可接 32.768kHz LSE（ioc 已預留 RTC 時脈）。
4. **最小單位為「分鐘」**：`SET` 以分鐘計，最小 1 分鐘；要秒級需改指令解析與 `×60000`。
5. **時間用 `×0.88` 硬校正**：`calibrated_ms = total_ms × 88 / 100` 是針對 `HAL_Delay` 累積誤差的
   經驗值，非精準計時；精準需求應改用硬體定時器中斷計時。
6. **RX 緩衝僅 30 bytes**，且字元過濾只收 32–126；過長指令或夾雜控制字元可能被截斷。
7. **型別混用**：`sscanf(rx_buffer, "SET %lu", &temp_val)` 搭配 `uint32_t`，屬可接受的寫法但需注意平台寬度。
8. **替身未做外殼 / 真實顯示**：LED 與按鈕皆外接，未在板載 LD2 / B1；
   換真實裝置時，把 LED 替身換成段顯 / 螢幕，把 `TIME_UP` 換成蜂鳴器即可。

---

### 專案資訊
- 開發板：NUCLEO-G474RE（STM32G474RET6）
- 工具鏈：STM32CubeMX 6.18.1 / STM32CubeIDE
- 系統時脈：HSI 16MHz → PLL（PLLM÷4、PLLN=85、PLLR÷2）→ SYSCLK 170MHz
- 專案位置：`weeeeeeeeeeee/we1`
