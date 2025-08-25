@echo off
echo === XMC4800 CANopen 手動編譯 ===

REM 設定編譯器路徑
set GCC_PATH="C:\Infineon\Tools\DAVE IDE\4.5.0.202105191637\eclipse\ARM-GCC-49\bin\arm-none-eabi-gcc"

REM 設定專案路徑
set PROJECT_ROOT=%CD%

REM Include 路徑
set INCLUDES=-I"%PROJECT_ROOT%" ^
-I"%PROJECT_ROOT%\CANopenNode" ^
-I"%PROJECT_ROOT%\CANopenNode\301" ^
-I"%PROJECT_ROOT%\port" ^
-I"%PROJECT_ROOT%\application" ^
-I"%PROJECT_ROOT%\Dave\Generated" ^
-I"%PROJECT_ROOT%\Libraries\XMCLib\inc" ^
-I"%PROJECT_ROOT%\Libraries\CMSIS\Include" ^
-I"%PROJECT_ROOT%\Libraries\CMSIS\Infineon\XMC4800_series\Include"

REM 編譯旗標
set CFLAGS=-DXMC4800_F144x2048 -O0 -ffunction-sections -fdata-sections -Wall -std=gnu99 -mfloat-abi=softfp -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mthumb -g -gdwarf-2

echo 編譯 main.c...
%GCC_PATH% %CFLAGS% %INCLUDES% -c -o main.o main.c

if exist main.o (
    echo ✓ 編譯成功！
    dir main.o
) else (
    echo ✗ 編譯失敗
    exit /b 1
)

echo 完成