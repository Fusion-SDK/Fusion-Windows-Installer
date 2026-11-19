using System.IO;
using System.Text.Json;

namespace Fusion.Installer.Services;

public sealed class GameInstallSettingsService
{
    private readonly string _settingsPath;
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        WriteIndented = true
    };

    public GameInstallSettingsService(string? settingsPath = null)
    {
        _settingsPath = settingsPath ?? Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            "Fusion",
            "installer-settings.json");
    }

    public string? LoadGameDirectory()
    {
        try
        {
            if (!File.Exists(_settingsPath))
                return null;

            var settings = JsonSerializer.Deserialize<InstallerSettings>(
                File.ReadAllText(_settingsPath));

            return string.IsNullOrWhiteSpace(settings?.GameDirectory)
                ? null
                : Path.GetFullPath(settings.GameDirectory);
        }
        catch (Exception ex) when (
            ex is JsonException or
            IOException or
            UnauthorizedAccessException or
            ArgumentException or
            NotSupportedException)
        {
            return null;
        }
    }

    public void SaveGameDirectory(string gameDirectory)
    {
        if (string.IsNullOrWhiteSpace(gameDirectory))
            throw new ArgumentException("A game directory is required.", nameof(gameDirectory));

        var fullPath = Path.GetFullPath(gameDirectory);
        var directory = Path.GetDirectoryName(_settingsPath)
            ?? throw new InvalidOperationException("The settings path has no parent directory.");

        Directory.CreateDirectory(directory);
        var temporaryPath = _settingsPath + "." + Guid.NewGuid().ToString("N") + ".tmp";

        try
        {
            var json = JsonSerializer.Serialize(
                new InstallerSettings(fullPath),
                JsonOptions);
            File.WriteAllText(temporaryPath, json);
            File.Move(temporaryPath, _settingsPath, overwrite: true);
        }
        finally
        {
            if (File.Exists(temporaryPath))
                File.Delete(temporaryPath);
        }
    }

    private sealed record InstallerSettings(string? GameDirectory);
}
