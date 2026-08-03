// ==============================================================================
// LTC Reader — License Generator (GUI)
//
// Standalone JUCE app for generating .ltclic license files.
// Requires license_private.pem in the same directory.
//
// Build:
//   cmake --build build --config Release --target license_gen
// ==============================================================================

#include <JuceHeader.h>
#include "../src/LicenseSigner.h"
#include "../src/LicenseVerifier.h"

// ==============================================================================
// Dark LookAndFeel
// ==============================================================================
struct GenLookAndFeel : juce::LookAndFeel_V4
{
    GenLookAndFeel()
    {
        setColour(juce::Label::textColourId,              juce::Colour(0xffd4d4d4));
        setColour(juce::TextEditor::backgroundColourId,   juce::Colour(0xff1e2128));
        setColour(juce::TextEditor::textColourId,         juce::Colour(0xffe0e0e0));
        setColour(juce::TextEditor::outlineColourId,      juce::Colour(0xff3a3e47));
        setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff4ec9b0));
        setColour(juce::TextButton::buttonColourId,       juce::Colour(0xff4ec9b0));
        setColour(juce::TextButton::textColourOnId,       juce::Colour(0xff1a1d23));
        setColour(juce::TextButton::textColourOffId,      juce::Colour(0xffd4d4d4));
        setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff1a1d23));
        setColour(juce::ComboBox::backgroundColourId,     juce::Colour(0xff1e2128));
        setColour(juce::ComboBox::textColourId,           juce::Colour(0xffe0e0e0));
        setColour(juce::ComboBox::outlineColourId,        juce::Colour(0xff3a3e47));
        setColour(juce::PopupMenu::backgroundColourId,    juce::Colour(0xff24272e));
        setColour(juce::PopupMenu::textColourId,          juce::Colour(0xffd4d4d4));
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& btn,
                              const juce::Colour& bg, bool over, bool down) override
    {
        auto r = btn.getLocalBounds().toFloat().reduced(0.5f);
        auto c = bg;
        if (over) c = c.brighter(0.12f);
        if (down) c = c.darker(0.12f);
        g.setColour(c);
        g.fillRoundedRectangle(r, 5.0f);
    }
};

// ==============================================================================
// Main component
// ==============================================================================
class LicenseGenComponent : public juce::Component,
                             public juce::Button::Listener
{
public:
    LicenseGenComponent()
    {
        setLookAndFeel(&laf);

        // ── Title ─────────────────────────────────────────────────────
        addAndMakeVisible(titleLabel);
        titleLabel.setText("LTC Reader — License Generator", juce::dontSendNotification);
        titleLabel.setFont(juce::FontOptions("Segoe UI", 18.0f, juce::Font::bold));
        titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffffff));
        titleLabel.setJustificationType(juce::Justification::centred);

        // ── Key status bar ────────────────────────────────────────────
        addAndMakeVisible(keyStatusLabel);
        keyStatusLabel.setFont(juce::FontOptions("Consolas", 12.0f, juce::Font::plain));
        keyStatusLabel.setJustificationType(juce::Justification::centredLeft);

        keyLoadBtn.setButtonText("Load Private Key...");
        keyLoadBtn.addListener(this);
        addAndMakeVisible(keyLoadBtn);

        // ── Form fields ───────────────────────────────────────────────
        addAndMakeVisible(licenseeLabel);
        licenseeLabel.setText("Licensee (customer name)", juce::dontSendNotification);
        licenseeLabel.setFont(juce::FontOptions("Segoe UI", 12.5f, juce::Font::bold));

        addAndMakeVisible(licenseeInput);
        licenseeInput.setFont(juce::FontOptions("Consolas", 13.5f, juce::Font::plain));
        licenseeInput.setTextToShowWhenEmpty("e.g. Beijing Studio", juce::Colour(0xff5b6d80));

        addAndMakeVisible(machineLabel);
        machineLabel.setText("Machine ID (from plugin UI)", juce::dontSendNotification);
        machineLabel.setFont(juce::FontOptions("Segoe UI", 12.5f, juce::Font::bold));

        addAndMakeVisible(machineInput);
        machineInput.setFont(juce::FontOptions("Consolas", 13.5f, juce::Font::bold));
        machineInput.setTextToShowWhenEmpty("ABCDEF12-34567890", juce::Colour(0xff5b6d80));
        machineInput.onTextChange = [this] {
            auto t = machineInput.getText().toUpperCase();
            if (t != machineInput.getText())
                machineInput.setText(t);
        };

        addAndMakeVisible(expiryLabel);
        expiryLabel.setText("Expiry date (empty = perpetual)", juce::dontSendNotification);
        expiryLabel.setFont(juce::FontOptions("Segoe UI", 12.5f, juce::Font::bold));

        addAndMakeVisible(expiryInput);
        expiryInput.setFont(juce::FontOptions("Consolas", 13.5f, juce::Font::plain));
        expiryInput.setTextToShowWhenEmpty("perpetual", juce::Colour(0xff5b6d80));
        expiryInput.setInputRestrictions(10, "0123456789-");

        // ── Generate button ───────────────────────────────────────────
        generateBtn.setButtonText("Generate & Save License File (.ltclic)");
        generateBtn.addListener(this);
        generateBtn.setEnabled(false);
        addAndMakeVisible(generateBtn);

        // ── Output log ────────────────────────────────────────────────
        addAndMakeVisible(outputLabel);
        outputLabel.setText("Log", juce::dontSendNotification);
        outputLabel.setFont(juce::FontOptions("Segoe UI", 12.5f, juce::Font::bold));

        addAndMakeVisible(outputEditor);
        outputEditor.setMultiLine(true, false);
        outputEditor.setReadOnly(true);
        outputEditor.setFont(juce::FontOptions("Consolas", 11.5f, juce::Font::plain));
        outputEditor.setText("Ready. Load a private key to begin.");

        // Auto-detect private key
        tryAutoLoadKey();
    }

    ~LicenseGenComponent() override { setLookAndFeel(nullptr); }

    void resized() override
    {
        auto a = getLocalBounds().reduced(24);

        titleLabel.setBounds(a.removeFromTop(32));

        // Key bar
        auto keyBar = a.removeFromTop(30);
        keyStatusLabel.setBounds(keyBar.removeFromLeft(keyBar.getWidth() - 140));
        keyLoadBtn.setBounds(keyBar.removeFromRight(130));

        a.removeFromTop(14);

        // Form
        licenseeLabel.setBounds(a.removeFromTop(20));
        licenseeInput.setBounds(a.removeFromTop(30).reduced(2, 0));
        a.removeFromTop(10);

        machineLabel.setBounds(a.removeFromTop(20));
        machineInput.setBounds(a.removeFromTop(30).reduced(2, 0));
        a.removeFromTop(10);

        expiryLabel.setBounds(a.removeFromTop(20));
        expiryInput.setBounds(a.removeFromTop(30).reduced(2, 0));
        a.removeFromTop(16);

        generateBtn.setBounds(a.removeFromTop(34));

        a.removeFromTop(10);

        outputLabel.setBounds(a.removeFromTop(20));
        outputEditor.setBounds(a);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff1a1d23));
    }

    void buttonClicked(juce::Button* btn) override
    {
        if (btn == &keyLoadBtn)  browseForPrivateKey();
        if (btn == &generateBtn) generateAndSave();
    }

private:
    // ── Key management ────────────────────────────────────────────────
    void tryAutoLoadKey()
    {
        auto exeDir = juce::File::getSpecialLocation(
            juce::File::currentExecutableFile).getParentDirectory();

        juce::File candidates[] = {
            exeDir.getChildFile("license_private.pem"),
            juce::File::getCurrentWorkingDirectory().getChildFile("license_private.pem"),
            juce::File::getCurrentWorkingDirectory()
                .getChildFile("installer").getChildFile("license_private.pem"),
        };

        for (auto& f : candidates)
        {
            if (f.existsAsFile()) { loadKey(f); return; }
        }

        setKeyStatus("No key found. Click [Load Private Key...]",
                     juce::Colour(0xffff8844));
    }

    void browseForPrivateKey()
    {
        juce::FileChooser chooser("Select Private Key (license_private.pem)",
                                   juce::File::getCurrentWorkingDirectory(),
                                   "*.pem;*.key;*");

        auto flags = juce::FileBrowserComponent::openMode
                   | juce::FileBrowserComponent::canSelectFiles;

        chooser.launchAsync(flags, [this](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result != juce::File())
                loadKey(result);
        });
    }

    void loadKey(const juce::File& f)
    {
        signer = LicenseSigner::createFromPEM(f);
        if (signer && signer->isValid())
        {
            keyPath = f.getFullPathName();
            setKeyStatus("Key: " + f.getFileName() + " (RSA-4096 valid)",
                         juce::Colour(0xff4ec9b0));
            generateBtn.setEnabled(true);
        }
        else
        {
            signer.reset();
            setKeyStatus("Invalid key file: " + f.getFileName(),
                         juce::Colour(0xfff44747));
            generateBtn.setEnabled(false);
        }
    }

    void setKeyStatus(const juce::String& msg, juce::Colour c)
    {
        keyStatusLabel.setText(msg, juce::dontSendNotification);
        keyStatusLabel.setColour(juce::Label::textColourId, c);
    }

    // ── Generate & save ──────────────────────────────────────────────
    void generateAndSave()
    {
        if (!signer || !signer->isValid()) return;

        auto licensee  = licenseeInput.getText().trim();
        auto machineId = machineInput.getText().trim().toUpperCase();
        auto expiry    = expiryInput.getText().trim();
        if (expiry.isEmpty()) expiry = "perpetual";

        if (licensee.isEmpty() || machineId.isEmpty())
        {
            log("ERROR: Licensee name and Machine ID are required.",
                juce::Colour(0xfff44747));
            return;
        }

        juce::String ltclic;
        try
        {
            ltclic = signer->sign(licensee, machineId, expiry);
        }
        catch (...)
        {
            log("ERROR: Signing failed. Check private key.",
                juce::Colour(0xfff44747));
            return;
        }

        // Build default filename
        juce::String safeName = licensee.replaceCharacter(' ', '_')
                                  .retainCharacters(
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-");
        juce::String defName = safeName + "_" + machineId + ".ltclic";

        juce::File defPath = juce::File::getCurrentWorkingDirectory()
                                 .getChildFile(defName);

        juce::FileChooser saver("Save License File", defPath, "*.ltclic");
        auto flags = juce::FileBrowserComponent::saveMode
                   | juce::FileBrowserComponent::canSelectFiles
                   | juce::FileBrowserComponent::warnAboutOverwriting;

        saver.launchAsync(flags, [this, licensee, machineId, expiry, ltclic]
            (const juce::FileChooser& fc)
        {
            auto dest = fc.getResult();
            if (dest == juce::File()) return; // cancelled

            dest.replaceWithText(ltclic);

            log(juce::String() +
                "=== LICENSE GENERATED ===\n"
                "  Licensee:   " + licensee + "\n"
                "  Machine ID: " + machineId + "\n"
                "  Expiry:     " + expiry + "\n"
                "  Saved to:   " + dest.getFullPathName() + "\n\n"
                "Install on client machine:\n"
                "  ~/Documents/LTC Reader/license.ltclic\n",
                juce::Colour(0xff4ec9b0));
        });
    }

    void log(const juce::String& msg, juce::Colour c)
    {
        outputEditor.setText(msg);
        outputEditor.applyColourToAllText(c);
    }

    // ── Members ──────────────────────────────────────────────────────
    GenLookAndFeel laf;
    std::unique_ptr<LicenseSigner> signer;
    juce::String keyPath;

    juce::Label       titleLabel;
    juce::Label       keyStatusLabel;
    juce::TextButton  keyLoadBtn;

    juce::Label       licenseeLabel;
    juce::TextEditor  licenseeInput;

    juce::Label       machineLabel;
    juce::TextEditor  machineInput;

    juce::Label       expiryLabel;
    juce::TextEditor  expiryInput;

    juce::TextButton  generateBtn;

    juce::Label       outputLabel;
    juce::TextEditor  outputEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LicenseGenComponent)
};

// ==============================================================================
// Main window
// ==============================================================================
class LicenseGenWindow : public juce::DocumentWindow
{
public:
    LicenseGenWindow()
        : DocumentWindow("LTC Reader — License Generator",
                         juce::Colour(0xff1a1d23),
                         juce::DocumentWindow::closeButton, true)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new LicenseGenComponent(), true);
        setResizable(true, false);
        setResizeLimits(440, 530, 680, 750);
        centreWithSize(480, 580);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
};

// ==============================================================================
// App
// ==============================================================================
class LicenseGenApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return "LTC License Gen"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }

    void initialise(const juce::String&) override
    {
        mainWindow = std::make_unique<LicenseGenWindow>();
    }

    void shutdown() override { mainWindow.reset(); }

    std::unique_ptr<LicenseGenWindow> mainWindow;
};

START_JUCE_APPLICATION(LicenseGenApp)
