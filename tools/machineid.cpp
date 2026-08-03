// ==============================================================================
// License debug tool.
//
// Usage:
//   machineid_tool.exe                          → print plugin machine ID
//   machineid_tool.exe <license.ltclic>         → verify a license file
//   machineid_tool.exe --check-standard         → verify license in standard location
//
// Build:  cmake --build build --config Release --target machineid_tool
// ==============================================================================
#include <JuceHeader.h>
#include "../src/LicenseVerifier.h"

static void printMachineId()
{
    auto id = LicenseVerifier::getMachineId();
    std::cout << "Plugin machine ID (use THIS for licensing): "
              << id.toStdString() << std::endl;

    auto macs = juce::MACAddress::getAllAddresses();
    std::cout << "MAC addresses (" << macs.size() << "): ";
    for (auto& mac : macs)
        std::cout << mac.toString().toStdString() << " ";
    std::cout << std::endl;
}

static void verifyLicense(const juce::File& file)
{
    auto machineId = LicenseVerifier::getMachineId();
    auto content = file.loadFileAsString();
    auto result = LicenseVerifier::verify(content, machineId);

    std::cout << "License file: " << file.getFullPathName().toStdString() << std::endl;
    std::cout << "Machine ID  : " << machineId.toStdString() << std::endl;
    std::cout << "Authorized  : " << (result.authorized ? "YES" : "NO") << std::endl;
    if (result.authorized)
    {
        std::cout << "Licensee    : " << result.licensee.toStdString() << std::endl;
        std::cout << "Expiry      : " << result.expiryDate.toStdString() << std::endl;
    }
    else
    {
        std::cout << "Error       : " << result.error.toStdString() << std::endl;
    }
}

int main(int argc, char* argv[])
{
    if (argc >= 2)
    {
        juce::String arg(argv[1]);
        if (arg == "--check-standard")
        {
            auto result = LicenseVerifier::checkStandardLocations();
            auto file = juce::File::getSpecialLocation(
                juce::File::userDocumentsDirectory)
                .getChildFile("LTC Reader").getChildFile("license.ltclic");
            std::cout << "Standard license path: "
                      << file.getFullPathName().toStdString() << std::endl;
            std::cout << "Authorized  : " << (result.authorized ? "YES" : "NO") << std::endl;
            std::cout << "Error       : " << result.error.toStdString() << std::endl;
            return result.authorized ? 0 : 1;
        }
        else
        {
            verifyLicense(juce::File(arg));
            return 0;
        }
    }

    printMachineId();
    return 0;
}
