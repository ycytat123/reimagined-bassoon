#!/bin/bash
set -euo pipefail
# ============================================================================
# Generate a signed LTC Reader license file.
#
# Usage:   ./gen_license.sh <licensee> <machine_id> [expiry_date]
# Example: ./gen_license.sh "Acme Studios" "A1B2-C3D4" "2027-12-31"
#
# Output:  base64-encoded license file written to stdout (redirect to file)
# ============================================================================

LICENSEE="${1:?Usage: $0 <licensee> <machine_id> [expiry_date]}"
MACHINE_ID="${2:?Usage: $0 <licensee> <machine_id> [expiry_date]}"
EXPIRY="${3:-perpetual}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PRIVATE_KEY="${SCRIPT_DIR}/license_private.pem"

if [ ! -f "$PRIVATE_KEY" ]; then
    echo "ERROR: Private key not found: $PRIVATE_KEY" >&2
    echo "Run this script where license_private.pem lives, or set the path." >&2
    exit 1
fi

# ── Payload ────────────────────────────────────────────────────────────
PAYLOAD="licensee=${LICENSEE}\nmachine_id=${MACHINE_ID}\nexpiry=${EXPIRY}"

# ── Sign with RSA-SHA256 ───────────────────────────────────────────────
SIG_FILE="$(mktemp)"
trap 'rm -f "$SIG_FILE"' EXIT

echo -ne "$PAYLOAD" | openssl dgst -sha256 -sign "$PRIVATE_KEY" \
    -out "$SIG_FILE"

# ── Encode: payload + signature, base64 ───────────────────────────────
{
    echo -ne "$PAYLOAD"
    cat "$SIG_FILE"
} | base64
