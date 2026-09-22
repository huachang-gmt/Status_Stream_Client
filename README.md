# Status Stream Client 

現在可以確認 STM32 Server 實際送出的 TCP byte stream 已經符合我們目前定義的 Transport Packet。

### 我逐包確認

| Packet | Magic   | Version | Type | Sequence |        Length | Total | CRC     |
| ------ | ------- | ------- | ---- | -------: | ------------: | ----: | ------- |
| 1      | `47 53` | `01`    | `01` |        0 | `8B 00` = 139 |   151 | `15 EE` |
| 2      | `47 53` | `01`    | `01` |        1 | `8B 00` = 139 |   151 | `13 2E` |
| 3      | `47 53` | `01`    | `01` |        2 | `8B 00` = 139 |   151 | `E9 95` |
| 4      | `47 53` | `01`    | `01` |        3 | `8B 00` = 139 |   151 | `D5 FA` |

而且每個 Payload 都是：

```text
3E                         '>'
136 bytes ASCII HEX
0D 0A                      "\r\n"
```

所以現在完整結構已經確認：

```text
47 53 01 01
│  │  │  │
│  │  │  └── STATUS
│  │  └───── Version 1
│  └──────── Magic 'S'
└─────────── Magic 'G'

00 00 00 00                 Sequence
8B 00                       Payload Length = 139

3E ... 0D 0A                139-byte Payload

15 EE                       CRC16
```

第一包尤其清楚：

```text
47 53 01 01 00 00 00 00 8B 00
```

完全符合我們的設計。

```
Offset
0       47       Magic
1       53       Magic
2       01       Version
3       01       Type
4-7     xx xx xx xx   Sequence
8-9     8B 00          Payload Length = 139

完整長度

Header 10
+
Payload Length 139
+
CRC 2
=
151
```

---

# 程式路徑 與編譯方法、執行方法
```bash
D:\RaspberryPi\Status_Stream_Client\build>cmake ..
D:\RaspberryPi\Status_Stream_Client\build>cmake --build . --config Release
D:\RaspberryPi\Status_Stream_Client\build>.\Release\Status_Stream_Client.exe
```

### 目前已確認：
```
✅ Magic 47 53
✅ TCP fragmentation
✅ Magic 跨 fragment
✅ 前置 noise
✅ Version 0x01
✅ Type 0x01
✅ 合法封包仍可正常通過
✅ 多個封包連續存在時可以正確取出
```
這是使用 檔案 ： src\packet_assembly_test.cpp 驗證的結果。

### 編譯與執行

```bash
D:\RaspberryPi\Status_Stream_Client\build>cmake ..
D:\RaspberryPi\Status_Stream_Client\build>cmake --build . --config Release
D:\RaspberryPi\Status_Stream_Client\build>.\Release\Packet_Assembly_Test.exe
```

### 輸出結果 ：

```
========================================
 Status Stream Packet Assembly Test
========================================

[TEST 1] Fragmented packet: 80 + 71
[PASS] 80 + 71 -> 151 bytes

[TEST 2] Two packets: 302 bytes
[PASS] 302 bytes -> 151 + 151

[TEST 3] Fragmented packet: 50 + 50 + 51
[PASS] 50 + 50 + 51 -> 151 bytes

[TEST 4] Noise before Magic
[PASS] Noise + packet -> 151 bytes

[TEST 5] Magic split across fragments
[PASS] Magic 47 + 53 across fragments -> 151 bytes

[TEST 6] Invalid Version
[PASS] Invalid Version rejected

[TEST 7] Invalid Type
[PASS] Invalid Type rejected

[TEST 8] Valid Version and Type
[PASS] Valid Version and Type accepted

========================================
 ALL ASSEMBLY TESTS PASSED
========================================
```

# GMT Status Stream Client

## 1. 專案說明

`Status_Stream_Client` 是 GMT 六軸控制器 Status Stream 的 Windows TCP Client 測試程式。

本專案目前主要用途為：

* 接收 STM32H755 CM4 Status Stream TCP Server 傳送的狀態封包。
* 驗證 Status Stream 封包組裝與接收流程。
* 驗證封包 Header、Payload、CRC16 與封包完整性。
* 將 136 Byte ASCII HEX Status Payload 還原成 68 Byte Raw Status Data。
* 將 68 Byte Raw Status Data 解析成 `StatusData`。
* 依照 Status Stream 文件規範進行 Controller Status、AI 與六軸位置資料解析。
* 驗證 AI Scaling 計算。
* 作為後續 Qt Status Board 圖形介面的資料來源。

目前 STM32H755 CM4 端使用五組模擬 Status Data 進行連續測試。

正式系統中，68 Byte Status Data 將由 STM32H755 CM4 上的 EtherCAT Master Controller 與六軸控制器相關程式提供。

---

# 2. 系統架構

目前 Status Stream 系統架構如下：

```text
┌──────────────────────────────┐
│ STM32H755 CM4                │
│                              │
│ EtherCAT Master Controller   │
│          │                   │
│          ▼                   │
│ 68 Byte Status Data          │
│          │                   │
│          ▼                   │
│ Status Stream TCP Server     │
│          │                   │
│          │ TCP : 8888        │
└──────────┼───────────────────┘
           │
           ▼
┌──────────────────────────────┐
│ Windows                      │
│                              │
│ Status_Stream_Client         │
│                              │
│ TcpClient                    │
│   │                          │
│   ├─ TCP Receive             │
│   ├─ Packet Assembly         │
│   ├─ Header Validation       │
│   ├─ Payload Validation      │
│   ├─ CRC Validation          │
│   ├─ HEX Decode              │
│   └─ StatusData Parse        │
│          │                   │
│          ▼                   │
│      StatusData              │
│          │                   │
│          ▼                   │
│      CLI / Qt UI             │
└──────────────────────────────┘
```

本專案的設計原則是：

> TCP 傳輸、封包組裝與資料解析集中於 `TcpClient`，上層程式只使用解析完成的 `StatusData`。

因此，未來建立 Qt Status Board 時，不應重新實作 TCP、封包組裝、CRC 或 HEX Decode。

---

# 3. Status Stream 封包格式

每一個 Status Stream TCP Packet 固定為：

```text
151 Byte
```

組成如下：

```text
┌───────────────┬───────────────┬────────┐
│ Header 10 Byte│ Payload 139 B │ CRC 2 B│
└───────────────┴───────────────┴────────┘
              Total = 151 Byte
```

## 3.1 Header

| Byte | 長度 | 說明                                  |
| ---- | -: | ----------------------------------- |
| 0    |  1 | Magic `0x47` (`G`)                  |
| 1    |  1 | Magic `0x53` (`S`)                  |
| 2    |  1 | Version `0x01`                      |
| 3    |  1 | Type `0x01`，Status                  |
| 4～7  |  4 | Sequence，UINT32 Little-Endian       |
| 8～9  |  2 | Payload Length，UINT16 Little-Endian |

目前 Payload Length：

```text
139 Byte
```

---

# 4. Payload 格式

Payload 固定為：

```text
> + 136 ASCII HEX characters + CRLF
```

因此：

```text
1 + 136 + 2 = 139 Byte
```

格式：

```text
┌────┬────────────────────────────────────────────┬──────┐
│  > │              136 HEX Characters            │ CRLF │
└────┴────────────────────────────────────────────┴──────┘
  1                    136                         2
```

136 個 ASCII HEX 字元代表：

```text
136 / 2 = 68 Byte
```

因此 Payload 的實際 Status Data 為：

```text
68 Byte Raw Status Data
```

---

# 5. 68 Byte Raw Status Data

68 Byte Status Data 定義如下：

```text
Byte 0～3
    Controller Status
    UINT32

Byte 4～19
    AI00～AI07
    8 × UINT16
    共 16 Byte

Byte 20～67
    X
    Y
    Z
    RX
    RY
    RZ

    6 × IEEE-754 Double
    共 48 Byte
```

總長度：

```text
4 + 16 + 48 = 68 Byte
```

---

# 6. Controller Status

Controller Status 為 UINT32。

目前定義：

| Bit | 名稱        | 意義                         |
| --: | --------- | -------------------------- |
|   0 | CONNECT   | EtherCAT / Controller 連線狀態 |
|   1 | VOLTAGEON | 電源狀態                       |
|   3 | ISMOVING  | Motion 狀態                  |
|   4 | ISFA      | FA 狀態                      |
|   5 | HOMINGEND | Homing 完成狀態                |
|   6 | ERROR     | Error 狀態                   |

Client 會將 UINT32 解析成：

```cpp
bool connect;
bool voltage_on;
bool is_moving;
bool is_fa;
bool homing_end;
bool error;
```

---

# 7. AI 資料

AI00～AI07 為 UINT16 Raw Value。

目前文件定義：

| Channel | 名稱        | Scaling                 |
| ------- | --------- | ----------------------- |
| AI00    | Load Cell | `Raw / 65535 × 20 - 10` |
| AI01    | Reserved  | 未定義                     |
| AI02    | I2C0      | 未定義                     |
| AI03    | I2C3      | 未定義                     |
| AI04    | PM Ch0    | `Raw / 65535 × 10`      |
| AI05    | PM Ch1    | `Raw / 65535 × 10`      |
| AI06    | PM Ch2    | `Raw / 65535 × 10`      |
| AI07    | PM Ch3    | `Raw / 65535 × 10`      |

Client 同時保存：

```text
Raw Value
```

以及已定義 Scaling 的：

```text
Voltage Value
```

因此 `StatusData` 中：

```cpp
uint16_t ai[8];
double ai_voltage[8];
```

AI01～AI03 的 Scaling 尚未由文件定義，因此 Client 不自行推測其實際物理電壓。

---

# 8. 六軸位置資料

Status Stream 的 68 Byte Status Data 已經包含：

```text
X
Y
Z
RX
RY
RZ
```

每一項為 IEEE-754 64-bit Double。

傳輸資料採：

```text
MSB First / Big-Endian
```

Client 會依照 64-bit IEEE-754 bit pattern 還原成 `double`。

注意：

本 Client 不負責從 12 軸 Encoder Position 自行計算 X/Y/Z/RX/RY/RZ。

12 軸 Encoder Position 的轉換屬於 STM32H755 EtherCAT / Motion Controller 上游處理。

Client 的責任是：

```text
接收 → 還原 → 解析 → 提供 StatusData
```

---

# 9. CRC16

CRC 使用：

```text
Initial Value = 0xFFFF
Polynomial    = 0xA001
```

CRC 計算範圍：

```text
Header + Payload
```

也就是：

```text
10 + 139 = 149 Byte
```

CRC 本身為：

```text
2 Byte
```

傳輸順序為：

```text
UINT16 Little-Endian
```

因此完整封包：

```text
149 Byte Header + Payload
+ 2 Byte CRC
= 151 Byte
```

---

# 10. StatusData

Client 最終提供給上層程式的資料結構為：

```cpp
struct StatusData
{
    uint32_t controller_status;

    bool connect;
    bool voltage_on;
    bool is_moving;
    bool is_fa;
    bool homing_end;
    bool error;

    uint16_t ai[8];
    double ai_voltage[8];

    double x;
    double y;
    double z;
    double rx;
    double ry;
    double rz;
};
```

未來 Qt Status Board 應以 `StatusData` 作為資料來源。

Qt UI 不應重新解析 TCP Packet。

---

# 11. 設計方法

本專案採用分層方式設計。

## 第一層：TCP Transport

負責：

* 建立 TCP Connection
* 接收 TCP Byte Stream
* 判斷 Connection 狀態

主要類別：

```text
TcpClient
```

---

## 第二層：Packet Assembly

負責處理 TCP Stream 的實際特性。

TCP 不保證一次 `Receive()` 就得到一個完整 Packet。

因此必須支援：

```text
完整封包
分段封包
多封包
Magic 被分割
雜訊資料
不完整資料
```

---

## 第三層：Packet Validation

依序驗證：

```text
Magic
Version
Type
Sequence
Payload Length
Payload Format
CRC
```

---

## 第四層：Payload Decode

將：

```text
139 Byte Payload
```

轉換成：

```text
68 Byte Raw Status Data
```

---

## 第五層：StatusData Parsing

將 68 Byte Raw Data 解析成：

```text
StatusData
```

包含：

* Controller Status
* Controller Flags
* AI Raw Values
* AI Scaling
* X/Y/Z
* RX/RY/RZ

---

# 12. API 設計原則

目前 Client 的核心處理應保持集中於 `TcpClient`。

上層程式不需要知道：

* CRC 演算法
* HEX Decode 細節
* TCP Fragmentation
* Magic 搜尋
* Payload Validation
* Endian Conversion

上層程式只需要：

```text
取得完整 Status Packet
        ↓
Decode
        ↓
Parse
        ↓
使用 StatusData
```

此設計可以讓未來 CLI 與 Qt GUI 共用相同的資料處理核心。

---

# 13. STM32 Server 測試資料

目前 STM32H755 CM4 Server 使用五組模擬 Status Data：

```text
Sample 0
Sample 1
Sample 2
Sample 3
Sample 4
```

依序循環：

```text
0 → 1 → 2 → 3 → 4 → 0 → ...
```

目前實驗傳送週期：

```text
1000 ms
```

正式 Status Stream 規格目標：

```text
200 ms
```

目前維持 1000 ms 是為了方便 CLI 觀察資料變化。

---

# 14. 五組模擬資料

## Sample 0

```text
Controller Status = 0x00000003

AI00 = -5V
AI04 = 3V
AI05 = 4V
AI06 = 5V
AI07 = 6V

X  = 0
Y  = 0
Z  = 0
RX = 0
RY = 0
RZ = 0
```

## Sample 1

```text
Controller Status = 0x0000000B

AI00 = -2.5V
AI04 = 4V
AI05 = 5V
AI06 = 6V
AI07 = 7V

X  = 10
Y  = 20
Z  = 30
RX = 1
RY = 2
RZ = 3
```

## Sample 2

```text
Controller Status = 0x00000013

AI00 = 0V
AI04 = 5V
AI05 = 6V
AI06 = 7V
AI07 = 8V

X  = 20
Y  = 10
Z  = 5
RX = 2
RY = 4
RZ = 6
```

## Sample 3

```text
Controller Status = 0x0000002B

AI00 = 5V
AI04 = 6V
AI05 = 7V
AI06 = 8V
AI07 = 9V

X  = 30
Y  = 15
Z  = 10
RX = 3
RY = 6
RZ = 9
```

## Sample 4

```text
Controller Status = 0x00000023

AI00 = 8V
AI04 = 7V
AI05 = 8V
AI06 = 9V
AI07 = 10V

X  = 40
Y  = 20
Z  = 15
RX = 4
RY = 8
RZ = 12
```

---

# 15. 文件規範要求

本專案後續開發遵循以下規範。

## 15.1 不任意修改已驗證功能

已經通過測試的：

* Packet Assembly
* Header Validation
* Payload Validation
* CRC
* HEX Decode
* StatusData Parse
* Controller Status
* AI Scaling

不得因為新增功能而任意重寫。

---

## 15.2 優先新增，不任意刪除

若新功能可以透過新增函式或新增模組完成，優先採用新增方式。

除非確認舊程式碼不再需要，否則不直接刪除已驗證程式。

---

## 15.3 一次只修改一個小範圍

每次修改應：

1. 指定完整檔案路徑。
2. 指定函式或區域。
3. 說明 Old → New。
4. 說明修改原因。
5. 編譯。
6. 測試。
7. 確認 PASS 後再進行下一步。

---

## 15.4 測試程式與正式程式分離

測試新功能時，可以建立獨立測試檔案或測試 Target。

目的：

```text
避免測試程式破壞正式 main.cpp
```

測試成功後，功能應整合回正式架構。

---

## 15.5 不在上層重複實作核心功能

例如 Qt GUI 不應重新實作：

```text
TCP Receive
Packet Assembly
CRC
HEX Decode
StatusData Parse
```

這些功能由 Client Core 負責。

---

# 16. 目前開發狀態

目前已完成：

```text
[PASS] TCP Connection
[PASS] 151 Byte Packet Receive
[PASS] Packet Assembly
[PASS] Header Validation
[PASS] Payload Validation
[PASS] CRC Validation
[PASS] HEX Decode
[PASS] StatusData Parsing
[PASS] Controller Status Parsing
[PASS] AI Scaling
[PASS] AI Raw/Scaled Integration
[PASS] STM32 → Windows End-to-End Test
```

測試項目：

```text
TEST 1 ～ TEST 31
```

全部通過。

目前已完成第一個完整 Status Stream Client 驗證階段。

---

# 17. 後續開發方向

下一階段預計：

```text
目前 CLI
   │
   ▼
StatusData
   │
   ▼
Qt Status Board
```

Qt GUI 的責任為：

* Controller Status 顯示
* Controller Flags 顯示
* AI Raw Value 顯示
* AI Voltage 顯示
* X/Y/Z 顯示
* RX/RY/RZ 顯示
* Status Stream 即時更新

TCP 與 StatusData Parsing 維持使用目前已驗證的 Client Core。

---

# 18. 專案測試文件

完整測試項目與每項測試目的請參閱：

```text
TEST_SPEC.md
```

此文件記錄 TEST 1～TEST 31 的測試目的與驗證範圍。

---

# 19. 專案目前基準

本版本建立的基準為：

```text
Status Stream Client
TCP + Packet + CRC + HEX Decode + StatusData
完整驗證版本
```

此版本作為後續 Qt Status Board 開發的基準點。

任何後續修改應避免破壞本版本已通過的測試。

---
