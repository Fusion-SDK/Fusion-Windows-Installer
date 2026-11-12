using Microsoft.Win32;
using Fusion.Installer.Models;

namespace Fusion.Installer.Services;

public sealed class GameDetectionService
{
    private const string GameFolderName = "Geometry Dash";
    private const string ExecutableName = "GeometryDash.exe";

    public Task<GameInstall?> DetectAsync() => Task.Run(Detect);

    private static GameInstall? Detect()
    {
        foreach (var library in FindSteamLibraries())
        {
            var folder = Path.Combine(library, "steamapps", "common", GameFolderName);
            var executable = Path.Combine(folder, ExecutableName);
            if (File.Exists(executable))
                return new GameInstall(folder, executable);
        }
        return null;
    }

    private static IEnumerable<string> FindSteamLibraries()
    {
        var roots = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var registryPaths = new[]
        {
            (Registry.CurrentUser, @"Software\Valve\Steam", "SteamPath"),
            (Registry.LocalMachine, @"SOFTWARE\WOW6432Node\Valve\Steam", "InstallPath"),
            (Registry.LocalMachine, @"SOFTWARE\Valve\Steam", "InstallPath")
        };

        foreach (var (hive, keyPath, valueName) in registryPaths)
        {
            try
            {
                using var key = hive.OpenSubKey(keyPath);
                var value = key?.GetValue(valueName)?.ToString();
                if (!string.IsNullOrWhiteSpace(value))
                    roots.Add(value.Replace('/', '\\'));
            }
            catch { }
        }

        var common = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Steam");
        if (Directory.Exists(common)) roots.Add(common);

        foreach (var root in roots)
        {
            yield return root;
            var vdf = Path.Combine(root, "steamapps", "libraryfolders.vdf");
            if (!File.Exists(vdf)) continue;
            foreach (var line in File.ReadLines(vdf))
            {
                var marker = line.IndexOf("\"path\"", StringComparison.OrdinalIgnoreCase);
                if (marker < 0) continue;
                var pieces = line.Split('"', StringSplitOptions.RemoveEmptyEntries);
                if (pieces.Length >= 3)
                {
                    var path = pieces[^1].Replace("\\\\", "\\");
                    if (Path.IsPathRooted(path) && Directory.Exists(path))
                        yield return path;
                }
            }
        }
    }
}
