# XMC4800 程式燒錄工具
# 支援使用 J-Link 和 DAVE IDE 燒錄 ELF 檔案

param(
    [string]$Target = "",
    [string]$ElfFile = "",
    [string]$Method = "jlink",  # jlink, dave, segger
    [switch]$List = $false,
    [switch]$Help = $false
)

# 設定控制台輸出編碼
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

# 顏色定義
$Colors = @{
    Success = "Green"
    Warning = "Yellow" 
    Error   = "Red"
    Info    = "Cyan"
    Header  = "Magenta"
}

function Write-ColorOutput {
    param(
        [string]$Message,
        [string]$Color = "White"
    )
    Write-Host $Message -ForegroundColor $Colors[$Color]
}

function Show-Help {
    Write-ColorOutput "=== XMC4800 程式燒錄工具 ===" "Header"
    Write-Host ""
    Write-ColorOutput "用法:" "Info"
    Write-Host "  .\flash_program.ps1 [-Target <目標>] [-ElfFile <檔案>] [-Method <方法>]"
    Write-Host ""
    Write-ColorOutput "參數:" "Info"
    Write-Host "  -Target <目標>    指定預定義的建置目標"
    Write-Host "  -ElfFile <檔案>   直接指定 ELF 檔案路徑"
    Write-Host "  -Method <方法>    燒錄方法 (jlink, dave, segger)"
    Write-Host "  -List             列出可用的 ELF 檔案"
    Write-Host "  -Help             顯示此說明"
    Write-Host ""
    Write-ColorOutput "燒錄方法:" "Info"
    Write-Host "  jlink    使用 J-Link Commander (推薦)"
    Write-Host "  dave     使用 DAVE IDE 命令列"
    Write-Host "  segger   使用 SEGGER Ozone"
    Write-Host ""
    Write-ColorOutput "可用目標:" "Info"
    Write-Host "  basic-test   基本 CAN 硬體測試"
    Write-Host "  monitor      CANopen 網路監控工具"
    Write-Host "  simple       簡化 CANopen 節點"
    Write-Host "  main         完整 CANopen 節點"
    Write-Host ""
    Write-ColorOutput "範例:" "Info"
    Write-Host "  .\flash_program.ps1 -Target monitor"
    Write-Host "  .\flash_program.ps1 -ElfFile .\build\xmc4800_canopen_monitor.elf"
    Write-Host "  .\flash_program.ps1 -List"
}

# 專案路徑設定
$ProjectRoot = Split-Path $MyInvocation.MyCommand.Path -Parent
$BuildPath = "$ProjectRoot\build"
$ScriptsPath = "$ProjectRoot\scripts"

# 目標檔案對應
$TargetFiles = @{
    "basic-test"         = "xmc4800_can_basic_test.elf"
    "monitor"            = "xmc4800_canopen_monitor.elf"
    "monitor-standalone" = "xmc4800_canopen_monitor_standalone.elf"
    "simple"             = "xmc4800_canopen_simple.elf"
    "main"               = "xmc4800_canopen_main.elf"
}

# J-Link 設定
$JLinkConfig = @{
    Device    = "XMC4800-2048"
    Interface = "SWD"
    Speed     = "4000"
    JLinkExe  = ""  # 將在檢查時填入
}

# DAVE IDE 設定
$DaveConfig = @{
    WorkspacePath = "$ProjectRoot\dave_workspace"
    EclipseExe    = ""  # 將在檢查時填入
}

function Find-JLinkExecutable {
    # 根據 VS Code 設定檔的路徑
    $VSCodePath = "C:\Program Files (x86)\SEGGER\JLink\JLink.exe"
    
    $PossiblePaths = @(
        $VSCodePath,
        "${env:ProgramFiles}\SEGGER\JLink\JLink.exe",
        "${env:ProgramFiles(x86)}\SEGGER\JLink\JLink.exe",
        "C:\Program Files\SEGGER\JLink\JLink.exe",
        "C:\Program Files (x86)\SEGGER\JLink\JLink.exe"
    )
    
    foreach ($Path in $PossiblePaths) {
        if (Test-Path $Path) {
            return $Path
        }
    }
    return $null
}

function Find-DaveExecutable {
    $PossiblePaths = @(
        "${env:ProgramFiles}\Infineon\DAVE IDE 4.5.0\eclipse\DAVE.exe",
        "${env:ProgramFiles(x86)}\Infineon\DAVE IDE 4.5.0\eclipse\DAVE.exe",
        "C:\Infineon\DAVE IDE 4.5.0\eclipse\DAVE.exe"
    )
    
    foreach ($Path in $PossiblePaths) {
        if (Test-Path $Path) {
            return $Path
        }
    }
    return $null
}

function Test-FlashingEnvironment {
    Write-ColorOutput "檢查燒錄環境..." "Info"
    
    # 檢查建置目錄
    if (-not (Test-Path $BuildPath)) {
        Write-ColorOutput "錯誤: 建置目錄不存在: $BuildPath" "Error"
        return $false
    }
    
    # 尋找 J-Link
    $JLinkConfig.JLinkExe = Find-JLinkExecutable
    if ($JLinkConfig.JLinkExe) {
        Write-ColorOutput "✓ 找到 J-Link: $($JLinkConfig.JLinkExe)" "Success"
    }
    else {
        Write-ColorOutput "⚠ 未找到 J-Link 執行檔" "Warning"
    }
    
    # 尋找 DAVE IDE
    $DaveConfig.EclipseExe = Find-DaveExecutable
    if ($DaveConfig.EclipseExe) {
        Write-ColorOutput "✓ 找到 DAVE IDE: $($DaveConfig.EclipseExe)" "Success"
    }
    else {
        Write-ColorOutput "⚠ 未找到 DAVE IDE 執行檔" "Warning"
    }
    
    return $true
}

function List-AvailableElfFiles {
    Write-ColorOutput "=== 可用的 ELF 檔案 ===" "Header"
    
    if (-not (Test-Path $BuildPath)) {
        Write-ColorOutput "建置目錄不存在: $BuildPath" "Error"
        return
    }
    
    $ElfFiles = Get-ChildItem -Path $BuildPath -Filter "*.elf"
    
    if ($ElfFiles.Count -eq 0) {
        Write-ColorOutput "沒有找到 ELF 檔案" "Warning"
        Write-ColorOutput "請先執行 .\build_all.ps1 建置專案" "Info"
        return
    }
    
    foreach ($File in $ElfFiles) {
        $FileSize = [math]::Round($File.Length / 1024, 2)
        $LastWrite = $File.LastWriteTime.ToString("yyyy-MM-dd HH:mm:ss")
        Write-ColorOutput "檔案: $($File.Name)" "Info"
        Write-Host "  大小: $FileSize KB"
        Write-Host "  修改時間: $LastWrite"
        Write-Host ""
    }
}

function Create-JLinkScript {
    param (
        [string]$BinPath,
        [string]$ScriptPath
    )
    
    $scriptContent = @"
si 1
speed 12000
device XMC4800-2048
connect
erase
loadbin "$BinPath", 0x08000000
r
g
qc
"@
    
    Set-Content -Path $ScriptPath -Value $scriptContent -Encoding ASCII
}

function Flash-WithJLink {
    param([string]$ElfFilePath)
    
    Write-ColorOutput "使用 J-Link 燒錄..." "Info"
    
    if (-not $JLinkConfig.JLinkExe) {
        Write-ColorOutput "錯誤: 找不到 J-Link 執行檔" "Error"
        Write-ColorOutput "請安裝 SEGGER J-Link 軟體套件" "Info"
        return $false
    }
    
    # 檢查是否為二進制檔案
    $BinFilePath = $ElfFilePath -replace '\.elf$', '.bin'
    $UsesBinaryFormat = Test-Path $BinFilePath
    
    if ($UsesBinaryFormat) {
        Write-ColorOutput "使用二進制格式燒錄: $BinFilePath" "Info"
        return Flash-BinaryWithJLink $BinFilePath
    }
    
    # 建立腳本目錄
    if (-not (Test-Path $ScriptsPath)) {
        New-Item -ItemType Directory -Path $ScriptsPath -Force | Out-Null
    }
    
    $ScriptFile = "$ScriptsPath\flash_temp.jlink"
    Create-JLinkScript $ElfFilePath $ScriptFile
    
    try {
        Write-ColorOutput "正在燒錄 $ElfFilePath..." "Info"
        
        $Process = Start-Process -FilePath $JLinkConfig.JLinkExe -ArgumentList "-CommanderScript", $ScriptFile -Wait -PassThru -NoNewWindow
        
        if ($Process.ExitCode -eq 0) {
            Write-ColorOutput "✓ J-Link 燒錄成功!" "Success"
            return $true
        }
        else {
            Write-ColorOutput "✗ J-Link 燒錄失敗 (退出代碼: $($Process.ExitCode))" "Error"
            return $false
        }
    }
    catch {
        Write-ColorOutput "J-Link 燒錄過程中發生錯誤: $($_.Exception.Message)" "Error"
        return $false
    }
    finally {
        # 清理臨時腳本
        if (Test-Path $ScriptFile) {
            Remove-Item $ScriptFile -Force
        }
    }
}

function Flash-BinaryWithJLink {
    param([string]$BinFilePath)
    
    # 建立腳本目錄
    if (-not (Test-Path $ScriptsPath)) {
        New-Item -ItemType Directory -Path $ScriptsPath -Force | Out-Null
    }
    
    $ScriptFile = "$ScriptsPath\flash_binary.jlink"
    Create-JLinkScript $BinFilePath $ScriptFile
    
    try {
        Write-ColorOutput "正在燒錄二進制檔案 $BinFilePath..." "Info"
        
        $Process = Start-Process -FilePath $JLinkConfig.JLinkExe -ArgumentList "-CommanderScript", $ScriptFile -Wait -PassThru -NoNewWindow
        
        if ($Process.ExitCode -eq 0) {
            Write-ColorOutput "✓ J-Link 二進制燒錄成功!" "Success"
            return $true
        }
        else {
            Write-ColorOutput "✗ J-Link 二進制燒錄失敗 (退出代碼: $($Process.ExitCode))" "Error"
            return $false
        }
    }
    catch {
        Write-ColorOutput "J-Link 二進制燒錄過程中發生錯誤: $($_.Exception.Message)" "Error"
        return $false
    }
    finally {
        # 清理臨時腳本
        if (Test-Path $ScriptFile) {
            Remove-Item $ScriptFile -Force
        }
    }
}

function Flash-WithDave {
    param([string]$ElfFilePath)
    
    Write-ColorOutput "使用 DAVE IDE 燒錄..." "Info"
    
    if (-not $DaveConfig.EclipseExe) {
        Write-ColorOutput "錯誤: 找不到 DAVE IDE 執行檔" "Error"
        Write-ColorOutput "請確保 DAVE IDE 已正確安裝" "Info"
        return $false
    }
    
    if (-not (Test-Path $DaveConfig.WorkspacePath)) {
        Write-ColorOutput "錯誤: DAVE 工作空間不存在: $($DaveConfig.WorkspacePath)" "Error"
        Write-ColorOutput "請先在 DAVE IDE 中建立專案" "Info"
        return $false
    }
    
    try {
        Write-ColorOutput "正在使用 DAVE IDE 燒錄..." "Info"
        
        # DAVE IDE 命令列燒錄 (這需要適當的專案配置)
        $Arguments = @(
            "-data", $DaveConfig.WorkspacePath,
            "-application", "com.infineon.dave.application",
            "-nosplash",
            "-consoleLog",
            "-vmargs", "-Dfile.encoding=UTF-8"
        )
        
        Write-ColorOutput "注意: DAVE IDE 燒錄需要手動操作" "Warning"
        Write-ColorOutput "建議使用 J-Link 方法進行自動化燒錄" "Info"
        
        return $false
    }
    catch {
        Write-ColorOutput "DAVE IDE 燒錄失敗: $($_.Exception.Message)" "Error"
        return $false
    }
}

function Flash-Program {
    param(
        [string]$ElfFilePath,
        [string]$FlashMethod
    )
    
    Write-ColorOutput "=== 開始程式燒錄 ===" "Header"
    Write-ColorOutput "目標檔案: $ElfFilePath" "Info"
    Write-ColorOutput "燒錄方法: $FlashMethod" "Info"
    
    # 檢查檔案是否存在
    if (-not (Test-Path $ElfFilePath)) {
        Write-ColorOutput "錯誤: ELF 檔案不存在: $ElfFilePath" "Error"
        return $false
    }
    
    $FileSize = (Get-Item $ElfFilePath).Length
    Write-ColorOutput "檔案大小: $FileSize bytes" "Info"
    
    switch ($FlashMethod.ToLower()) {
        "jlink" {
            return Flash-WithJLink $ElfFilePath
        }
        "dave" {
            return Flash-WithDave $ElfFilePath
        }
        "segger" {
            Write-ColorOutput "SEGGER Ozone 燒錄尚未實現" "Warning"
            Write-ColorOutput "請使用 SEGGER Ozone 手動載入 ELF 檔案" "Info"
            return $false
        }
        default {
            Write-ColorOutput "不支援的燒錄方法: $FlashMethod" "Error"
            return $false
        }
    }
}

# 主程式邏輯
if ($Help) {
    Show-Help
    exit 0
}

Write-ColorOutput "=== XMC4800 程式燒錄工具 ===" "Header"

# 檢查燒錄環境
if (-not (Test-FlashingEnvironment)) {
    Write-ColorOutput "燒錄環境檢查失敗" "Error"
    exit 1
}

# 列出可用檔案
if ($List) {
    List-AvailableElfFiles
    exit 0
}

# 確定要燒錄的檔案
$TargetElfFile = ""

if ($Target) {
    if ($TargetFiles.ContainsKey($Target)) {
        $TargetElfFile = "$BuildPath\$($TargetFiles[$Target])"
    }
    else {
        Write-ColorOutput "未知的目標: $Target" "Error"
        Write-ColorOutput "可用目標: $($TargetFiles.Keys -join ', ')" "Info"
        exit 1
    }
}
elseif ($ElfFile) {
    if ([System.IO.Path]::IsPathRooted($ElfFile)) {
        $TargetElfFile = $ElfFile
    }
    else {
        $TargetElfFile = Join-Path $PWD $ElfFile
    }
}
else {
    Write-ColorOutput "請指定目標或 ELF 檔案" "Error"
    Write-ColorOutput "使用 -Help 查看使用說明" "Info"
    exit 1
}

# 執行燒錄
if (Flash-Program $TargetElfFile $Method) {
    Write-ColorOutput "程式燒錄完成!" "Success"
    Write-ColorOutput "請檢查開發板是否正常運行" "Info"
}
else {
    Write-ColorOutput "程式燒錄失敗!" "Error"
    Write-ColorOutput "請檢查硬體連接和軟體配置" "Info"
    exit 1
}