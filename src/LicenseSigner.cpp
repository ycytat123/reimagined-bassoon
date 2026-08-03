#include "LicenseSigner.h"

#if defined(_MSC_VER)
#pragma warning(disable: 4244)  // possible loss of data (BigInteger<->size_t)
#endif

// ==============================================================================
// Public API
// ==============================================================================

std::unique_ptr<LicenseSigner> LicenseSigner::createFromPEM(const juce::File& pemFile)
{
    if (!pemFile.existsAsFile())
        return nullptr;

    juce::String pemText = pemFile.loadFileAsString();
    return createFromPEMString(pemText);
}

std::unique_ptr<LicenseSigner> LicenseSigner::createFromPEMString(const juce::String& pemText)
{
    auto der = pemToDER(pemText);
    if (der.getSize() == 0)
        return nullptr;

    auto signer = std::make_unique<LicenseSigner>();

    // ── Parse PKCS#8 DER structure ──────────────────────────────────────
    const uint8_t* p = static_cast<const uint8_t*>(der.getData());
    const uint8_t* end = p + der.getSize();

    // Outer SEQUENCE (tag 0x30)
    DERItem outerSeq;
    if (!readDERItem(p, end, 0x30, outerSeq))
        return nullptr;

    const uint8_t* inner = outerSeq.value;
    const uint8_t* innerEnd = outerSeq.end;

    // INTEGER version (should be 0)
    if (!skipDERItem(inner, innerEnd))
        return nullptr;

    // SEQUENCE AlgorithmIdentifier — skip it
    if (!skipDERItem(inner, innerEnd))
        return nullptr;

    // OCTET STRING — contains the DER-encoded RSAPrivateKey
    DERItem octetStr;
    if (!readDERItem(inner, innerEnd, 0x04, octetStr))
        return nullptr;

    // ── Parse inner RSAPrivateKey SEQUENCE ──────────────────────────────
    const uint8_t* rsaP = octetStr.value;
    const uint8_t* rsaEnd = octetStr.end;

    DERItem rsaSeq;
    if (!readDERItem(rsaP, rsaEnd, 0x30, rsaSeq))
        return nullptr;

    const uint8_t* fieldP = rsaSeq.value;
    const uint8_t* fieldEnd = rsaSeq.end;

    // Field 0: version INTEGER (0)
    DERItem verInt;
    if (!readDERItem(fieldP, fieldEnd, 0x02, verInt))
        return nullptr;

    // Field 1: modulus n
    DERItem nItem;
    if (!readDERItem(fieldP, fieldEnd, 0x02, nItem))
        return nullptr;
    signer->n = readDERInteger(nItem);

    // Field 2: publicExponent e
    DERItem eItem;
    if (!readDERItem(fieldP, fieldEnd, 0x02, eItem))
        return nullptr;
    signer->e = readDERInteger(eItem);

    // Field 3: privateExponent d
    DERItem dItem;
    if (!readDERItem(fieldP, fieldEnd, 0x02, dItem))
        return nullptr;
    signer->d = readDERInteger(dItem);

    // Validate key size
    int bits = signer->n.getHighestBit();
    if (bits < 2048)
        return nullptr;

    signer->valid = true;
    return signer;
}

juce::String LicenseSigner::sign(const juce::String& licensee,
                                  const juce::String& machineId,
                                  const juce::String& expiry) const
{
    jassert(valid);

    juce::String payload =
        "licensee=" + licensee + "\n"
        "machine_id=" + machineId + "\n"
        "expiry=" + expiry;

    auto sigBlock = signRaw(payload);

    // Build the final blob: payload bytes + signature bytes → base64
    size_t payloadLen = payload.getNumBytesAsUTF8();

    juce::MemoryBlock blob(payloadLen + sigBlock.getSize(), true);
    auto* blobData = static_cast<char*>(blob.getData());
    payload.copyToUTF8(blobData, payloadLen + 1);
    memcpy(blobData + payloadLen, sigBlock.getData(), sigBlock.getSize());

    return blob.toBase64Encoding();
}

juce::MemoryBlock LicenseSigner::signRaw(const juce::String& payload) const
{
    jassert(valid);

    // 1. SHA-256 hash the payload
    juce::SHA256 hashObj(payload.toUTF8());
    auto hashBlock = hashObj.getRawData();

    // 2. Build PKCS#1 v1.5 padded block
    static constexpr size_t kKeyBytes = 512;  // RSA-4096
    auto padded = buildPKCS1Padding(hashBlock, kKeyBytes);

    // 3. Load padded block as BigInteger (big-endian)
    juce::BigInteger plain;
    plain.loadFromMemoryBlock(padded);

    // 4. RSA sign: signature = plain^d mod n
    juce::BigInteger sigValue = modPow(plain, d, n);

    // 5. Convert signature to 512-byte big-endian block
    juce::MemoryBlock signature(kKeyBytes, true);
    auto sigRaw = sigValue.toMemoryBlock();
    size_t sigLen = sigRaw.getSize();
    auto* sigData = static_cast<uint8_t*>(signature.getData());

    if (sigLen <= kKeyBytes)
        memcpy(sigData + (kKeyBytes - sigLen), sigRaw.getData(), sigLen);
    else
        memcpy(sigData,
               static_cast<const uint8_t*>(sigRaw.getData()) + (sigLen - kKeyBytes),
               kKeyBytes);

    return signature;
}

// ==============================================================================
// PEM → DER
// ==============================================================================

juce::MemoryBlock LicenseSigner::pemToDER(const juce::String& pemText)
{
    // Strip PEM headers and whitespace, extract base64 body
    juce::StringArray lines = juce::StringArray::fromLines(pemText);
    juce::String base64;
    for (auto& line : lines)
    {
        auto trimmed = line.trim();
        if (trimmed.startsWith("-----"))  // skip header/footer
            continue;
        base64 += trimmed;
    }

    juce::MemoryBlock der;
    if (!der.fromBase64Encoding(base64))
        return {};

    return der;
}

// ==============================================================================
// Minimal ASN.1 DER parsing
// ==============================================================================

bool LicenseSigner::readDERLength(const uint8_t*& p, const uint8_t* end,
                                   size_t& outLen)
{
    if (p >= end)
        return false;

    uint8_t first = *p++;
    if (first < 0x80)
    {
        outLen = first;
        return true;
    }

    // Long form: first & 0x7F gives number of length bytes
    size_t numLenBytes = first & 0x7F;
    if (numLenBytes > 4 || p + numLenBytes > end)  // 4 bytes = 4 GiB max
        return false;

    outLen = 0;
    for (size_t i = 0; i < numLenBytes; ++i)
        outLen = (outLen << 8) | static_cast<uint8_t>(*p++);

    return true;
}

bool LicenseSigner::readDERItem(const uint8_t*& p, const uint8_t* end,
                                 uint8_t expectedTag, DERItem& item)
{
    if (p >= end)
        return false;

    item.tag = *p++;
    if (item.tag != expectedTag)
        return false;

    if (!readDERLength(p, end, item.valueLen))
        return false;

    if (p + item.valueLen > end)
        return false;

    item.value = p;
    p += item.valueLen;
    item.end = p;
    return true;
}

bool LicenseSigner::skipDERItem(const uint8_t*& p, const uint8_t* end)
{
    if (p >= end)
        return false;

    uint8_t tag = *p++;
    if (tag == 0)
        return false;

    size_t valueLen;
    if (!readDERLength(p, end, valueLen))
        return false;

    // For constructed types (SEQUENCE, SET, context-specific), recurse into children
    // For primitive types (INTEGER, OCTET STRING, OID, NULL, etc.), skip the value
    if (tag & 0x20)  // constructed
    {
        const uint8_t* childEnd = p + valueLen;
        while (p < childEnd)
        {
            if (!skipDERItem(p, childEnd))
                return false;
        }
    }
    else
    {
        p += valueLen;
    }

    return (p <= end);
}

juce::BigInteger LicenseSigner::readDERInteger(const DERItem& item)
{
    juce::BigInteger result;
    result.loadFromMemoryBlock(
        juce::MemoryBlock(item.value, item.valueLen));
    return result;
}

// ==============================================================================
// PKCS#1 v1.5 padding (for SHA-256)
// ==============================================================================

juce::MemoryBlock LicenseSigner::buildPKCS1Padding(
    const juce::MemoryBlock& hash, size_t keyBytes)
{
    // DigestInfo prefix for SHA-256 (DER-encoded)
    static const uint8_t digestInfoPrefix[] = {
        0x30, 0x31, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86,
        0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
        0x00, 0x04, 0x20
    };
    static constexpr size_t prefixLen = sizeof(digestInfoPrefix);
    static constexpr size_t kHashLen = 32;  // SHA-256

    jassert(hash.getSize() == kHashLen);

    size_t padLen = keyBytes - 3 - prefixLen - kHashLen;

    juce::MemoryBlock padded(keyBytes, true);
    auto* b = static_cast<uint8_t*>(padded.getData());

    b[0] = 0x00;
    b[1] = 0x01;  // block type 1 (private-key operation / signature)
    for (size_t i = 0; i < padLen; ++i)
        b[2 + i] = 0xFF;
    b[2 + padLen] = 0x00;
    memcpy(b + 3 + padLen, digestInfoPrefix, prefixLen);
    memcpy(b + 3 + padLen + prefixLen, hash.getData(), kHashLen);

    return padded;
}

// ==============================================================================
// Modular exponentiation (square-and-multiply)
// ==============================================================================

juce::BigInteger LicenseSigner::modPow(const juce::BigInteger& base,
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
        if (e[0])  // LSB is 1
        {
            result = result * b;
            result %= modulus;
        }

        juce::BigInteger remainder;
        e.divideBy(two, remainder);

        b = b * b;
        b %= modulus;
    }

    return result;
}
