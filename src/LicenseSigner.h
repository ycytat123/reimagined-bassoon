#pragma once

#include <JuceHeader.h>

// ==============================================================================
// RSA-4096 License Signer
// ==============================================================================
// Loads a PKCS#8 PEM private key and signs license payloads using
// RSA-SHA256 with PKCS#1 v1.5 padding — matching LicenseVerifier's format.
//
// Usage:
//   auto signer = LicenseSigner::createFromPEM(keyFile);
//   juce::String ltclic = signer->sign(licensee, machineId, expiry);
//   licFile.replaceWithText(ltclic);
// ==============================================================================

class LicenseSigner
{
public:
    /// Attempt to load an RSA private key from a PKCS#8 PEM file.
    /// Returns nullptr on failure.
    static std::unique_ptr<LicenseSigner> createFromPEM(const juce::File& pemFile);

    /// Attempt to load from raw PEM text (e.g. from an embedded resource).
    static std::unique_ptr<LicenseSigner> createFromPEMString(const juce::String& pemText);

    /// Sign a license payload and return the complete base64-encoded .ltclic content.
    /// @param licensee    customer name
    /// @param machineId   target machine ID (e.g. "ABCDEF12-34567890")
    /// @param expiry      "perpetual" or "YYYY-MM-DD"
    juce::String sign(const juce::String& licensee,
                      const juce::String& machineId,
                      const juce::String& expiry) const;

    /// Sign raw payload bytes — exposed for testing.
    juce::MemoryBlock signRaw(const juce::String& payload) const;

    /// True if a valid private key was loaded.
    bool isValid() const { return valid; }

public:
    LicenseSigner() = default;

    bool valid = false;
    juce::BigInteger n;  // RSA modulus (4096-bit)
    juce::BigInteger d;  // private exponent
    juce::BigInteger e;  // public exponent (65537)

    // ── PEM helpers ─────────────────────────────────────────────────────
    static juce::MemoryBlock pemToDER(const juce::String& pemText);

    // ── ASN.1 DER minimal parser ────────────────────────────────────────
    struct DERItem
    {
        uint8_t tag = 0;
        const uint8_t* value = nullptr;
        size_t valueLen = 0;
        const uint8_t* end = nullptr;  // first byte after this item
    };

    static bool readDERLength(const uint8_t*& p, const uint8_t* end,
                              size_t& outLen);
    static bool readDERItem(const uint8_t*& p, const uint8_t* end,
                            uint8_t expectedTag, DERItem& item);
    static bool skipDERItem(const uint8_t*& p, const uint8_t* end);
    static juce::BigInteger readDERInteger(const DERItem& item);

    // ── RSA signing ─────────────────────────────────────────────────────
    static juce::MemoryBlock buildPKCS1Padding(const juce::MemoryBlock& hash,
                                                size_t keyBytes);
    static juce::BigInteger modPow(const juce::BigInteger& base,
                                    const juce::BigInteger& exponent,
                                    const juce::BigInteger& modulus);

    JUCE_DECLARE_NON_COPYABLE(LicenseSigner)
};
