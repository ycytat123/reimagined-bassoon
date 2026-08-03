#!/bin/bash
# ==============================================================================
# Generate a signed LTC Reader license file.
# ------------------------------------------------------------------------------
# Prerequisites:  openssl, license_private.pem (same directory)
#
# Usage:
#   终端1（客户机器 - 生成机器码）:   ./gen_license.sh machine
#   终端2（授权签发 - 生成许可证）:   ./gen_license.sh sign <licensee> <machine_id> [expiry]
#
# Example:
#   ./gen_license.sh machine                          → 输出机器码
#   ./gen_license.sh sign "Acme" A1B2C3D4-E5F6A7B8    → 输出许可证文件内容（永久）
#   ./gen_license.sh sign "Acme" A1B2C3D4-E5F6A7B8 2027-12-31  → 带有效期
# ==============================================================================
set -euo pipefail

SUBCOMMAND="${1:-help}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PRIVATE_KEY="${SCRIPT_DIR}/license_private.pem"

case "$SUBCOMMAND" in
# ── 生成客户机器码 ─────────────────────────────────────────────────────
getmachineid)
    MACHINE_ID=$(cat /var/lib/dbus/machine-id 2>/dev/null || echo "unknown")
    HASH=$(echo -n "$MACHINE_ID" | openssl dgst -sha256 | tr 'a-z' 'A-Z' | cut -c1-16)
    echo "Machine ID: $HASH"
    ;;

# ── 签发许可证 ─────────────────────────────────────────────────────────
sign)
    LICENSEE="${2:?Usage: $0 sign <licensee> <machine_id> [expiry]}"
    MACHINE_ID="${3:?Usage: $0 sign <licensee> <machine_id> [expiry]}"
    EXPIRY="${4:-perpetual}"

    if [ ! -f "$PRIVATE_KEY" ]; then
        echo "ERROR: Private key not found: $PRIVATE_KEY" >&2
        exit 1
    fi

    # ── Build payload ───────────────────────────────────────────────────
    PAYLOAD="licensee=${LICENSEE}\nmachine_id=${MACHINE_ID}\nexpiry=${EXPIRY}"

    # ── Sign payload with RSA-SHA256 ──────────────────────────────────
    SIG_FILE="$(mktemp)"
    trap 'rm -f "$SIG_FILE"' EXIT

    echo -ne "$PAYLOAD" | openssl dgst -sha256 -sign "$PRIVATE_KEY" \
        -out "$SIG_FILE"

    # ── Encode: payload + signature → base64 → license file ─────────
    LIC_FILE="${LICENSEE// /_}_${MACHINE_ID}.ltclic"
    {
        echo -ne "$PAYLOAD"
        cat "$SIG_FILE"
    } | base64 > "$LIC_FILE"

    echo "License generated: $LIC_FILE"
    echo "  Licensee:   $LICENSEE"
    echo "  Machine ID: $MACHINE_ID"
    echo "  Expiry:     $EXPIRY"

    # ── Instructions ─────────────────────────────────────────────────
    echo ""
    echo "=== Installation ==="
    echo "Copy this file to the target machine:"
    echo "  macOS:   ~/Documents/LTC Reader/license.ltclic"
    echo "  Windows: %USERPROFILE%\\Documents\\LTC Reader\\license.ltclic"
    echo ""
    echo "  mkdir -p ~/Documents/LTC\\ Reader/"
    echo "  cp ${LIC_FILE} ~/Documents/LTC\\ Reader/license.ltclic"
    ;;

# ── 查看许可证内容（不验证） ─────────────────────────────────────────
inspect)
    LIC_FILE="${2:?Usage: $0 inspect <license_file>}"
    echo -n "Raw base64 length: "
    wc -c "$LIC_FILE"

    DECODED=$(base64 -d "$LIC_FILE" | xxd | head -20)
    echo "$DECODED"

    # Try to extract payload text
    echo ""
    echo "=== Payload (text portion) ==="
    base64 -d "$LIC_FILE" | head -c -512
    echo ""
    ;;

help|*)
    cat << 'EOF'
LTC Reader — License Generator
===============================

Usage:
  ./gen_license.sh machine                输出本机机器码
  ./gen_license.sh sign <name> <machine_id> [expiry]   签发许可证
  ./gen_license.sh inspect <file>         查看许可证内容
  ./gen_license.sh help                   本帮助

Important:
  - Keep license_private.pem SECRET — never distribute it.
  - The private key lives only on the issuer's machine.
  - The public key is embedded in the compiled plugin.
EOF
    ;;
esac
