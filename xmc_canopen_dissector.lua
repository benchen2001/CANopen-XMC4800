--[[
    XMC4800 CANopen Monitor Protocol Dissector for Wireshark
    專業級 CANopen 監控協議解析器
    
    安裝方法:
    1. 將此檔案複製到 Wireshark 的 plugins 目錄
    2. 重新啟動 Wireshark
    3. 在 Capture 中設定 DLT_USER0 或使用 UDP 捕獲
    
    使用方法:
    - 啟動 XMC4800 監控程式
    - 使用 canopen_monitor.py 捕獲並輸出到檔案
    - 在 Wireshark 中開啟捕獲檔案
--]]

-- 建立協議解析器
local xmc_canopen_proto = Proto("xmc_canopen", "XMC4800 CANopen Monitor Protocol")

-- 定義欄位
local f_magic = ProtoField.uint32("xmc_canopen.magic", "Magic Number", base.HEX)
local f_version = ProtoField.uint16("xmc_canopen.version", "Protocol Version", base.DEC)
local f_frame_count = ProtoField.uint16("xmc_canopen.frame_count", "Frame Count", base.DEC)
local f_checksum = ProtoField.uint32("xmc_canopen.checksum", "CRC32 Checksum", base.HEX)

-- CANopen 訊息欄位
local f_timestamp = ProtoField.uint32("xmc_canopen.frame.timestamp", "Timestamp (μs)", base.DEC)
local f_can_id = ProtoField.uint32("xmc_canopen.frame.can_id", "CAN ID", base.HEX)
local f_dlc = ProtoField.uint8("xmc_canopen.frame.dlc", "Data Length Code", base.DEC)
local f_data = ProtoField.bytes("xmc_canopen.frame.data", "CAN Data")
local f_flags = ProtoField.uint8("xmc_canopen.frame.flags", "Flags", base.HEX)
local f_reserved = ProtoField.uint8("xmc_canopen.frame.reserved", "Reserved", base.HEX)

-- CANopen 解析欄位
local f_msg_type = ProtoField.string("xmc_canopen.frame.msg_type", "Message Type")
local f_node_id = ProtoField.uint8("xmc_canopen.frame.node_id", "Node ID", base.DEC)
local f_function_code = ProtoField.uint16("xmc_canopen.frame.function_code", "Function Code", base.HEX)

-- 將欄位加入協議
xmc_canopen_proto.fields = {
    f_magic, f_version, f_frame_count, f_checksum,
    f_timestamp, f_can_id, f_dlc, f_data, f_flags, f_reserved,
    f_msg_type, f_node_id, f_function_code
}

-- CANopen 訊息類型映射
local canopen_msg_types = {
    [0x000] = "NMT Control",
    [0x080] = "SYNC",
    [0x100] = "TIME Stamp"
}

-- CANopen 訊息類型範圍
local function get_canopen_msg_type(can_id)
    if can_id == 0x000 then return "NMT_CTRL" end
    if can_id == 0x080 then return "SYNC" end
    if can_id == 0x100 then return "TIME" end
    
    if can_id >= 0x081 and can_id <= 0x0FF then return "EMERGENCY" end
    if can_id >= 0x180 and can_id <= 0x1FF then return "PDO1_TX" end
    if can_id >= 0x200 and can_id <= 0x27F then return "PDO1_RX" end
    if can_id >= 0x280 and can_id <= 0x2FF then return "PDO2_TX" end
    if can_id >= 0x300 and can_id <= 0x37F then return "PDO2_RX" end
    if can_id >= 0x380 and can_id <= 0x3FF then return "PDO3_TX" end
    if can_id >= 0x400 and can_id <= 0x47F then return "PDO3_RX" end
    if can_id >= 0x480 and can_id <= 0x4FF then return "PDO4_TX" end
    if can_id >= 0x500 and can_id <= 0x57F then return "PDO4_RX" end
    if can_id >= 0x580 and can_id <= 0x5FF then return "SDO_TX" end
    if can_id >= 0x600 and can_id <= 0x67F then return "SDO_RX" end
    if can_id >= 0x700 and can_id <= 0x77F then return "HEARTBEAT" end
    
    return "UNKNOWN"
end

-- 取得節點 ID
local function get_node_id(can_id)
    if can_id == 0x000 or can_id == 0x080 or can_id == 0x100 then
        return 0  -- 廣播訊息
    end
    return bit.band(can_id, 0x7F)  -- 取低 7 位
end

-- 協議解析主函數
function xmc_canopen_proto.dissector(buffer, pinfo, tree)
    local length = buffer:len()
    
    -- 檢查最小長度
    if length < 8 then
        return 0
    end
    
    -- 讀取魔術字元
    local magic = buffer(0, 4):le_uint()
    if magic ~= 0x43414E4F then  -- "CANO"
        return 0
    end
    
    -- 設定協議資訊
    pinfo.cols.protocol = xmc_canopen_proto.name
    pinfo.cols.info = "XMC4800 CANopen Monitor"
    
    -- 建立協議樹
    local subtree = tree:add(xmc_canopen_proto, buffer(), "XMC4800 CANopen Monitor Protocol")
    
    -- 解析標頭
    subtree:add_le(f_magic, buffer(0, 4))
    local version = buffer(4, 2):le_uint()
    subtree:add_le(f_version, buffer(4, 2))
    local frame_count = buffer(6, 2):le_uint()
    subtree:add_le(f_frame_count, buffer(6, 2))
    
    -- 更新資訊欄
    pinfo.cols.info = string.format("CANopen Monitor: %d frames (v%d)", frame_count, version)
    
    -- 解析 CAN 訊息
    local offset = 8
    for i = 0, frame_count - 1 do
        if offset + 16 > length then
            break
        end
        
        -- 建立訊息子樹
        local frame_tree = subtree:add(xmc_canopen_proto, buffer(offset, 16), 
                                     string.format("CANopen Frame %d", i + 1))
        
        -- 解析訊息內容
        local timestamp = buffer(offset, 4):le_uint()
        local can_id = buffer(offset + 4, 4):le_uint()
        local dlc = buffer(offset + 8, 1):uint()
        local data = buffer(offset + 9, 8)
        local flags = buffer(offset + 17, 1):uint()
        
        -- 添加基本欄位
        frame_tree:add_le(f_timestamp, buffer(offset, 4))
        frame_tree:add_le(f_can_id, buffer(offset + 4, 4))
        frame_tree:add(f_dlc, buffer(offset + 8, 1))
        frame_tree:add(f_data, buffer(offset + 9, dlc))
        frame_tree:add(f_flags, buffer(offset + 17, 1))
        frame_tree:add(f_reserved, buffer(offset + 18, 1))
        
        -- CANopen 協議解析
        local msg_type = get_canopen_msg_type(can_id)
        local node_id = get_node_id(can_id)
        local function_code = bit.rshift(can_id, 7)
        
        frame_tree:add(f_msg_type, buffer(offset + 4, 4), msg_type)
        frame_tree:add(f_node_id, buffer(offset + 4, 4), node_id)
        frame_tree:add(f_function_code, buffer(offset + 4, 4), function_code)
        
        -- 設定訊息摘要
        frame_tree:set_text(string.format("Frame %d: ID=0x%03X %s Node=%d DLC=%d [%s]",
                                        i + 1, can_id, msg_type, node_id, dlc,
                                        tostring(buffer(offset + 9, dlc))))
        
        offset = offset + 16
    end
    
    -- 解析校驗碼
    if offset + 4 <= length then
        subtree:add_le(f_checksum, buffer(offset, 4))
    end
    
    return length
end

-- 註冊解析器
local udp_port = DissectorTable.get("udp.port")
udp_port:add(8888, xmc_canopen_proto)  -- 假設使用 UDP 8888 端口

-- 也可以註冊到其他協議表
local tcp_port = DissectorTable.get("tcp.port")
tcp_port:add(8888, xmc_canopen_proto)

-- 註冊為 DLT_USER 解析器
DissectorTable.get("wtap_encap"):add(wtap.USER0, xmc_canopen_proto)

-- 輸出載入訊息
print("XMC4800 CANopen Monitor Protocol Dissector loaded successfully")