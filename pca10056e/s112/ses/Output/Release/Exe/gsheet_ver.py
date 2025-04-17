#!/usr/bin/env python3
import sys, base64, hashlib, random
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.backends import default_backend
import gspread
from oauth2client.service_account import ServiceAccountCredentials

def sha256(data):
    digest = hashlib.new("sha256")
    digest.update(data)
    return digest.digest()

def get_last_id(sheet):
    rows = sheet.get_all_records()
    if rows:
        last_id = rows[-1]['ID']
        print(last_id)
        return int(str(last_id), 16)
    else:
        return 0

def set_addr_from_key(base64_key):
    # Base64 解碼
    binary_key = base64.b64decode(base64_key)
    if len(binary_key) != 28:
        print(f"警告：解碼後的長度為 {len(binary_key)}，但預期為 28")
        return None

    # 設定 addr
    addr = [0] * 6
    addr[0] = binary_key[0] | 0b11000000
    addr[1] = binary_key[1]
    addr[2] = binary_key[2]
    addr[3] = binary_key[3]
    addr[4] = binary_key[4]
    addr[5] = binary_key[5]

    return addr

# Google Sheets API 設定
scope = ["https://spreadsheets.google.com/feeds", "https://www.googleapis.com/auth/drive"]
creds = ServiceAccountCredentials.from_json_keyfile_name('credentials.json', scope)
client = gspread.authorize(creds)

# 打開 Google Sheets
sheet = client.open("BeaconList").sheet1

id = 0
last_id = get_last_id(sheet)

rows_to_append = []

for i in range(100):
    priv = random.getrandbits(224)
    adv = ec.derive_private_key(priv, ec.SECP224R1(), default_backend()).public_key().public_numbers().x

    priv_bytes = int.to_bytes(priv, 28, 'big')
    adv_bytes = int.to_bytes(adv, 28, 'big')

    priv_b64 = base64.b64encode(priv_bytes).decode("ascii")
    adv_b64 = base64.b64encode(adv_bytes).decode("ascii")
    s256_b64 = base64.b64encode(sha256(adv_bytes)).decode("ascii")

    if '/' in s256_b64[:7]:
        print('no key file written, there was a / in the b64 of the hashed pubkey :(')
    else:
        id += 1
        current_id = last_id + id
        addr = set_addr_from_key(adv_b64)
        if addr:
            addr_str = ":".join(f"{b:02X}" for b in addr)
            rows_to_append.append([format(current_id, '04x'), priv_b64, adv_b64, s256_b64, '', addr_str, ''])

# 一次性寫入所有資料
sheet.append_rows(rows_to_append)

print("✅ Google Sheets 已更新！")