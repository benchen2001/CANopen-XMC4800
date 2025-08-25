# DAVE IDE 燒錄設定指南

## 問題解決：無法燒錄 xmc4800_canopen_monitor.elf

### 問題分析

目前無法燒錄的主要原因：

1. **ELF 檔案格式問題** - 目前的建置腳本可能生成的是模擬檔案
2. **DAVE IDE 專案配置缺失** - 缺少完整的 DAVE 專案結構
3. **連結腳本和啟動檔案缺失** - 沒有 XMC4800 特定的記憶體配置

### 解決方案

## 方案一：使用 J-Link 直接燒錄 (推薦)

### 1. 安裝 SEGGER J-Link

下載並安裝 SEGGER J-Link 軟體套件：
- 官方網站：https://www.segger.com/downloads/jlink/
- 選擇 "J-Link Software and Documentation Pack"
- 安裝到預設路徑

### 2. 使用燒錄腳本

```powershell
# 檢查可用的 ELF 檔案
.\flash_program.ps1 -List

# 燒錄監控程式
.\flash_program.ps1 -Target monitor

# 或直接指定檔案
.\flash_program.ps1 -ElfFile .\build\xmc4800_canopen_monitor.elf
```

### 3. 手動 J-Link 燒錄

如果自動腳本失敗，可以手動使用 J-Link Commander：

```bash
# 啟動 J-Link Commander
"C:\Program Files\SEGGER\JLink\JLink.exe"

# 在 J-Link Commander 中執行：
device XMC4800-2048
si SWD
speed 4000
connect
halt
loadfile C:\prj\AI\CANOpen\build\xmc4800_canopen_monitor.elf
reset
go
exit
```

## 方案二：建立完整的 DAVE 專案

### 1. 在 DAVE IDE 中建立新專案

1. 開啟 DAVE IDE 4.5.0
2. File → New → DAVE Project
3. 選擇：
   - Microcontroller: XMC4800-F144x2048
   - Project Name: XMC4800_CANopen_Monitor
   - Location: `C:\prj\AI\CANOpen\dave_workspace`

### 2. 配置 DAVE APP

在 DAVE 專案中添加必要的 APP：

#### CPU APP
- **CPU_CTRL_XMC4**: 系統時鐘配置
  - Main PLL: 120 MHz
  - CPU Clock: 120 MHz
  - Peripheral Clock: 60 MHz

#### CAN APP  
- **CAN_NODE**: CAN 控制器配置
  - Node ID: CAN_NODE_0
  - Bit Rate: 125000 bps
  - TX Pin: P1.9
  - RX Pin: P1.8

#### UART APP (除錯輸出)
- **UART**: 串列通訊配置
  - Baud Rate: 115200
  - TX Pin: P1.5
  - RX Pin: P1.4

#### GPIO APP (狀態 LED)
- **DIGITAL_IO**: GPIO 配置
  - Pin: P1.1 (開發板 LED)
  - Mode: Output Push Pull

### 3. 複製源碼到 DAVE 專案

將 `xmc4800_canopen_monitor.c` 的內容複製到 DAVE 專案的 `main.c`：

```c
// 在 main.c 中包含必要的標頭檔案
#include <DAVE.h>

// 複製 xmc4800_canopen_monitor.c 的內容
// 並適配 DAVE APP 的 API
```

### 4. 適配 DAVE API

修改源碼以使用 DAVE APP API：

```c
// CAN 初始化
CAN_NODE_Init(&CAN_NODE_0);

// CAN 訊息接收
CAN_NODE_MO_Receive(&CAN_NODE_0, &rx_message);

// UART 除錯輸出
UART_Transmit(&UART_0, debug_buffer, length);

// LED 狀態指示
DIGITAL_IO_ToggleOutput(&DIGITAL_IO_0);
```

### 5. 在 DAVE IDE 中建置和燒錄

1. 右鍵點擊專案 → Build Project
2. 右鍵點擊專案 → Debug As → GDB SEGGER J-Link Debugging
3. 配置除錯器設定：
   - Debugger: SEGGER J-Link
   - Device: XMC4800-F144x2048
   - Interface: SWD
   - Speed: 4000 kHz

## 方案三：修正建置腳本

### 1. 安裝 ARM GCC 工具鏈

如果您有獨立的 ARM GCC 工具鏈：

```powershell
# 檢查 ARM GCC 是否安裝
arm-none-eabi-gcc --version

# 如果沒有，可以使用 DAVE IDE 內建的編譯器
$env:PATH += ";C:\Infineon\DAVE IDE 4.5.0\eclipse\ARM-GCC-49\bin"
```

### 2. 建立完整的建置配置

建立 `linker_script.ld` 檔案：

```ld
/* XMC4800 Memory Layout */
MEMORY
{
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 2048K
    RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 352K
}

/* Entry point */
ENTRY(Reset_Handler)

SECTIONS
{
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        . = ALIGN(4);
    } > FLASH

    .data : AT(LOADADDR(.text) + SIZEOF(.text))
    {
        . = ALIGN(4);
        *(.data)
        *(.data*)
        . = ALIGN(4);
    } > RAM

    .bss :
    {
        . = ALIGN(4);
        *(.bss)
        *(.bss*)
        . = ALIGN(4);
    } > RAM
}
```

### 3. 建立啟動檔案

建立 `startup_xmc4800.s` 檔案（簡化版本）：

```assembly
.syntax unified
.cpu cortex-m4
.thumb

/* Reset Handler */
.section .text.Reset_Handler
.weak Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
    /* Set stack pointer */
    ldr sp, =_estack
    
    /* Call main function */
    bl main
    
    /* Infinite loop */
loop:
    b loop
.size Reset_Handler, .-Reset_Handler

/* Vector Table */
.section .isr_vector,"a",%progbits
.type g_pfnVectors, %object
.size g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
    .word _estack
    .word Reset_Handler
```

## 除錯提示

### 1. 檢查硬體連接

- 確認 J-Link 連接到 XMC4800 的 SWD 接口
- 檢查電源供應（可通過 USB 或外部電源）
- 確認 Boot 模式設定正確

### 2. 檢查軟體配置

```powershell
# 檢查 J-Link 版本
"C:\Program Files\SEGGER\JLink\JLink.exe" -v

# 檢查 DAVE IDE 安裝
Get-ChildItem "C:\Infineon\DAVE IDE 4.5.0" -Recurse -Name "*.exe"
```

### 3. 常見錯誤解決

**錯誤：「Cannot connect to target」**
- 檢查 SWD 連接線
- 降低 J-Link 速度到 1000 kHz
- 確認目標板電源

**錯誤：「Invalid ELF file」**
- 確認 ELF 檔案是用正確的編譯器建置
- 檢查目標處理器設定
- 重新建置專案

**錯誤：「Programming failed」**
- 檢查 Flash 寫入保護
- 確認記憶體映射正確
- 嘗試 Mass Erase

## 推薦工作流程

1. **立即解決方案**：使用 J-Link 和現有的 ELF 檔案
2. **短期方案**：建立完整的 DAVE 專案
3. **長期方案**：建立完整的 Makefile 建置系統

### 立即測試步驟

```powershell
# 1. 檢查環境
.\flash_program.ps1 -Help

# 2. 列出可用檔案  
.\flash_program.ps1 -List

# 3. 嘗試燒錄
.\flash_program.ps1 -Target monitor

# 4. 如果失敗，檢查 J-Link 連接
"C:\Program Files\SEGGER\JLink\JLink.exe" -commanderscript test_connection.jlink
```

如果還有問題，請提供：
1. 錯誤訊息的完整內容
2. 使用的燒錄方法
3. 硬體連接狀況
4. DAVE IDE 版本資訊