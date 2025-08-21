# VS Code + DAVE IDE XMC4800 CANopen 開發環境配置

## 開發環境架構

### 混合開發模式
- **VS Code**: 主要程式碼編輯、版本控制、專案管理
- **DAVE IDE**: XMC4800 專案建立、硬體配置、編譯燒錄
- **Git**: 版本控制和 CANopenNode 整合

## VS Code 擴展套件安裝

### 必要擴展套件
```json
{
  "recommendations": [
    "ms-vscode.cpptools",           // C/C++ IntelliSense
    "ms-vscode.cpptools-extension-pack", // C/C++ 擴展包
    "ms-vscode.cmake-tools",        // CMake 支援
    "twxs.cmake",                   // CMake 語法高亮
    "ms-vscode.vscode-serial-monitor", // 串列埠監控
    "marus25.cortex-debug",         // ARM Cortex 調試
    "dan-c-underwood.arm",          // ARM 組合語言支援
    "ms-vscode.hexeditor",          // 十六進制編輯器
    "streetsidesoftware.code-spell-checker", // 拼字檢查
    "ms-vscode.vscode-json"         // JSON 支援
  ]
}
```

### CANopen 開發輔助擴展
```json
{
  "additional_recommendations": [
    "ms-vscode.live-server",        // 文件預覽
    "yzhang.markdown-all-in-one",   // Markdown 支援
    "shardulm94.trailing-spaces",   // 清除多餘空格
    "ms-vscode.theme-tomorrow-kit", // 深色主題
    "formulahendry.code-runner"     // 程式碼執行器
  ]
}
```

## VS Code 工作區配置

### 工作區設定檔 (.vscode/settings.json)
```json
{
  "C_Cpp.default.compilerPath": "C:/Infineon/Tools/DAVE IDE/4.5.0.202105191637/eclipse/ARM-GCC/bin/arm-none-eabi-gcc.exe",
  "C_Cpp.default.cStandard": "c11",
  "C_Cpp.default.cppStandard": "c++14",
  "C_Cpp.default.intelliSenseMode": "gcc-arm",
  "C_Cpp.default.includePath": [
    "${workspaceFolder}/**",
    "${workspaceFolder}/src/**",
    "${workspaceFolder}/CANopenNode/**",
    "${workspaceFolder}/Dave/Generated/**",
    "C:/Infineon/Tools/DAVE IDE/4.5.0.202105191637/eclipse/ARM-GCC/arm-none-eabi/include/**"
  ],
  "C_Cpp.default.defines": [
    "XMC4800_E196x2048",
    "ARM_MATH_CM4",
    "__FPU_PRESENT=1",
    "CO_USE_GLOBALS=1"
  ],
  "files.associations": {
    "*.h": "c",
    "*.c": "c",
    "*.ld": "linkerscript",
    "*.eds": "ini"
  },
  "editor.insertSpaces": true,
  "editor.tabSize": 4,
  "editor.detectIndentation": false,
  "files.encoding": "utf8",
  "files.eol": "\n",
  "git.ignoreLimitWarning": true,
  "cortex-debug.armToolchainPath": "C:/Infineon/Tools/DAVE IDE/4.5.0.202105191637/eclipse/ARM-GCC/bin",
  "cortex-debug.JLinkGDBServerPath": "C:/Program Files (x86)/SEGGER/JLink/JLinkGDBServerCL.exe"
}
```

### 編譯任務配置 (.vscode/tasks.json)
```json
{
  "version": "2.0.0",
  "tasks": [
    {
      "label": "Build with DAVE",
      "type": "shell",
      "command": "C:/Infineon/Tools/DAVE IDE/4.5.0.202105191637/eclipse/eclipse.exe",
      "args": [
        "-nosplash",
        "-application",
        "org.eclipse.cdt.managedbuilder.core.headlessbuild",
        "-data",
        "${workspaceFolder}/Dave",
        "-build",
        "XMC4800_CANopen/Debug"
      ],
      "group": {
        "kind": "build",
        "isDefault": true
      },
      "presentation": {
        "echo": true,
        "reveal": "always",
        "focus": false,
        "panel": "shared"
      },
      "problemMatcher": [
        "$gcc"
      ]
    },
    {
      "label": "Clean with DAVE",
      "type": "shell",
      "command": "C:/Infineon/Tools/DAVE IDE/4.5.0.202105191637/eclipse/eclipse.exe",
      "args": [
        "-nosplash",
        "-application",
        "org.eclipse.cdt.managedbuilder.core.headlessbuild",
        "-data",
        "${workspaceFolder}/Dave",
        "-cleanBuild",
        "XMC4800_CANopen/Debug"
      ],
      "group": "build",
      "presentation": {
        "echo": true,
        "reveal": "always",
        "focus": false,
        "panel": "shared"
      }
    },
    {
      "label": "Flash with J-Link",
      "type": "shell",
      "command": "C:/Program Files (x86)/SEGGER/JLink/JLink.exe",
      "args": [
        "-device",
        "XMC4800-2048",
        "-if",
        "SWD",
        "-speed",
        "4000",
        "-CommanderScript",
        "${workspaceFolder}/scripts/flash.jlink"
      ],
      "group": "test",
      "dependsOn": "Build with DAVE"
    }
  ]
}
```

### 調試配置 (.vscode/launch.json)
```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug XMC4800 (J-Link)",
      "cwd": "${workspaceFolder}",
      "executable": "${workspaceFolder}/Dave/XMC4800_CANopen/Debug/XMC4800_CANopen.elf",
      "request": "launch",
      "type": "cortex-debug",
      "runToEntryPoint": "main",
      "servertype": "jlink",
      "device": "XMC4800-2048",
      "interface": "swd",
      "serverArgs": [
        "-singlerun",
        "-strict",
        "-timeout",
        "0",
        "-nogui"
      ],
      "svdFile": "${workspaceFolder}/scripts/XMC4800.svd",
      "preLaunchTask": "Build with DAVE",
      "armToolchainPath": "C:/Infineon/Tools/DAVE IDE/4.5.0.202105191637/eclipse/ARM-GCC/bin"
    },
    {
      "name": "Attach to Target",
      "cwd": "${workspaceFolder}",
      "executable": "${workspaceFolder}/Dave/XMC4800_CANopen/Debug/XMC4800_CANopen.elf",
      "request": "attach",
      "type": "cortex-debug",
      "servertype": "jlink",
      "device": "XMC4800-2048",
      "interface": "swd"
    }
  ]
}
```

## 專案目錄結構

### 推薦的目錄結構
```
c:\prj\AI\CANOpen\
├── .vscode/                    # VS Code 配置
│   ├── settings.json
│   ├── tasks.json
│   ├── launch.json
│   └── extensions.json
├── Dave/                       # DAVE IDE 專案
│   ├── XMC4800_CANopen/       # DAVE 專案主目錄
│   └── Generated/             # DAVE 自動產生檔案
├── src/                        # 主要原始碼
│   ├── main.c
│   ├── can_driver/
│   ├── canopen/
│   └── application/
├── CANopenNode/               # Git submodule
├── scripts/                   # 輔助腳本
│   ├── flash.jlink
│   └── XMC4800.svd
├── docs/                      # 文件
├── tests/                     # 測試程式
└── tools/                     # 開發工具
```

## 工作流程

### 1. 專案建立流程
```bash
# 1. 在 VS Code 中打開專案資料夾
# 2. 使用 DAVE IDE 建立 XMC4800 專案
# 3. 在 VS Code 中配置工作區設定
# 4. 新增 CANopenNode 作為 Git submodule
```

### 2. 日常開發流程
```
VS Code 編輯程式碼 → DAVE IDE 編譯 → J-Link 燒錄 → VS Code 調試
```

### 3. 版本控制流程
```bash
# Git 工作流程
git add .
git commit -m "描述變更"
git push origin main

# CANopenNode 更新
git submodule update --remote
```

## DAVE IDE 整合

### DAVE 專案設定
1. **建立新專案**
   - 開啟 DAVE IDE
   - File → New → DAVE Project
   - 選擇 XMC4800-E196x2048
   - 專案名稱：XMC4800_CANopen

2. **硬體配置 (DAVE Apps)**
   ```
   必要的 DAVE Apps：
   ├── CLOCK_XMC4         # 系統時鐘
   ├── CPU_CTRL_XMC4      # CPU 控制
   ├── CAN_NODE           # CAN 節點
   ├── DIGITAL_IO         # GPIO 控制
   ├── UART               # 除錯輸出
   └── SYSTIMER           # 系統定時器
   ```

3. **CAN 節點配置**
   ```
   CAN Node Settings：
   ├── Node: CAN_NODE_0
   ├── Bitrate: 500000 bps
   ├── Sample Point: 87.5%
   ├── Tx Pin: P1.13
   └── Rx Pin: P1.12
   ```

### 建置設定
```makefile
# 編譯器設定
CC = arm-none-eabi-gcc
CFLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS += -DXMC4800_E196x2048 -DARM_MATH_CM4 -D__FPU_PRESENT=1

# 連結器設定
LDSCRIPT = XMC4800x2048.ld
LDFLAGS = -T$(LDSCRIPT) -Wl,-Map=output.map

# 包含路徑
INCLUDES = -Isrc -ICANopenNode -IDave/Generated
```

## 輔助工具設定

### J-Link 腳本 (scripts/flash.jlink)
```
connect
halt
loadfile Dave/XMC4800_CANopen/Debug/XMC4800_CANopen.hex
r
g
exit
```

### Git 配置 (.gitignore)
```gitignore
# DAVE IDE 產生檔案
Dave/Generated/
Dave/.metadata/
Dave/**/Debug/
Dave/**/Release/
*.launch

# VS Code 暫存檔案
.vscode/ipch/
.vscode/.browse.VC.db*

# 編譯產生檔案
*.o
*.elf
*.hex
*.bin
*.map

# 其他
.DS_Store
Thumbs.db
```

## 開發輔助功能

### 程式碼片段 (.vscode/snippets/c.json)
```json
{
  "CANopen Function": {
    "prefix": "cofunc",
    "body": [
      "/**",
      " * @brief ${1:Description}",
      " * @param ${2:param} ${3:Description}",
      " * @return ${4:CO_ReturnError_t}",
      " */",
      "${4:CO_ReturnError_t} ${1:functionName}(${5:parameters}) {",
      "    ${6:// Implementation}",
      "    return CO_ERROR_NO;",
      "}"
    ],
    "description": "CANopen function template"
  },
  "XMC CAN Init": {
    "prefix": "xmccan",
    "body": [
      "/* XMC4800 CAN initialization */",
      "XMC_CAN_NODE_t *can_node = CAN_NODE_${1:0}_HW;",
      "XMC_CAN_Init(CAN, XMC_CAN_CANCLKSRC_FPERI, ${2:120000000});",
      "XMC_CAN_NODE_Init(can_node);",
      "XMC_CAN_NODE_EnableConfigurationChange(can_node);",
      "XMC_CAN_NODE_SetBaudrate(can_node, ${3:500000});",
      "XMC_CAN_NODE_DisableConfigurationChange(can_node);",
      "${4:// Additional configuration}"
    ],
    "description": "XMC4800 CAN initialization code"
  }
}
```

### 建置和除錯快捷鍵
```json
{
  "key": "ctrl+shift+b",
  "command": "workbench.action.tasks.build"
},
{
  "key": "f5",
  "command": "workbench.action.debug.start"
},
{
  "key": "shift+f5",
  "command": "workbench.action.debug.stop"
},
{
  "key": "ctrl+shift+f5",
  "command": "workbench.action.debug.restart"
}
```

## 故障排除

### 常見問題解決

1. **編譯器路徑錯誤**
   - 檢查 DAVE IDE 安裝路徑
   - 更新 settings.json 中的編譯器路徑

2. **J-Link 連接問題**
   - 確認 J-Link 驅動程式已安裝
   - 檢查硬體連接
   - 驗證 launch.json 中的設備名稱

3. **IntelliSense 問題**
   - 重新載入視窗 (Ctrl+Shift+P → "Developer: Reload Window")
   - 檢查 includePath 設定
   - 確認 C++ 擴展套件已啟用

### 除錯技巧

1. **使用 SVD 檔案**
   - 下載 XMC4800.svd 檔案
   - 在調試時查看周邊暫存器

2. **串列埠除錯**
   - 配置 UART 輸出
   - 使用 VS Code Serial Monitor

3. **CAN 訊息監控**
   - 使用 CAN 分析儀
   - 實現軟體 CAN 訊息記錄

---

**設定完成後的優勢**：
- 在 VS Code 中享受現代化的編輯體驗
- 保留 DAVE IDE 的硬體配置和編譯功能
- 完整的調試和版本控制支援
- 高效的 CANopen 開發工作流程