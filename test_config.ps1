# XMC4800 CANopen 測試配置檔案
# 定義各種測試場景和參數

# 基本 CAN 測試設定
$BasicCanTest = @{
    Name            = "基本 CAN 硬體測試"
    Bitrate         = 125000  # 125 kbps
    TestMode        = "LOOPBACK"  # NORMAL, LOOPBACK, SILENT
    TestDuration    = 30  # 秒
    MessageInterval = 100  # 毫秒
    TestMessages    = @(
        @{ ID = 0x123; Data = @(0x01, 0x02, 0x03, 0x04) },
        @{ ID = 0x456; Data = @(0xAA, 0xBB, 0xCC, 0xDD) },
        @{ ID = 0x789; Data = @(0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88) }
    )
}

# CANopen 網路設定
$CanopenNetwork = @{
    Name              = "CANopen 網路配置"
    Bitrate           = 125000
    NodeId            = 1
    HeartbeatInterval = 1000  # 毫秒
    SyncInterval      = 100  # 毫秒
    PDOConfiguration  = @{
        TPDO1 = @{ COB_ID = 0x181; TransmissionType = 1; InhibitTime = 0 }
        TPDO2 = @{ COB_ID = 0x281; TransmissionType = 254; InhibitTime = 100 }
        RPDO1 = @{ COB_ID = 0x201; TransmissionType = 1 }
        RPDO2 = @{ COB_ID = 0x301; TransmissionType = 254 }
    }
}

# 物件字典配置
$ObjectDictionary = @{
    Name    = "標準 CANopen 物件字典"
    Entries = @{
        # 裝置類型
        0x1000 = @{ Name = "Device Type"; DataType = "UNSIGNED32"; Value = 0x00000000; Access = "RO" }
        
        # 錯誤暫存器
        0x1001 = @{ Name = "Error Register"; DataType = "UNSIGNED8"; Value = 0x00; Access = "RO" }
        
        # 製造商狀態暫存器
        0x1002 = @{ Name = "Manufacturer Status Register"; DataType = "UNSIGNED32"; Value = 0x00000000; Access = "RO" }
        
        # 心跳生產者時間
        0x1017 = @{ Name = "Producer Heartbeat Time"; DataType = "UNSIGNED16"; Value = 1000; Access = "RW" }
        
        # 身份物件
        0x1018 = @{
            Name       = "Identity Object"
            SubEntries = @{
                0 = @{ Name = "Number of Entries"; DataType = "UNSIGNED8"; Value = 4; Access = "RO" }
                1 = @{ Name = "Vendor ID"; DataType = "UNSIGNED32"; Value = 0x12345678; Access = "RO" }
                2 = @{ Name = "Product Code"; DataType = "UNSIGNED32"; Value = 0x87654321; Access = "RO" }
                3 = @{ Name = "Revision Number"; DataType = "UNSIGNED32"; Value = 0x00010001; Access = "RO" }
                4 = @{ Name = "Serial Number"; DataType = "UNSIGNED32"; Value = 0x11223344; Access = "RO" }
            }
        }
        
        # TPDO1 通訊參數
        0x1800 = @{
            Name       = "TPDO1 Communication Parameter"
            SubEntries = @{
                0 = @{ Name = "Number of Entries"; DataType = "UNSIGNED8"; Value = 6; Access = "RO" }
                1 = @{ Name = "COB-ID"; DataType = "UNSIGNED32"; Value = 0x181; Access = "RW" }
                2 = @{ Name = "Transmission Type"; DataType = "UNSIGNED8"; Value = 1; Access = "RW" }
                3 = @{ Name = "Inhibit Time"; DataType = "UNSIGNED16"; Value = 0; Access = "RW" }
                5 = @{ Name = "Event Timer"; DataType = "UNSIGNED16"; Value = 0; Access = "RW" }
            }
        }
        
        # TPDO1 映射參數
        0x1A00 = @{
            Name       = "TPDO1 Mapping Parameter"
            SubEntries = @{
                0 = @{ Name = "Number of Entries"; DataType = "UNSIGNED8"; Value = 2; Access = "RW" }
                1 = @{ Name = "1st Mapped Object"; DataType = "UNSIGNED32"; Value = 0x60000108; Access = "RW" }
                2 = @{ Name = "2nd Mapped Object"; DataType = "UNSIGNED32"; Value = 0x60010108; Access = "RW" }
            }
        }
        
        # 應用程式特定物件
        0x6000 = @{ Name = "Read Input 8 Bit"; DataType = "UNSIGNED8"; Value = 0x00; Access = "RO" }
        0x6001 = @{ Name = "Read Input 16 Bit"; DataType = "UNSIGNED16"; Value = 0x0000; Access = "RO" }
        0x6200 = @{ Name = "Write Output 8 Bit"; DataType = "UNSIGNED8"; Value = 0x00; Access = "WO" }
        0x6201 = @{ Name = "Write Output 16 Bit"; DataType = "UNSIGNED16"; Value = 0x0000; Access = "WO" }
    }
}

# 測試場景定義
$TestScenarios = @{
    "basic-hardware"   = @{
        Name            = "基本硬體測試"
        Description     = "測試 XMC4800 CAN 硬體功能"
        Target          = "basic-test"
        Configuration   = $BasicCanTest
        ExpectedResults = @{
            MinTransmitted  = 10
            MaxErrors       = 0
            ExpectedLatency = 50  # 微秒
        }
    }
    
    "canopen-node"     = @{
        Name            = "CANopen 節點測試"
        Description     = "測試 CANopen 協議實現"
        Target          = "main"
        Configuration   = $CanopenNetwork
        ExpectedResults = @{
            BootupMessage   = $true
            HeartbeatActive = $true
            SDOResponding   = $true
            PDOTransmission = $true
        }
    }
    
    "network-monitor"  = @{
        Name            = "網路監控測試"
        Description     = "測試 CANopen 網路監控功能"
        Target          = "monitor"
        Configuration   = $CanopenNetwork
        ExpectedResults = @{
            MessageDetection   = $true
            NodeStatusTracking = $true
            ErrorReporting     = $true
        }
    }
    
    "interoperability" = @{
        Name            = "互操作性測試"
        Description     = "測試與其他 CANopen 裝置的互操作性"
        Target          = "main"
        Configuration   = $CanopenNetwork
        RequiredDevices = @("CANopen Manager", "Other CANopen Node")
        TestSequence    = @(
            "啟動節點",
            "發送 NMT 啟動命令",
            "檢查 Bootup 訊息",
            "配置 PDO",
            "測試 SDO 通訊",
            "監控心跳",
            "測試緊急訊息"
        )
    }
}

# 硬體配置
$HardwareConfig = @{
    Board        = "XMC4800 Relax Kit"
    CANInterface = "CAN_NODE0"
    CANPins      = @{
        TX = "P1.9"
        RX = "P1.8"
    }
    Oscillator   = @{
        Frequency = 12000000  # 12 MHz
        Type      = "External Crystal"
    }
    Termination  = @{
        Required   = $true
        Resistance = 120  # 歐姆
    }
}

# 除錯配置
$DebugConfig = @{
    SerialOutput = @{
        Enabled  = $true
        Baudrate = 115200
        Port     = "UART0"
        Pins     = @{
            TX = "P1.5"
            RX = "P1.4"
        }
    }
    JTAGDebugger = @{
        Enabled   = $true
        Interface = "SEGGER J-Link"
        Speed     = 4000  # kHz
    }
    LogLevel     = "DEBUG"  # DEBUG, INFO, WARNING, ERROR
    LogModules   = @("CAN", "CANopen", "Hardware", "Application")
}

# 性能基準
$PerformanceBenchmarks = @{
    CANTransmission   = @{
        MaxLatency    = 100      # 微秒
        MinThroughput = 80    # % of theoretical maximum
        MaxJitter     = 10        # 微秒
    }
    CANopenProcessing = @{
        SDOResponseTime = 5   # 毫秒
        PDOLatency      = 1       # 毫秒
        HeartbeatJitter = 1   # 毫秒
    }
    SystemResources   = @{
        MaxCPUUsage   = 50      # %
        MaxRAMUsage   = 80      # %
        MaxStackUsage = 75    # %
    }
}

# 品質保證檢查清單
$QualityChecklist = @{
    Code          = @(
        "編譯無警告",
        "靜態分析通過",
        "代碼覆蓋率 > 80%",
        "記憶體洩漏檢查",
        "堆疊溢出檢查"
    )
    Functionality = @(
        "基本 CAN 通訊",
        "CANopen 狀態機",
        "物件字典訪問",
        "PDO 傳輸",
        "SDO 通訊",
        "NMT 協議",
        "錯誤處理"
    )
    Performance   = @(
        "延遲測試",
        "吞吐量測試",
        "資源使用率",
        "長時間穩定性",
        "錯誤恢復時間"
    )
    Compliance    = @(
        "CiA 301 標準",
        "CiA 401 I/O 模組",
        "電磁相容性",
        "溫度測試",
        "振動測試"
    )
}

# 導出設定供其他腳本使用
Export-ModuleMember -Variable @(
    'BasicCanTest',
    'CanopenNetwork', 
    'ObjectDictionary',
    'TestScenarios',
    'HardwareConfig',
    'DebugConfig',
    'PerformanceBenchmarks',
    'QualityChecklist'
)