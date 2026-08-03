#!/bin/bash
set -euo pipefail

# ============================================================
# Build macOS .pkg installer for LTC Reader VST3/AU plugin
# ============================================================
# Prerequisites:
#   - Plugin must already be built (Xcode Release)
#   - Build artifacts at build/LTCReader_artefacts/Release/
#
# Usage:
#   ./build_pkg.sh <build_dir> <version> [output_dir]
# ============================================================

BUILD_DIR="${1:-build}"
VERSION="${2:-1.0.0}"
OUTPUT_DIR="${3:-.}"

VST3_SRC="${BUILD_DIR}/LTCReader_artefacts/Release/VST3/LTC Reader.vst3"
AU_SRC="${BUILD_DIR}/LTCReader_artefacts/Release/AU/LTC Reader.component"

PKG_ROOT="${BUILD_DIR}/pkg_root"
IDENTIFIER="com.hahahah.ltcreader"
PKG_NAME="LTC Reader ${VERSION}.pkg"

echo "=== LTC Reader macOS .pkg Builder ==="
echo "Version:     ${VERSION}"
echo "Build dir:   ${BUILD_DIR}"
echo "Output dir:  ${OUTPUT_DIR}"
echo ""

# -------------------------------------------------------
# 1. Validate that the built plugins exist
# -------------------------------------------------------
VST3_FOUND=false
AU_FOUND=false

if [ -d "${VST3_SRC}" ]; then
    echo "  [OK] Found VST3: ${VST3_SRC}"
    VST3_FOUND=true
else
    echo "  [--] VST3 not found, skipping"
fi

if [ -d "${AU_SRC}" ]; then
    echo "  [OK] Found AU:   ${AU_SRC}"
    AU_FOUND=true
else
    echo "  [--] AU not found, skipping"
fi

if [ "${VST3_FOUND}" = false ] && [ "${AU_FOUND}" = false ]; then
    echo "ERROR: No plugins found in build directory."
    exit 1
fi

# -------------------------------------------------------
# 2. Create a signed product archive (.pkg) with a
#    preinstall + postinstall wrapper to make the UX
#    feel more polished. For unsigned / developer-ID
#    signed builds, pkgbuild still produces a valid
#    product that can be installed via "right-click →
#    Open" on the first launch.
# -------------------------------------------------------
rm -rf "${PKG_ROOT}"
mkdir -p "${PKG_ROOT}/Library/Audio/Plug-Ins/VST3"
mkdir -p "${PKG_ROOT}/Library/Audio/Plug-Ins/Components"

if [ "${VST3_FOUND}" = true ]; then
    cp -R "${VST3_SRC}" "${PKG_ROOT}/Library/Audio/Plug-Ins/VST3/"
fi

if [ "${AU_FOUND}" = true ]; then
    cp -R "${AU_SRC}" "${PKG_ROOT}/Library/Audio/Plug-Ins/Components/"
fi

# Fix permissions (Apple recommends root:wheel for system-wide plugins)
chmod -R 755 "${PKG_ROOT}/Library"
# If running as root, also fix ownership:
if [ "$(id -u)" -eq 0 ] 2>/dev/null; then
    chown -R root:wheel "${PKG_ROOT}/Library" 2>/dev/null || true
fi

echo ""
echo "=== Staged plugin tree ==="
find "${PKG_ROOT}" -type f -o -type d | sort
echo ""

# -------------------------------------------------------
# 3. Build a welcome + license HTML and a preinstall
#    script so the installer shows a nice UI.
#    (Optional — if missing pkgbuild skips it.)
# -------------------------------------------------------
mkdir -p "${BUILD_DIR}/pkg_scripts"

# Simple "nothing to run" preinstall
cat > "${BUILD_DIR}/pkg_scripts/preinstall" << 'PRE'
#!/bin/bash
# Remove old versions if present
rm -rf "/Library/Audio/Plug-Ins/VST3/LTC Reader.vst3" 2>/dev/null || true
rm -rf "/Library/Audio/Plug-Ins/Components/LTC Reader.component" 2>/dev/null || true
exit 0
PRE
chmod +x "${BUILD_DIR}/pkg_scripts/preinstall"

# -------------------------------------------------------
# 4. Build the component .pkg
# -------------------------------------------------------
mkdir -p "${OUTPUT_DIR}"

pkgbuild \
    --root "${PKG_ROOT}" \
    --identifier "${IDENTIFIER}" \
    --version "${VERSION}" \
    --install-location / \
    --scripts "${BUILD_DIR}/pkg_scripts" \
    --ownership preserve \
    "${OUTPUT_DIR}/${PKG_NAME}"

echo ""
echo "=== Package created ==="
echo "  ${PKG_NAME}"
ls -lh "${OUTPUT_DIR}/${PKG_NAME}"
echo ""

# -------------------------------------------------------
# 5. Show what's inside (informational)
# -------------------------------------------------------
echo "=== Package contents ==="
pkgutil --payload-files "${OUTPUT_DIR}/${PKG_NAME}" 2>/dev/null || true
echo ""
echo "Done."
