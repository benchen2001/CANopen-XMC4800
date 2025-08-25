# UART 除錯指令
# 1. 使用 J-Link SWO Viewer
& "C:\Program Files\SEGGER\JLink\JLinkSWOViewer.exe"

# 2. 或使用 J-Link Commander 設定 SWO
& "C:\Program Files\SEGGER\JLink\JLink.exe" -if SWD -device XMC4800-2048 -speed 4000 -autoconnect 1

# 在 J-Link Commander 中執行：
# SWOSpeed 4000000
# SWOStart
# 然後會看到 Debug_Printf 的輸出