import base64
import re
import subprocess
import gspread
from oauth2client.service_account import ServiceAccountCredentials
from dotenv import load_dotenv
import os

# 載入 .env 檔案
load_dotenv()

SES_BIN_PATH = os.getenv("SES_BIN_PATH")
PROJECT_PATH = os.getenv("PROJECT_PATH")
HEX_PATH = os.getenv("HEX_PATH")
main_c_filename = os.getenv("MAIN_C_FILENAME")

# Google Sheets API 設定
scope = ["https://spreadsheets.google.com/feeds", "https://www.googleapis.com/auth/drive"]
creds = ServiceAccountCredentials.from_json_keyfile_name(os.getenv('CREDENTIALS_PATH'), scope)
client = gspread.authorize(creds)

# 打開 Google Sheets
sheet = client.open("BeaconList").sheet1

# 讀取 Google Sheets 中的資料
rows = sheet.get_all_records()

for row in rows:
    base64_key = row['adv_b64']
    device_id_str = str(row['ID']).zfill(4)
    programed = row.get('programed', '')

    if programed.lower() == 'y':
        print(f"🔄 ID： {device_id_str} 已完成，跳過...")
        continue

    print(f"🔄 正在處理ID： {device_id_str} ...")
    # Base64 解碼
    binary_key = base64.b64decode(base64_key)
    if len(binary_key) != 28:
        print(f"警告：解碼後的長度為 {len(binary_key)}，但預期為 28")

    # 轉成 C 語言的陣列格式
    public_key_array = ", ".join(f"0x{b:02X}" for b in binary_key)

    # 將 Device ID 轉成十六進制值
    if len(device_id_str) != 4:
        print("警告：ID 長度不是 4")
    device_id_values = ", ".join(f"0x{int(device_id_str[i:i+2], 16):02X}" for i in range(0, len(device_id_str), 2))

    # 讀取 main.c 內容
    with open(main_c_filename, 'r', encoding='utf-8') as f:
        content = f.read()

    # 使用正則表達式替換 public_key 陣列
    content = re.sub(
        r'static char public_key\[28\] = \{.*?\};',
        f'static char public_key[28] = {{ {public_key_array} }};',
        content,
        flags=re.DOTALL
    )

    # 替換 deviceID 陣列
    content = re.sub(
        r'static char deviceID\[2\] = \{.*?\};',
        f'static char deviceID[2] = {{ {device_id_values} }};',
        content
    )

    # 將修改後的內容寫回 main.c
    with open(main_c_filename, 'w', encoding='utf-8') as f:
        f.write(content)

    print("✅ main.c 已更新！")
    # 1️⃣ 執行 SES 編譯
    print("⚙️ 開始編譯...")
    compile_result = subprocess.run([SES_BIN_PATH, PROJECT_PATH, "-config", "Release"], shell=True)
    if compile_result.returncode != 0:
        print("❌ 編譯失敗！")
        exit(1)

    print("✅ 編譯完成")

    # 2️⃣ 先清空 nRF52811
    print("🗑️ 清除 nRF52811 Flash...")
    subprocess.run(["nrfjprog", "--eraseall"], shell=True)

    # 3️⃣ 燒錄 SoftDevice
    print("🔥 燒錄 SoftDevice...")
    subprocess.run(["nrfjprog", "--program", "s112_nrf52_7.2.0_softdevice.hex", "--chiperase"], shell=True)

    # 4️⃣ 燒錄 App
    print("🚀 燒錄應用程式...")
    flash_result = subprocess.run(["nrfjprog", "--program", HEX_PATH, "--verify"], shell=True)
    if flash_result.returncode != 0:
        print("❌ 燒錄失敗！")
        exit(1)

    # 5️⃣ 執行
    print("🔄 啟動 nRF52811")
    subprocess.run(["nrfjprog", "--reset"], shell=True)

    print("✅ 完成！")

    # 更新 Google Sheets 中的 programed 欄位
    cell = sheet.find(device_id_str)
    sheet.update_cell(cell.row, cell.col + 6, 'y')  # 假設 programed 欄位在第 7 列

    input("按下 Enter 鍵以繼續下一輪...")