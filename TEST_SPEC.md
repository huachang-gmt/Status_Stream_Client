# GMT Status Stream Client

# 測試規格與測試項目

## 1. 測試目的

本文件記錄 `Status_Stream_Client` 的測試項目與測試目的。

測試主要驗證：

```text
TCP Stream
    ↓
Packet Assembly
    ↓
Header Validation
    ↓
Payload Validation
    ↓
CRC Validation
    ↓
HEX Decode
    ↓
StatusData Parsing
    ↓
AI Scaling
    ↓
End-to-End Status Stream
```

目前共完成：

```text
TEST 1 ～ TEST 31
```

全部通過。

---

# 2. 測試原則

每項測試應針對單一功能或明確功能範圍。

測試目的不是單純確認程式可以執行，而是確認：

```text
輸入條件
    ↓
指定處理
    ↓
預期結果
```

完全符合 Status Stream 設計規範。

測試程式目前集中於：

```text
src/packet_assembly_test.cpp
```

---

# 3. TEST 1 ～ TEST 5

# Packet Assembly 基本測試

## TEST 1：完整封包

### 測試目的

確認收到完整 151 Byte Status Packet 時，可以正確組成一個完整封包。

### 驗證內容

```text
151 Byte
↓
Complete Packet
```

---

## TEST 2：分段封包

### 測試目的

模擬 TCP Stream 將一個封包分成多次 Receive。

確認 Packet Assembly 可以累積資料，直到收到完整 151 Byte。

### 驗證內容

```text
Partial Data
+
Partial Data
+
Partial Data
↓
Complete Packet
```

---

## TEST 3：多封包

### 測試目的

確認一次 TCP Receive 中包含多個 Status Packet 時，可以正確分離每一個 Packet。

---

## TEST 4：Magic Split

### 測試目的

確認 Magic Bytes：

```text
47 53
```

如果被 TCP Fragmentation 分割，也可以正確組裝。

例如：

```text
Receive 1:
47

Receive 2:
53 ...
```

仍然可以找到完整封包。

---

## TEST 5：Noise Data

### 測試目的

確認封包前存在非 Status Packet 的資料時，可以搜尋有效 Magic 並找到正確封包。

---

# 4. TEST 6 ～ TEST 11

# Header Validation

## TEST 6：Magic Validation

### 測試目的

確認錯誤 Magic 不會被當成有效 Status Packet。

有效：

```text
47 53
```

---

## TEST 7：Version Validation

### 測試目的

確認 Version 不等於目前支援版本 `0x01` 時，封包會被拒絕。

---

## TEST 8：Type Validation

### 測試目的

確認非 Status Type 的封包不會被當成 Status Packet。

目前 Status Type：

```text
0x01
```

---

## TEST 9：Payload Length Validation

### 測試目的

確認 Payload Length 不等於：

```text
139
```

時，封包不會被接受。

---

## TEST 10：Sequence Validation

### 測試目的

確認封包 Sequence 欄位可以正確解析。

Sequence 為：

```text
UINT32 Little-Endian
```

此測試主要驗證 Sequence 欄位的格式與解析。

---

## TEST 11：Header 完整驗證

### 測試目的

確認 Header 各欄位同時符合規範時，才可以進入後續 Payload / CRC 處理。

---

# 5. TEST 12 ～ TEST 16

# CRC Validation

## TEST 12：正確 CRC

### 測試目的

確認合法封包的 CRC 可以通過驗證。

---

## TEST 13：CRC 錯誤

### 測試目的

修改封包資料，使 CRC 不再符合。

確認封包會被拒絕。

---

## TEST 14：Header 修改後 CRC 錯誤

### 測試目的

確認即使只是修改 Header，也會造成 CRC 驗證失敗。

---

## TEST 15：Payload 修改後 CRC 錯誤

### 測試目的

確認修改 Payload 後，CRC 驗證會失敗。

---

## TEST 16：CRC Byte Order

### 測試目的

確認 CRC 16-bit value 的傳輸順序符合：

```text
Little-Endian
```

---

# 6. TEST 17 ～ TEST 24

# Payload Validation

## TEST 17：Payload Magic

### 測試目的

確認 Payload 必須以：

```text
>
```

開始。

---

## TEST 18：Payload HEX 長度

### 測試目的

確認 Payload 中 HEX 部分必須為：

```text
136 characters
```

---

## TEST 19：Payload HEX 字元

### 測試目的

確認 HEX 區域只能包含合法：

```text
0～9
A～F
a～f
```

HEX 字元。

---

## TEST 20：Payload CRLF

### 測試目的

確認 Payload 結尾必須為：

```text
0D 0A
```

也就是：

```text
\r\n
```

---

## TEST 21：Payload 完整格式

### 測試目的

確認：

```text
>
+
136 HEX
+
\r\n
```

三個部分必須同時符合規範。

---

## TEST 22：Payload 長度錯誤

### 測試目的

確認 Payload 長度錯誤時，不會進行 Status Data Decode。

---

## TEST 23：Invalid HEX

### 測試目的

確認 Payload 中包含非法 HEX 字元時，Decode 會失敗。

---

## TEST 24：Invalid HEX through GetStatusPacket

### 測試目的

確認透過完整：

```text
GetStatusPacket()
```

流程處理時，Invalid HEX 封包仍然會被拒絕。

此測試確認 Payload Validation 有正確整合進 Packet Processing 流程。

---

# 7. TEST 25

# HEX Decode

## TEST 25：136 HEX → 68 Byte

### 測試目的

確認：

```text
136 ASCII HEX
```

可以正確還原成：

```text
68 Byte Raw Status Data
```

### 驗證內容

```text
136 / 2 = 68 Byte
```

並確認每一個 Byte 的值都正確。

---

# 8. TEST 26

# StatusData Parsing

## TEST 26：68 Byte Raw Status → StatusData

### 測試目的

確認 68 Byte Raw Status Data 可以正確解析成 `StatusData`。

驗證：

```text
Controller Status
AI00～AI07
X
Y
Z
RX
RY
RZ
```

---

# 9. TEST 27

# Complete Packet → StatusData

## TEST 27：完整 151 Byte Packet

### 測試目的

確認完整流程：

```text
151 Byte Packet
↓
Payload
↓
HEX Decode
↓
68 Byte Raw Data
↓
StatusData
```

可以完整執行。

此測試將前面的 Packet、Payload 與 StatusData 處理串接起來。

---

# 10. TEST 28

# Controller Status

## TEST 28：Controller Status Bit Parsing

### 測試目的

確認 Controller Status UINT32 可以正確解析成各個狀態 Bit。

驗證：

```text
bit0 CONNECT
bit1 VOLTAGEON
bit3 ISMOVING
bit4 ISFA
bit5 HOMINGEND
bit6 ERROR
```

並確認每個 Bit 的值可以正確反映到 `StatusData`。

---

# 11. TEST 29

# AI Scaling 基本測試

## TEST 29：AI Scaling Formula

### 測試目的

確認文件定義的 AI Scaling 公式正確實作。

### AI00

公式：

```text
V = Raw / 65535 × 20 - 10
```

驗證：

```text
Raw = 0
→ -10V

Raw = 65535
→ +10V
```

### AI04～AI07

公式：

```text
V = Raw / 65535 × 10
```

驗證：

```text
Raw = 0
→ 0V

Raw = 65535
→ 10V
```

### AI01～AI03

文件未定義 Scaling。

因此測試確認程式不自行推測其物理電壓。

---

# 12. TEST 30

# AI Scaling 中間值

## TEST 30：AI Scaling Intermediate Values

### 測試目的

確認 Scaling 不只是端點正確，中間值也可以正確計算。

測試：

```text
AI00
Raw = 32767
Raw = 32768
```

以及：

```text
AI04
Raw = 32767
Raw = 32768
```

### 特別驗證

確認程式使用 Floating-Point 計算：

```text
double
```

避免發生整數除法造成的精度錯誤。

---

# 13. TEST 31

# AI Raw + Scaled Integration

## TEST 31：AI Raw and Scaled Integration

### 測試目的

確認完整 `StatusData` Parsing 過程中：

```text
Raw UINT16
```

與：

```text
Scaled Voltage
```

可以同時正確保存。

### 驗證內容

確認：

```cpp
status_data.ai[8]
```

保留原始 UINT16。

同時：

```cpp
status_data.ai_voltage[8]
```

保存已經計算的 Scaling 結果。

並確認：

```text
AI01～AI03
```

仍維持未定義 Scaling 的處理方式。

---

# 14. End-to-End 實機測試

除了 TEST 1～31 的單元 / 組合測試之外，目前也已完成 STM32H755 CM4 與 Windows Client 的實際 TCP 串接。

## 測試架構

```text
STM32H755 CM4
192.168.137.10:8888
        │
        │ TCP
        ▼
Windows Status Stream Client
```

---

## 測試條件

STM32 Server：

```text
Status Sample Count = 5
傳送週期 = 1000 ms
```

Sample 順序：

```text
0 → 1 → 2 → 3 → 4 → 0 ...
```

---

## 驗證項目

確認 Windows Client 可以實際收到：

```text
151 Byte Status Packet
```

並正確顯示：

```text
Controller Status
Controller Flags
AI Raw
AI Scaled Voltage
X
Y
Z
RX
RY
RZ
```

---

# 15. End-to-End 測試結果

目前五組 STM32 模擬資料均已正確解析。

## Sample 0

```text
Controller Status = 0x00000003

X=0
Y=0
Z=0
RX=0
RY=0
RZ=0
```

## Sample 1

```text
Controller Status = 0x0000000B

X=10
Y=20
Z=30
RX=1
RY=2
RZ=3
```

## Sample 2

```text
Controller Status = 0x00000013

X=20
Y=10
Z=5
RX=2
RY=4
RZ=6
```

## Sample 3

```text
Controller Status = 0x0000002B

X=30
Y=15
Z=10
RX=3
RY=6
RZ=9
```

## Sample 4

```text
Controller Status = 0x00000023

X=40
Y=20
Z=15
RX=4
RY=8
RZ=12
```

五組資料均可正常循環接收與解析。

---

# 16. 測試結果總結

目前：

```text
TEST 1  ～ TEST 31
全部 PASS
```

並且完成：

```text
STM32H755
    ↓
TCP
    ↓
Windows Client
    ↓
151 Byte Packet
    ↓
68 Byte Raw Status
    ↓
StatusData
```

的實機 End-to-End 驗證。

此結果作為目前 GitHub checkpoint 的測試基準。

---

# 17. 後續測試

後續建立 Qt Status Board 後，測試範圍將增加：

```text
StatusData → Qt UI
```

包括：

* Controller Status 顯示
* Controller Flags 顯示
* AI Raw 顯示
* AI Voltage 顯示
* X/Y/Z 顯示
* RX/RY/RZ 顯示
* Status 更新週期
* TCP Disconnect / Reconnect
* 長時間連續接收

正式 Status Stream 週期預計由目前：

```text
1000 ms
```

調整為：

```text
200 ms
```

相關測試將於後續階段進行。

---

# 18. 測試基準原則

任何後續修改不得無理由破壞目前：

```text
TEST 1～TEST 31
```

若新增功能造成既有測試失敗，應先確認修改是否真的需要改變既有行為。

新的測試應在不破壞既有測試的前提下加入。

---

# 19. 目前測試基準版本

本文件所記錄的 TEST 1～TEST 31 為目前 Status Stream Client 的第一階段測試基準。

後續功能：

```text
Qt Status Board
200 ms Status Stream
實際 EtherCAT Status Data
長時間運行測試
```

應建立在此測試基準之上。

這個 GitHub checkpoint 就不只是「程式碼可以跑」，而是把**規格 → 架構 → 設計原則 → 測試 → End-to-End 結果**一起固定下來。

