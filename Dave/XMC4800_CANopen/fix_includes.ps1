# Fix Include Paths for CANopen Integration
# 修正 CANopen 整合的 Include 路徑

Write-Host "=== 修正 CANopen Include 路徑 ==="

# 檢查必要目錄
$projectRoot = Get-Location
$directories = @(
    "CANopenNode",
    "CANopenNode/301", 
    "port",
    "application"
)

Write-Host "檢查必要目錄..."
foreach ($dir in $directories) {
    if (Test-Path $dir) {
        Write-Host "✓ $dir 存在"
    }
    else {
        Write-Host "✗ $dir 遺失" -ForegroundColor Red
    }
}

# 建立包含所有必要路徑的編譯命令
$includes = @(
    "-I`"$projectRoot`"",
    "-I`"$projectRoot/CANopenNode`"",
    "-I`"$projectRoot/CANopenNode/301`"",
    "-I`"$projectRoot/port`"",
    "-I`"$projectRoot/application`"",
    "-I`"$projectRoot/Dave/Generated`"",
    "-I`"$projectRoot/Libraries/XMCLib/inc`"",
    "-I`"$projectRoot/Libraries/CMSIS/Include`"",
    "-I`"$projectRoot/Libraries/CMSIS/Infineon/XMC4800_series/Include`""
)

Write-Host "`n必要的 Include 路徑:"
foreach ($inc in $includes) {
    Write-Host "  $inc"
}

# 建立手動編譯命令
$gccPath = "C:/Infineon/Tools/DAVE IDE/4.5.0.202105191637/eclipse/ARM-GCC-49/bin/arm-none-eabi-gcc"
$compileFlags = @(
    "-DXMC4800_F144x2048",
    "-O0",
    "-ffunction-sections", 
    "-fdata-sections",
    "-Wall",
    "-std=gnu99",
    "-mfloat-abi=softfp",
    "-mcpu=cortex-m4",
    "-mfpu=fpv4-sp-d16", 
    "-mthumb",
    "-g",
    "-gdwarf-2"
)

$includeString = $includes -join " "
$flagString = $compileFlags -join " "

$compileCommand = "`"$gccPath`" $flagString $includeString -c -o main.o main.c"

Write-Host "`n手動編譯命令:"
Write-Host $compileCommand -ForegroundColor Green

# 執行編譯測試
Write-Host "`n執行編譯測試..."
try {
    Invoke-Expression $compileCommand
    if (Test-Path "main.o") {
        Write-Host "✓ 編譯成功！產生 main.o" -ForegroundColor Green
        Get-ChildItem main.o | Select-Object Name, Length, LastWriteTime
    }
    else {
        Write-Host "✗ 編譯失敗 - 未產生 main.o" -ForegroundColor Red
    }
}
catch {
    Write-Host "✗ 編譯錯誤: $($_.Exception.Message)" -ForegroundColor Red
}

Write-Host "`n=== 完成 ==="