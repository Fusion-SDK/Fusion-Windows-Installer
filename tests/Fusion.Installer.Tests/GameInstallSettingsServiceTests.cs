using Fusion.Installer.Services;
using Xunit;

namespace Fusion.Installer.Tests;

public sealed class GameInstallSettingsServiceTests
{
    [Fact]
    public void SaveAndLoad_RestoresSelectedGameDirectory()
    {
        var root = CreateTemporaryDirectory();
        try
        {
            var settingsPath = Path.Combine(root, "config", "installer-settings.json");
            var service = new GameInstallSettingsService(settingsPath);

            service.SaveGameDirectory(root);

            var loaded = new GameInstallSettingsService(settingsPath).LoadGameDirectory();

            Assert.Equal(Path.GetFullPath(root), loaded);
        }
        finally
        {
            Directory.Delete(root, recursive: true);
        }
    }

    [Fact]
    public void LoadGameDirectory_ReturnsNullWhenSettingsAreMissing()
    {
        var settingsPath = Path.Combine(
            Path.GetTempPath(),
            Guid.NewGuid().ToString("N"),
            "installer-settings.json");

        var loaded = new GameInstallSettingsService(settingsPath).LoadGameDirectory();

        Assert.Null(loaded);
    }

    [Fact]
    public void LoadGameDirectory_ReturnsNullForMalformedSettings()
    {
        var root = CreateTemporaryDirectory();
        try
        {
            var settingsPath = Path.Combine(root, "installer-settings.json");
            File.WriteAllText(settingsPath, "{ invalid json");

            var loaded = new GameInstallSettingsService(settingsPath).LoadGameDirectory();

            Assert.Null(loaded);
        }
        finally
        {
            Directory.Delete(root, recursive: true);
        }
    }

    [Fact]
    public void SaveGameDirectory_RejectsBlankPath()
    {
        var service = new GameInstallSettingsService(
            Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N"), "settings.json"));

        Assert.Throws<ArgumentException>(() => service.SaveGameDirectory(" "));
    }

    private static string CreateTemporaryDirectory()
    {
        var path = Path.Combine(Path.GetTempPath(), "FusionTests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(path);
        return path;
    }
}
