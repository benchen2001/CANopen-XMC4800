#******************************************************************************
# Makefile for XMC4800 CANopen Project
# 測試編譯配置
#******************************************************************************

# 項目名稱
PROJECT = XMC4800_CANopen_test

# 目標平台
MCU = xmc4800

# 編譯器工具鏈
TOOLCHAIN_PATH = C:/Infineon/Tools/DAVE IDE/4.5.0.202105191637/eclipse/ARM-GCC-49/bin
CC = "$(TOOLCHAIN_PATH)/arm-none-eabi-gcc.exe"
AS = "$(TOOLCHAIN_PATH)/arm-none-eabi-gcc.exe"
LD = "$(TOOLCHAIN_PATH)/arm-none-eabi-gcc.exe"
OBJCOPY = "$(TOOLCHAIN_PATH)/arm-none-eabi-objcopy.exe"
OBJDUMP = "$(TOOLCHAIN_PATH)/arm-none-eabi-objdump.exe"
SIZE = "$(TOOLCHAIN_PATH)/arm-none-eabi-size.exe"
MAKE = "$(TOOLCHAIN_PATH)/make.exe"

# 編譯選項
CFLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -Wall -Wextra -Wno-unused-parameter
CFLAGS += -std=c99
CFLAGS += -O2 -g

# 定義
DEFINES = -DXMC4800_E196x2048
DEFINES += -DUSE_DAVE_IDE

# 包含路徑
INCLUDES = -Isrc
INCLUDES += -Isrc/canopen
INCLUDES += -ICANopenNode
INCLUDES += -ICANopenNode/301
INCLUDES += -IDave/XMC4800_CANopen/Dave/Generated
INCLUDES += -IDave/XMC4800_CANopen/Libraries/XMCLib/inc
INCLUDES += -IDave/XMC4800_CANopen/Libraries/CMSIS/Include
INCLUDES += -IDave/XMC4800_CANopen/Libraries/CMSIS/Infineon/XMC4800_series/Include

# 連接器選項
LDFLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,--print-memory-usage

# 源文件
SOURCES = src/main.c
SOURCES += src/canopen/can_xmc4800.c

# 如果 DAVE 文件存在，添加它們
ifneq (,$(wildcard Dave/XMC4800_CANopen/Dave/Generated/*.c))
    SOURCES += $(wildcard Dave/XMC4800_CANopen/Dave/Generated/*.c)
endif

# 目標文件
OBJECTS = $(SOURCES:.c=.o)

# 預設目標
all: check-env $(PROJECT).elf size

# DAVE 整合測試目標
dave-test: check-env
	@echo "=== 編譯 DAVE 整合測試 ==="
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c test_dave_integration.c -o test_dave_integration.o
	@echo "✓ DAVE 整合測試編譯成功"
	$(SIZE) test_dave_integration.o

# 編譯 CAN 驅動測試
can-driver-test: check-env
	@echo "=== 編譯 CAN 驅動 ==="
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c can_xmc4800.c -o can_xmc4800.o
	@echo "✓ CAN 驅動編譯成功"
	$(SIZE) can_xmc4800.o

# 檢查編譯環境
check-env:
	@echo "=== 檢查編譯環境 ==="
	@which $(CC) > /dev/null || (echo "錯誤: 找不到 ARM GCC 編譯器"; exit 1)
	@echo "編譯器: $$($(CC) --version | head -n1)"
	@echo "目標平台: $(MCU)"
	@echo "專案名稱: $(PROJECT)"
	@echo ""

# 編譯目標
$(PROJECT).elf: $(OBJECTS)
	@echo "=== 連接 $(PROJECT).elf ==="
	$(LD) $(LDFLAGS) -o $@ $^

# 編譯 C 源文件
%.o: %.c
	@echo "編譯: $<"
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

# 顯示記憶體使用
size: $(PROJECT).elf
	@echo "=== 記憶體使用統計 ==="
	$(SIZE) $(PROJECT).elf

# 測試編譯 (不生成執行檔)
test-compile:
	@echo "=== 測試編譯 (僅語法檢查) ==="
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -fsyntax-only src/main.c
	@echo "✓ main.c 語法檢查通過"
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -fsyntax-only src/canopen/can_xmc4800.c
	@echo "✓ can_xmc4800.c 語法檢查通過"
	@echo "✓ 所有源文件語法檢查通過"

# 檢查標頭檔相依性
check-headers:
	@echo "=== 檢查標頭檔相依性 ==="
	@echo "檢查 CO_driver_target.h..."
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -E src/canopen/CO_driver_target.h > /dev/null
	@echo "✓ CO_driver_target.h 檢查通過"
	@echo "檢查 can_xmc4800.h..."
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -E src/canopen/can_xmc4800.h > /dev/null
	@echo "✓ can_xmc4800.h 檢查通過"

# 清理
clean:
	@echo "=== 清理編譯產物 ==="
	rm -f $(OBJECTS)
	rm -f $(PROJECT).elf
	rm -f $(PROJECT).hex
	rm -f $(PROJECT).bin
	@echo "✓ 清理完成"

# 顯示幫助
help:
	@echo "可用目標:"
	@echo "  all           - 完整編譯"
	@echo "  test-compile  - 僅測試語法"
	@echo "  check-headers - 檢查標頭檔"
	@echo "  check-env     - 檢查編譯環境"
	@echo "  clean         - 清理編譯產物"
	@echo "  help          - 顯示此幫助"

.PHONY: all clean test-compile check-headers check-env size help