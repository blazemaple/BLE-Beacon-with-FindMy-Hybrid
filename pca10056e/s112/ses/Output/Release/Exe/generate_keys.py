#!/usr/bin/env python3
import sys, base64, hashlib, random, csv
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.backends import default_backend
import argparse

def sha256(data):
    digest = hashlib.new("sha256")
    digest.update(data)
    return digest.digest()

def get_last_id(filename):
    try:
        with open(filename, 'r') as csvfile:
            reader = csv.reader(csvfile)
            rows = list(reader)
            if rows:
                last_id = rows[-1][0]
                return int(last_id, 16)
            else:
                return 0
    except FileNotFoundError:
        return 0

id = 0
last_id = get_last_id('data.csv')
with open('data.csv', 'a', newline='') as csvfile:
    writer = csv.writer(csvfile)

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
            writer.writerow([format(current_id, '04x'), priv_b64, adv_b64, s256_b64])