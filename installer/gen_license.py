#!/usr/bin/env python3
"""
LTC Reader — 一键许可证生成器
=============================

这是一个独立的 Python 脚本，**不依赖 OpenSSL 外部命令**，
使用 Python 标准库的 cryptography 完成 RSA-SHA256 签名。

用法:
    # 1. 用客户机器码生成许可证
    python gen_license.py generate \
        --name "北京录音棚" \
        --machine ABCDEF12-34567890 \
        --output 北京录音棚.ltclic

    # 2. 带有效期
    python gen_license.py generate \
        --name "张三工作室" \
        --machine ABCDEF12-34567890 \
        --expiry 2027-12-31

    # 3. 查看许可证内容
    python gen_license.py inspect 北京录音棚.ltclic

    # 4. 获取本机机器码（用于测试）
    python gen_license.py machineid

前置条件:
    pip install cryptography
    (或者直接用系统自带的，只有 hashlib + base64 也行 —
     签名需要用 OpenSSL，见下面 "方案B")
"""

import sys
import os
import base64
import hashlib
import struct
from datetime import datetime

# ── 密钥文件路径 ────────────────────────────────────────────────────────
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PRIVATE_KEY_PATH = os.path.join(SCRIPT_DIR, "license_private.pem")
PUBLIC_KEY_PATH = os.path.join(SCRIPT_DIR, "license_public.pem")


# ==========================================================================
# 方案 A: 纯 Python (cryptography)
# ==========================================================================
try:
    from cryptography.hazmat.primitives import hashes, serialization
    from cryptography.hazmat.primitives.asymmetric import padding
    from cryptography.hazmat.backends import default_backend

    _HAS_CRYPTO = True
except ImportError:
    _HAS_CRYPTO = False


def sign_with_cryptography(payload_bytes):
    """RSA-SHA256 PKCS#1 v1.5 签名（cryptography 库）"""
    if not os.path.exists(PRIVATE_KEY_PATH):
        raise FileNotFoundError(f"找不到私钥: {PRIVATE_KEY_PATH}")

    with open(PRIVATE_KEY_PATH, "rb") as f:
        private_key = serialization.load_pem_private_key(
            f.read(), password=None, backend=default_backend()
        )

    sig = private_key.sign(
        payload_bytes,
        padding.PKCS1v15(),
        hashes.SHA256(),
    )
    return sig


# ==========================================================================
# 方案 B: 调用 OpenSSL CLI（备选，无需额外安装）
# ==========================================================================
import subprocess
import tempfile


def sign_with_openssl(payload_bytes):
    """RSA-SHA256 签名（调用系统 openssl）"""
    if not os.path.exists(PRIVATE_KEY_PATH):
        raise FileNotFoundError(f"找不到私钥: {PRIVATE_KEY_PATH}")

    with tempfile.NamedTemporaryFile(delete=False, suffix=".sig") as sig_file:
        sig_path = sig_file.name

    try:
        proc = subprocess.run(
            [
                "openssl", "dgst", "-sha256", "-sign", PRIVATE_KEY_PATH,
                "-out", sig_path,
            ],
            input=payload_bytes,
            capture_output=True,
            text=False,
        )

        if proc.returncode != 0:
            err_msg = proc.stderr.decode("utf-8", errors="replace") if proc.stderr else "unknown"
            raise RuntimeError(f"OpenSSL 签名失败: {err_msg}")

        with open(sig_path, "rb") as f:
            sig = f.read()

        return sig
    finally:
        if os.path.exists(sig_path):
            os.unlink(sig_path)


def sign(payload_bytes):
    """自动选择可用的签名方式"""
    if _HAS_CRYPTO:
        return sign_with_cryptography(payload_bytes)
    else:
        return sign_with_openssl(payload_bytes)


# ==========================================================================
# 许可证生成
# ==========================================================================
def generate_license(licensee, machine_id, expiry="perpetual"):
    """
    生成 .ltclic 文件内容。
    格式: base64(payload + RSA-4096-SHA256-signature)
    """
    # 验证机器码格式（可选）
    if not machine_id or len(machine_id) < 8:
        raise ValueError("无效的机器码: 至少需要 8 位字符")

    # 构建 payload
    payload = (
        f"licensee={licensee}\n"
        f"machine_id={machine_id}\n"
        f"expiry={expiry}"
    )
    payload_bytes = payload.encode("utf-8")

    # RSA-4096 签名 = 512 字节
    sig = sign(payload_bytes)
    print(f"  签名： {len(sig)} 字节 (RSA-4096)")
    print(f"  机器码： {machine_id}")

    # 拼接 + base64
    blob = payload_bytes + sig
    encoded = base64.b64encode(blob).decode("ascii")

    return encoded


def inspect_license(filepath):
    """查看许可证文件的 payload 部分"""
    if not os.path.exists(filepath):
        print(f"File not found: {filepath}")
        return

    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read().strip()

    decoded = base64.b64decode(content)
    payload_len = len(decoded) - 512  # RSA-4096 sig = 512 bytes

    if payload_len <= 0:
        print("Invalid license file (too short)")
        return

    payload_text = decoded[:payload_len].decode("utf-8", errors="replace")
    print("=== License Info ===")
    for line in payload_text.split("\n"):
        print(f"  {line}")
    print(f"  Signature: {len(decoded) - payload_len} bytes (RSA-4096)")


def get_machine_id():
    """
    从本机 MAC 地址生成机器码。
    格式: 8位hex-8位hex（与 C++ 插件一致）
    """
    # macOS / Linux
    if sys.platform in ("darwin", "linux"):
        import subprocess
        try:
            result = subprocess.run(
                ["ifconfig"], capture_output=True, text=True
            )
            import re
            macs = re.findall(
                r"([0-9a-f]{2}(?::[0-9a-f]{2}){5})",
                result.stdout,
                re.IGNORECASE,
            )
        except Exception:
            macs = []

        if not macs:
            # fallback: use dbus machine-id
            try:
                with open("/var/lib/dbus/machine-id", "r") as f:
                    seed = f.read().strip()
            except FileNotFoundError:
                try:
                    with open("/etc/machine-id", "r") as f:
                        seed = f.read().strip()
                except FileNotFoundError:
                    seed = "unknown"
            macs = [seed]

        seed = "".join(macs)
    else:
        # Windows — use getmac
        import subprocess
        try:
            result = subprocess.run(
                ["getmac", "/v", "/fo", "csv"], capture_output=True, text=True
            )
            import re
            macs = re.findall(
                r"([0-9A-F]{2}-[0-9A-F]{2}-[0-9A-F]{2}-[0-9A-F]{2}-[0-9A-F]{2}-[0-9A-F]{2})",
                result.stdout,
            )
        except Exception:
            macs = []

        if not macs:
            import socket
            seed = socket.gethostname()
        else:
            seed = "".join(macs)

    h = hashlib.sha256(seed.encode())
    hex_str = h.hexdigest().upper()
    return f"{hex_str[:8]}-{hex_str[8:16]}"


# ==========================================================================
# CLI
# ==========================================================================
def print_help():
    print(__doc__)
    print("\n当前环境: ", end="")
    if _HAS_CRYPTO:
        print("✅ cryptography 可用（纯 Python 签名）")
    else:
        print("⚠️  cryptography 未安装，将使用 openssl CLI")
        print("   安装: pip install cryptography")


def main():
    if len(sys.argv) < 2:
        print_help()
        return

    cmd = sys.argv[1].lower()

    if cmd == "machineid":
        mid = get_machine_id()
        print(f"Machine ID: {mid}")
        print(f"\nSend this ID to the license issuer.")

    elif cmd == "generate":
        import argparse
        parser = argparse.ArgumentParser()
        parser.add_argument("generate", help="生成许可证")
        parser.add_argument("--name", "-n", required=True, help="授权客户名称")
        parser.add_argument("--machine", "-m", required=True, help="客户机器码")
        parser.add_argument("--expiry", "-e", default="perpetual",
                            help="到期日 (YYYY-MM-DD)，默认永久")
        parser.add_argument("--output", "-o", default=None,
                            help="输出文件名（默认: 名称_机器码.ltclic）")
        args = parser.parse_args()

        encoded = generate_license(args.name, args.machine, args.expiry)

        outname = args.output or f"{args.name}_{args.machine}.ltclic"
        # 替换空格
        outname = outname.replace(" ", "_")

        with open(outname, "w", encoding="utf-8") as f:
            f.write(encoded)

        print(f"\nLicense generated: {outname}")
        print(f"  Licensee:   {args.name}")
        print(f"  Machine ID: {args.machine}")
        print(f"  Expiry:     {args.expiry}")
        print(f"\nInstallation:")
        print(f"  macOS:   mkdir -p ~/Documents/LTC\\ Reader/")
        print(f"           cp {outname} ~/Documents/LTC\\ Reader/license.ltclic")
        print(f"  Windows: mkdir %USERPROFILE%\\Documents\\LTC Reader")
        print(f"           copy {outname} %USERPROFILE%\\Documents\\LTC Reader\\license.ltclic")

    elif cmd == "inspect":
        if len(sys.argv) < 3:
            print("用法: python gen_license.py inspect <file.ltclic>")
            return
        inspect_license(sys.argv[2])

    else:
        print_help()


if __name__ == "__main__":
    main()
