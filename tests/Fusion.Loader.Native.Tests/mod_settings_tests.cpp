#include "fusion/loader/mod_settings.hpp"
#include "fusion/loader/package_manager.hpp"
#include "fusion/loader/sha256.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

class TemporaryDirectory {
public:
    TemporaryDirectory()
    {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
            ("fusion-native-tests-" + std::to_string(stamp));
        std::filesystem::create_directories(path_);
    }

    ~TemporaryDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    const std::filesystem::path& Path() const { return path_; }

private:
    std::filesystem::path path_;
};

void Require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void TestDefaultsProfilesAndPersistence(const std::filesystem::path& root)
{
    const auto settingsPath = root / "profiles.json";
    fusion::loader::ModSettingsStore settings;
    std::string error;

    Require(settings.Load(settingsPath, error), "A missing settings file should load as defaults: " + error);
    Require(settings.GetActiveProfileId() == "default", "The default profile should be active.");
    Require(settings.IsEnabled("example.mod"), "Mods should be enabled by default.");
    Require(settings.GetProfiles().size() == 1, "A new store should contain one default profile.");

    settings.SetEnabled("example.mod", false);
    Require(!settings.IsEnabled("example.mod"), "Disabling a mod should update the active profile.");

    Require(settings.CreateProfile("testing", "Testing", error), "A valid profile should be created: " + error);
    Require(!settings.CreateProfile("testing", "Duplicate", error), "Duplicate profile IDs must be rejected.");
    Require(!settings.CreateProfile("../bad", "Invalid", error), "Unsafe profile IDs must be rejected.");
    Require(settings.SetActiveProfile("testing", error), "The new profile should be selectable: " + error);
    Require(settings.IsEnabled("example.mod"), "A new profile should inherit the default enabled state.");
    settings.SetEnabled("another.mod", false);

    Require(settings.SetActiveProfile("default", error), "The default profile should remain selectable.");
    Require(!settings.IsEnabled("example.mod"), "Each profile should retain its own mod settings.");
    Require(!settings.DeleteProfile("missing", error), "Deleting an unknown profile must fail.");
    Require(settings.DeleteProfile("testing", error), "An inactive profile should be deletable: " + error);
    Require(!settings.DeleteProfile("default", error), "The final profile must not be deletable.");

    Require(settings.Save(error), "Settings should save successfully: " + error);
    fusion::loader::ModSettingsStore reloaded;
    Require(reloaded.Load(settingsPath, error), "Saved settings should reload: " + error);
    Require(reloaded.GetActiveProfileId() == "default", "The active profile should persist.");
    Require(!reloaded.IsEnabled("example.mod"), "Per-profile mod enablement should persist.");
}

void TestKeybindValidationAndPersistence(const std::filesystem::path& root)
{
    const auto settingsPath = root / "keybinds.json";
    fusion::loader::ModSettingsStore settings;
    std::string error;

    Require(settings.Load(settingsPath, error), "Keybind test settings should initialize: " + error);
    Require(settings.SetKeybind("menu.toggle", "Ctrl+F1", error), "A valid keybind should be accepted: " + error);
    Require(!settings.SetKeybind("menu.open", "Ctrl+F1", error), "A chord cannot be assigned to two actions.");
    Require(settings.SetKeybind("menu.toggle", "Ctrl+F2", error), "An action should be allowed to change its chord: " + error);
    Require(settings.GetKeybinds().size() == 1, "Updating an action should not create a duplicate entry.");
    Require(!settings.SetKeybind("../invalid", "Ctrl+F3", error), "Unsafe action identifiers must be rejected.");
    Require(!settings.SetKeybind("menu.close", "", error), "Empty keybind chords must be rejected.");

    Require(settings.Save(error), "Keybind settings should save: " + error);
    fusion::loader::ModSettingsStore reloaded;
    Require(reloaded.Load(settingsPath, error), "Keybind settings should reload: " + error);
    Require(reloaded.GetKeybinds().size() == 1, "Keybind assignments should persist.");
    Require(reloaded.GetKeybinds().front().action == "menu.toggle", "The keybind action should persist.");
    Require(reloaded.GetKeybinds().front().chord == "Ctrl+F2", "The updated keybind chord should persist.");
    Require(reloaded.RemoveKeybind("menu.toggle"), "An existing keybind should be removable.");
    Require(!reloaded.RemoveKeybind("menu.toggle"), "Removing a missing keybind should report no change.");
}

void TestVersionOneMigration(const std::filesystem::path& root)
{
    const auto settingsPath = root / "legacy.json";
    {
        std::ofstream output(settingsPath);
        Require(static_cast<bool>(output), "The legacy fixture should be writable.");
        output << R"({"version":1,"mods":{"legacy.mod":false,"enabled.mod":true}})";
    }

    fusion::loader::ModSettingsStore settings;
    std::string error;
    Require(settings.Load(settingsPath, error), "Version 1 settings should load: " + error);
    Require(settings.GetActiveProfileId() == "default", "Migrated settings should use the default profile.");
    Require(!settings.IsEnabled("legacy.mod"), "Legacy disabled state should be retained.");
    Require(settings.IsEnabled("enabled.mod"), "Legacy enabled state should be retained.");
    Require(settings.Save(error), "Migrated settings should save as version 2: " + error);

    std::ifstream input(settingsPath);
    const std::string serialized((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    Require(serialized.find("\"version\": 2") != std::string::npos,
        "Saving migrated settings should write version 2.");

    fusion::loader::ModSettingsStore reloaded;
    Require(reloaded.Load(settingsPath, error), "Migrated version 2 settings should reload: " + error);
    Require(!reloaded.IsEnabled("legacy.mod"), "Legacy state should survive migration and reload.");
}

void TestMalformedSettingsAreRejected(const std::filesystem::path& root)
{
    const auto settingsPath = root / "malformed.json";
    {
        std::ofstream output(settingsPath);
        Require(static_cast<bool>(output), "The malformed fixture should be writable.");
        output << R"({"version":2,"activeProfile":"default","profiles":[{"id":"default","name":"Default","mods":{}}],"keybinds":[{"action":"menu.one","chord":"Ctrl+F1"},{"action":"menu.one","chord":"Ctrl+F2"}]})";
    }

    fusion::loader::ModSettingsStore settings;
    std::string error;
    Require(!settings.Load(settingsPath, error), "Duplicate keybind actions should not silently load.");
    Require(!error.empty(), "Malformed settings should provide an error message.");
}


void WritePackage(
    const std::filesystem::path& package,
    const std::string& id,
    const std::string& version,
    const std::string& payload)
{
    std::filesystem::create_directories(package);
    {
        std::ofstream manifest(package / "mod.json");
        Require(static_cast<bool>(manifest), "The package manifest should be writable.");
        manifest << R"({"id":")" << id
                 << R"(","name":"Test Mod","author":"Fusion Tests","version":")"
                 << version << R"("})";
    }
    {
        std::ofstream file(package / "payload.txt");
        Require(static_cast<bool>(file), "The package payload should be writable.");
        file << payload;
    }
}

std::string ReadTextFile(const std::filesystem::path& path)
{
    std::ifstream input(path);
    return std::string(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>()
    );
}

void TestPackageInstallUpdateAndUninstall(const std::filesystem::path& root)
{
    const auto mods = root / "installed-mods";
    const auto firstPackage = root / "package-v1";
    const auto updatePackage = root / "package-v2";
    WritePackage(firstPackage, "sample.mod", "1.0.0", "version one");

    fusion::loader::ModPackageManager manager(mods);
    std::string error;
    Require(manager.Install(firstPackage, error), "A valid package should install: " + error);
    Require(ReadTextFile(mods / "sample.mod" / "payload.txt") == "version one",
        "The installed package should contain its payload.");

    WritePackage(updatePackage, "sample.mod", "2.0.0", "version two");
    Require(manager.Install(updatePackage, error), "A valid update should replace the previous package: " + error);
    Require(ReadTextFile(mods / "sample.mod" / "payload.txt") == "version two",
        "The update should activate the new package contents.");
    Require(!std::filesystem::exists(mods / "sample.mod.fusion-backup"),
        "A successful update should remove its backup directory.");
    Require(!std::filesystem::exists(mods / "sample.mod.fusion-staging"),
        "A successful update should remove its staging directory.");

    Require(!manager.Uninstall("../outside", error),
        "Uninstall must reject path-traversal identifiers.");
    Require(manager.Uninstall("sample.mod", error), "An installed package should uninstall: " + error);
    Require(!std::filesystem::exists(mods / "sample.mod"),
        "Uninstall should remove the selected Fusion-owned package.");
}

void TestPackageManagerRejectsUnsafeAndNestedSources(const std::filesystem::path& root)
{
    const auto mods = root / "safe-mods";
    const auto unsafePackage = root / "unsafe-package";
    WritePackage(unsafePackage, "../outside", "1.0.0", "not installed");

    fusion::loader::ModPackageManager manager(mods);
    std::string error;
    Require(!manager.Install(unsafePackage, error),
        "Package installation must reject unsafe mod identifiers.");
    Require(!std::filesystem::exists(root / "outside"),
        "An unsafe package identifier must not create an outside directory.");

    const auto nestedPackage = mods / "nested-package";
    WritePackage(nestedPackage, "nested.mod", "1.0.0", "nested");
    Require(!manager.Install(nestedPackage, error),
        "A package source inside the managed mods directory must be rejected.");
}

void TestPackageManagerRequiresRecoveryForStaleOperations(const std::filesystem::path& root)
{
    const auto mods = root / "recovery-mods";
    const auto package = root / "recovery-package";
    WritePackage(package, "recover.mod", "1.0.0", "payload");
    std::filesystem::create_directories(mods / "recover.mod.fusion-staging");

    fusion::loader::ModPackageManager manager(mods);
    std::string error;
    Require(!manager.Install(package, error),
        "A stale staging directory must block a new install until recovery.");
    Require(!error.empty(), "A blocked recovery operation should explain the failure.");
}


void TestPackageFormatTwoVerifiesSha256(const std::filesystem::path& root)
{
    const auto mods = root / "hashed-mods";
    const auto package = root / "hashed-package";
    std::filesystem::create_directories(package);
    {
        std::ofstream payload(package / "payload.txt", std::ios::binary);
        payload << "hello\n";
    }
    {
        std::ofstream manifest(package / "mod.json");
        manifest << R"({"id":"hashed.mod","name":"Hashed Mod","author":"Fusion Tests","version":"1.0.0","packageFormatVersion":2,"files":[{"path":"payload.txt","sha256":"5891b5b522d5df086d0ff0b110fbd9d21bb4fc7163af34d08286a2e846f6be03"}]})";
    }

    fusion::loader::ModPackageManager manager(mods);
    std::string error;
    Require(manager.Install(package, error),
        "A format 2 package with matching SHA-256 should install: " + error);
    Require(ReadTextFile(mods / "hashed.mod" / "payload.txt") == "hello\n",
        "The verified payload should be installed.");

    const auto tampered = root / "tampered-package";
    std::filesystem::create_directories(tampered);
    {
        std::ofstream payload(tampered / "payload.txt", std::ios::binary);
        payload << "changed";
    }
    {
        std::ofstream manifest(tampered / "mod.json");
        manifest << R"({"id":"tampered.mod","name":"Tampered Mod","author":"Fusion Tests","version":"1.0.0","packageFormatVersion":2,"files":[{"path":"payload.txt","sha256":"5891b5b522d5df086d0ff0b110fbd9d21bb4fc7163af34d08286a2e846f6be03"}]})";
    }
    Require(!manager.Install(tampered, error),
        "A package with a modified payload must be rejected.");
    Require(error.find("SHA-256 mismatch") != std::string::npos,
        "A hash mismatch should provide a clear diagnostic.");
    Require(!std::filesystem::exists(mods / "tampered.mod"),
        "A package that fails integrity verification must not be activated.");
}

void TestPackageFormatTwoRejectsUnlistedFiles(const std::filesystem::path& root)
{
    const auto package = root / "unlisted-package";
    std::filesystem::create_directories(package);
    {
        std::ofstream payload(package / "payload.txt", std::ios::binary);
        payload << "hello\n";
    }
    {
        std::ofstream extra(package / "extra.txt", std::ios::binary);
        extra << "not declared";
    }
    {
        std::ofstream manifest(package / "mod.json");
        manifest << R"({"id":"unlisted.mod","name":"Unlisted Mod","author":"Fusion Tests","version":"1.0.0","packageFormatVersion":2,"files":[{"path":"payload.txt","sha256":"5891b5b522d5df086d0ff0b110fbd9d21bb4fc7163af34d08286a2e846f6be03"}]})";
    }

    fusion::loader::ModPackageManager manager(root / "unlisted-mods");
    std::string error;
    Require(!manager.Install(package, error),
        "Format 2 packages must reject files not declared in the manifest.");
    Require(error.find("unlisted file") != std::string::npos,
        "An undeclared package file should be identified.");
}

} // namespace

int main()
{
    try {
        TemporaryDirectory temporary;
        TestDefaultsProfilesAndPersistence(temporary.Path());
        TestKeybindValidationAndPersistence(temporary.Path());
        TestVersionOneMigration(temporary.Path());
        TestMalformedSettingsAreRejected(temporary.Path());
        TestPackageInstallUpdateAndUninstall(temporary.Path());
        TestPackageManagerRejectsUnsafeAndNestedSources(temporary.Path());
        TestPackageManagerRequiresRecoveryForStaleOperations(temporary.Path());
        TestPackageFormatTwoVerifiesSha256(temporary.Path());
        TestPackageFormatTwoRejectsUnlistedFiles(temporary.Path());\n        TestPackageFormatThreeFailsClosedUntilSignaturesAreImplemented(temporary.Path());
        std::cout << "All Fusion native settings tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception) {
        std::cerr << "Fusion native settings test failure: " << exception.what() << '\n';
        return 1;
    }
}
