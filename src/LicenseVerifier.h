#pragma once

#include <JuceHeader.h>

// ==============================================================================
// RSA-4096 based license verification
// ==============================================================================
// The plugin embeds an RSA-4096 public key.
// License files (.ltclic) are base64-encoded signed payloads generated
// offline with the matching private key (installer/license_private.pem).
// Each license binds to a machine-specific ID derived from MAC addresses.
// ==============================================================================

class LicenseVerifier
{
public:
    struct LicenseInfo
    {
        bool authorized = false;
        juce::String licensee;
        juce::String expiryDate;   // "YYYY-MM-DD" or "perpetual"
        juce::String error;
    };

    /// Verify a license file's contents (base64-encoded signed payload).
    static LicenseInfo verify(const juce::String& licenseData,
                              const juce::String& machineId);

    /// Search standard locations for a .ltclic file and verify it.
    static LicenseInfo checkStandardLocations();

    /// Derive a stable machine ID from the system's MAC address(es).
    static juce::String getMachineId();

private:
    /// Parse a base64 license blob into (payload, signature) parts.
    static bool parseLicenseBlob(const juce::String& blob,
                                 juce::String& payload,
                                 juce::MemoryBlock& signature);

    /// Verify RSA-SHA256 PKCS#1 v1.5 signature.
    static bool verifySignature(const juce::String& payload,
                                const juce::MemoryBlock& signature);

    JUCE_DECLARE_NON_COPYABLE(LicenseVerifier)
};
