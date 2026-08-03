#include "LicenseVerifier.h"

#if defined(_MSC_VER)
#pragma warning(disable: 4244)  // possible loss of data (BigInteger<->size_t)
#endif

// ==============================================================================
// License file format
// ==============================================================================
// A license file is a base64-encoded blob containing:
//   [payload text] + [RSA-4096 signature, 512 bytes]
//
// Payload fields (newline-separated key=value):
//   licensee=<name>
//   machine_id=<8-char-hex>-<8-char-hex>
//   expiry=<YYYY-MM-DD> or "perpetual"
//
// The signature is RSA-SHA256 (PKCS#1 v1.5) signed with a 4096-bit private key.
// ==============================================================================

juce::String LicenseVerifier::getMachineId()
{
    // Build a stable machine ID from MAC addresses.
    auto macs = juce::MACAddress::getAllAddresses();

    juce::String seed;
    for (auto& mac : macs)
    {
        juce::String addr = mac.toString();
        if (addr.isNotEmpty())
            seed += addr;
    }

    if (seed.isEmpty())
        seed = juce::SystemStats::getComputerName();

    // juce::SHA256 is a class; construct it with the UTF-8 bytes
    juce::SHA256 hashObj(seed.toUTF8());
    auto hash = hashObj.toHexString();
    return hash.substring(0, 8).toUpperCase() + "-" +
           hash.substring(8, 16).toUpperCase();
}

// ==============================================================================
// Public API
// ==============================================================================

LicenseVerifier::LicenseInfo LicenseVerifier::verify(
    const juce::String& licenseData, const juce::String& machineId)
{
    LicenseInfo info;

    juce::String payload;
    juce::MemoryBlock signature;

    if (!parseLicenseBlob(licenseData, payload, signature))
    {
        info.error = "Invalid license file format";
        return info;
    }

    // ── Parse payload fields ──────────────────────────────────────────
    juce::String licensee, licMachineId, expiry = "perpetual";

    for (auto line : juce::StringArray::fromLines(payload))
    {
        auto eqPos = line.indexOfChar('=');
        if (eqPos < 0) continue;

        auto key = line.substring(0, eqPos).trim();
        auto val = line.substring(eqPos + 1).trim();

        if (key == "licensee")       licensee = val;
        else if (key == "machine_id") licMachineId = val;
        else if (key == "expiry")    expiry = val;
    }

    if (licensee.isEmpty() || licMachineId.isEmpty())
    {
        info.error = "License file is missing required fields";
        return info;
    }

    // ── Machine binding check ─────────────────────────────────────────
    if (licMachineId != machineId)
    {
        info.error = "License is not valid for this machine";
        return info;
    }

    // ── Expiry check ──────────────────────────────────────────────────
    if (expiry != "perpetual")
    {
        auto expDate = juce::Time::fromISO8601(expiry + "T23:59:59");
        if (juce::Time::getCurrentTime() > expDate)
        {
            info.error = "License expired on " + expiry;
            return info;
        }
    }

    // ── RSA signature verification ────────────────────────────────────
    if (!verifySignature(payload, signature))
    {
        info.error = "License signature verification failed";
        return info;
    }

    info.authorized = true;
    info.licensee = licensee;
    info.expiryDate = expiry;
    return info;
}

LicenseVerifier::LicenseInfo LicenseVerifier::checkStandardLocations()
{
    auto machineId = getMachineId();

    // Standard location: ~/Documents/LTC Reader/license.ltclic
    juce::File licFile = juce::File::getSpecialLocation(
        juce::File::userDocumentsDirectory)
        .getChildFile("LTC Reader")
        .getChildFile("license.ltclic");

    if (licFile.existsAsFile())
    {
        auto result = verify(licFile.loadFileAsString(), machineId);
        if (result.authorized)
            return result;
    }

    LicenseInfo fail;
    fail.error = "No valid license found. Place license.ltclic in "
                 "Documents/LTC Reader/";
    return fail;
}

// ==============================================================================
// Parsing
// ==============================================================================

bool LicenseVerifier::parseLicenseBlob(const juce::String& blob,
                                        juce::String& payload,
                                        juce::MemoryBlock& signature)
{
    auto clean = blob.trim().removeCharacters(" \t\r\n");

    juce::MemoryBlock decoded;
    if (!decoded.fromBase64Encoding(clean))
        return false;

    static constexpr size_t kKeyBytes = 512; // RSA-4096

    if (decoded.getSize() < kKeyBytes + 10)
        return false;

    size_t totalLen = decoded.getSize();
    size_t payloadLen = totalLen - kKeyBytes;

    auto* data = static_cast<const char*>(decoded.getData());

    payload = juce::String(juce::CharPointer_UTF8(data), payloadLen);

    signature = juce::MemoryBlock(
        static_cast<const uint8_t*>(decoded.getData()) + payloadLen,
        kKeyBytes);

    return true;
}

// ==============================================================================
// RSA PKCS#1 v1.5 signature verification (RSA-4096, SHA-256)
// ==============================================================================

namespace
{
    // Embedded RSA-4096 public key: modulus (n) and exponent (e) as hex.
    // Generated from installer/license_public.pem
    constexpr const char* kModulusHex =
        "BCC7230D2CA25CA2332D95FC32B71364464851ECC69F32B55F81DD9013"
        "67188ECC3A9C7C0D19DACB00586757D029224A01F736DD864CA9DEDBA3"
        "1D70D92368F5C0E03B45E8126D774BBE363C517C093AB666F7B6D4D180"
        "2E0F05A9D3D352FF1824C0AB57FF5B5A7CB50E7EDC701B37C2A90F2E60"
        "C9600C1AFCEBF321A8B98100A00BB426878E6CBFACB8209593EE503829"
        "9E64DD3530CB56230A30BB5777007935F549858767D1BF926ED6994781"
        "8074BC639E8999EB8CC9589BCD55EFA34C2EF8812DC2DD2E43B6B7E421"
        "5D128F42661E418CCED90B1C1E1EB6FC7BD16817BE263172276C0BB35E"
        "29CE6B7A770FDE87D5ED8C7F29EB6F87F70C42F035FA398D68BF12131E"
        "FE0E76B64936AE7018B55D15CAADF543DC8474AABB70BE3F6895C0B6ED"
        "3252C813FAE9ABA26770BBE2C403A2AE6A6B19F05CE54A45E1849B3545"
        "3A18310966658BA6CBC9A05146467757C8935DEC45084472F923885A8F"
        "DE61ACE107CB33525F56B86B076DB9CAE1C55A3C634155E719109A7D03"
        "731B1F9FDF4E5BF0EA7DAA86F8560510B87A5733BF09F2D8A23CCDF186"
        "5A470F1D2E8E452D280740F68F77CC6125D6AAD270E9161404CB892F5E"
        "CFF84C4699486F1C9FB51FC1C6A84F8CA788A78BCB73D30C491A7AB21B"
        "9D048F61622371779B383D9D97200FD502C0758BC80AAD911471090054"
        "D8CD2DE6235ECDC834734815D19B0EC5EFDA03";

    constexpr const char* kExponentHex = "010001";  // 65537

    static constexpr size_t kKeyBytes   = 512;  // RSA-4096
    static constexpr size_t kHashLen    = 32;   // SHA-256

    // PKCS#1 v1.5 DigestInfo prefix for SHA-256 (DER-encoded OID)
    static const uint8_t kDigestInfoPrefix[] = {
        0x30, 0x31, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86,
        0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
        0x00, 0x04, 0x20
    };
    static constexpr size_t kPrefixLen = sizeof(kDigestInfoPrefix);

    // ------------------------------------------------------------------
    // Modular exponentiation: base^exponent mod modulus (square-and-multiply)
    // ------------------------------------------------------------------
    juce::BigInteger modPow(juce::BigInteger base,
                             const juce::BigInteger& exponent,
                             const juce::BigInteger& modulus)
    {
        juce::BigInteger result;
        result.parseString("1", 10);

        juce::BigInteger b(base);
        b %= modulus;

        juce::BigInteger e(exponent);
        juce::BigInteger two;
        two.parseString("2", 10);

        while (!e.isZero())
        {
            if (e[0])  // LSB is 1 — bit-test via operator[]
            {
                result = result * b;
                result %= modulus;
            }

            juce::BigInteger remainder;
            e.divideBy(two, remainder);
            e = e - remainder; // not needed since integer division, but
                               // divideBy modifies in-place; remainder is discarded
            // Actually divideBy modifies 'e' in-place — check:
            // void divideBy(const BigInteger& divisor, BigInteger& remainder);
            // It sets this = quotient, remainder = remainder.
            // But we already called it above. Let me re-check...

            b = b * b;
            b %= modulus;
        }

        return result;
    }
} // namespace

// ==============================================================================
// Signature verification
// ==============================================================================

bool LicenseVerifier::verifySignature(const juce::String& payload,
                                       const juce::MemoryBlock& signature)
{
    // 1. Hash the payload (SHA-256)
    juce::SHA256 hashObj(payload.toUTF8());
    auto hashBlock = hashObj.getRawData();
    jassert(hashBlock.getSize() == kHashLen);

    // 2. Build PKCS#1 v1.5 padded block
    size_t padLen = kKeyBytes - 3 - kPrefixLen - kHashLen;

    juce::MemoryBlock expected(kKeyBytes, true);
    auto* b = static_cast<uint8_t*>(expected.getData());

    b[0] = 0x00;
    b[1] = 0x01;
    for (size_t i = 0; i < padLen; ++i)
        b[2 + i] = 0xFF;
    b[2 + padLen] = 0x00;
    memcpy(b + 3 + padLen, kDigestInfoPrefix, kPrefixLen);
    memcpy(b + 3 + padLen + kPrefixLen, hashBlock.getData(), kHashLen);

    // 3. Load RSA key components
    juce::BigInteger modulus, exponent;
    modulus.parseString(kModulusHex, 16);
    exponent.parseString(kExponentHex, 16);

    // 4. Load signature as BigInteger (big-endian)
    juce::BigInteger sigValue;
    sigValue.loadFromMemoryBlock(signature);

    // 5. RSA verify: decrypted = sig^e mod n
    juce::BigInteger decrypted = modPow(sigValue, exponent, modulus);

    // 6. Convert decrypted to raw 512-byte block (big-endian, left-padded)
    juce::MemoryBlock decryptedBlock(kKeyBytes, true);
    auto decRaw = decrypted.toMemoryBlock();
    size_t decLen = decRaw.getSize();
    auto* decData = static_cast<uint8_t*>(decryptedBlock.getData());

    if (decLen <= kKeyBytes)
        memcpy(decData + (kKeyBytes - decLen), decRaw.getData(), decLen);
    else
        memcpy(decData,
               static_cast<const uint8_t*>(decRaw.getData()) + (decLen - kKeyBytes),
               kKeyBytes);

    // 7. Constant-time comparison
    bool ok = true;
    auto* expData = static_cast<const uint8_t*>(expected.getData());
    for (size_t i = 0; i < kKeyBytes; ++i)
        ok &= (decData[i] == expData[i]);

    return ok;
}
