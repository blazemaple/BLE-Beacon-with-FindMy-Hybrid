#!/usr/bin/env python3
import sys, base64, hashlib, random
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.backends import default_backend

def set_addr_from_key(base64_key):
    # Base64 解碼
    binary_key = base64.b64decode(base64_key)
    if len(binary_key) != 28:
        print(f"警告：解碼後的長度為 {len(binary_key)}，但預期為 28")
        return None
    binary_key_str = ", ".join(f"{b:02X}" for b in binary_key)
    print(f"解碼後的 binary_key: {binary_key_str}")
    # 轉成 C 語言的陣列格式
    public_key_array = ", ".join(f"0x{b:02X}" for b in binary_key)
    print(f"轉成 C 語言的陣列格式: {public_key_array}")
    code = "static char public_key[28] = { " + public_key_array + " };"
    print(code)

    # 設定 addr
    addr = [0] * 6
    addr[0] = binary_key[0] | 0b11000000
    addr[1] = binary_key[1]
    addr[2] = binary_key[2]
    addr[3] = binary_key[3]
    addr[4] = binary_key[4]
    addr[5] = binary_key[5]

    return addr
adv_b64 = "+6+JjhLMG2tJTx6LSQzM6u1Lxal/fXdHnHrg8w=="
addr = set_addr_from_key(adv_b64)
if addr:
    addr_str = ":".join(f"{b:02X}" for b in addr)
    print(f"📡 Address: {addr_str}")