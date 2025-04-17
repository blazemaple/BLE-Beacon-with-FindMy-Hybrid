# 專案說明

此專案包含多個 Python 腳本，以下是每個腳本的用途說明：

## autoProgram.py
此腳本用於自動化處理 nRF52811 的編譯、燒錄與啟動流程。它會從 Google Sheets 中讀取設備資料，更新 `main.c` 中的公鑰與設備 ID，並執行編譯與燒錄操作。

## generate_keys.py
此腳本用於生成一組隨機的私鑰與公鑰，並將其以 Base64 格式儲存到 CSV 檔案中。這些密鑰可用於設備的身份驗證。

## gsheet_ver.py
此腳本用於與 Google Sheets 互動，生成設備的密鑰與地址，並將這些資料新增到 Google Sheets 中。它還會計算公鑰的 SHA-256 雜湊值。

## key2mac.py
此腳本用於將 Base64 編碼的公鑰轉換為設備的 MAC 地址。它會解碼公鑰並生成符合規範的 MAC 地址。

## credentials.json
此檔案包含 Google API 的服務帳戶憑證，用於授權腳本訪問 Google Sheets。

## .env
此檔案用於儲存專案中使用的環境變數，包括各種路徑設定與憑證檔案位置，方便集中管理與修改。

### 範例內容：
```
SES_BIN_PATH=C:\Program Files\SEGGER\SEGGER Embedded Studio for ARM 5.68\bin\emBuild.exe
PROJECT_PATH=C:\Users\ucl\lab\nRF5_SDK_17.1.0_ddde560\examples\ble_peripheral\ble_app_beacon\pca10056e\s112\ses\ble_app_beacon_pca10056e_s112.emProject
HEX_PATH=C:\Users\ucl\lab\nRF5_SDK_17.1.0_ddde560\examples\ble_peripheral\ble_app_beacon\pca10056e\s112\ses\Output\Release\Exe\ble_app_beacon_pca10056e_s112.hex
MAIN_C_FILENAME=C:\Users\ucl\lab\nRF5_SDK_17.1.0_ddde560\examples\ble_peripheral\ble_app_beacon\main.c
CREDENTIALS_PATH=C:\Users\ucl\lab\nRF5_SDK_17.1.0_ddde560\examples\ble_peripheral\ble_app_beacon\pca10056e\s112\ses\Output\Release\Exe\credentials.json
```