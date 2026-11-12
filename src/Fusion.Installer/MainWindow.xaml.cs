using System;
using System.Diagnostics;
using System.IO;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using Fusion.Installer.Models;
using Fusion.Installer.Services;

namespace Fusion.Installer.Services;

public partial class MainWindow : Window
{
    public Task<GameInstall?> DetectAsync()
    {
        return Task.Run(FindGeometryDash);
    }

    private static GameInstall? FindGeometryDash()
    {
        var steamPath = GetSteamInstallPath();

        if (string.IsNullOrWhiteSpace(steamPath))
            return null;

        foreach (var library in GetSteamLibraryPaths(steamPath))
        {
            var gameDirectory = Path.Combine(
                library,
                "steamapps",
                "common",
                "Geometry Dash"
            );

            var executable = Path.Combine(
                gameDirectory,
                "GeometryDash.exe"
            );

            if (!File.Exists(executable))
                continue;

            return new GameInstall
            {
                GameDirectory = gameDirectory
            };
        }

        return null;
    }

    private static string? GetSteamInstallPath()
    {
        string[] possiblePaths =
        {
            @"C:\Program Files (x86)\Steam",
            @"C:\Program Files\Steam",
            Path.Combine(
                Environment.GetFolderPath(
                    Environment.SpecialFolder.ProgramFilesX86
                ),
                "Steam"
            )
        };

        return possiblePaths.FirstOrDefault(Directory.Exists);
    }

    private static List<string> GetSteamLibraryPaths(string steamPath)
    {
        var libraries = new List<string> { steamPath };

        var vdfPath = Path.Combine(
            steamPath,
            "steamapps",
            "libraryfolders.vdf"
        );

        if (!File.Exists(vdfPath))
            return libraries;

        foreach (var line in File.ReadLines(vdfPath))
        {
            var trimmed = line.Trim();

            if (!trimmed.StartsWith(
                "\"path\"",
                StringComparison.OrdinalIgnoreCase))
                continue;

            var parts = trimmed.Split('"');

            if (parts.Length < 4)
                continue;

            var libraryPath = parts[3].Replace(@"\\", @"\");

            if (Directory.Exists(libraryPath) &&
                !libraries.Contains(
                    libraryPath,
                    StringComparer.OrdinalIgnoreCase))
            {
                libraries.Add(libraryPath);
            }
        }

        return libraries;
    }
}
