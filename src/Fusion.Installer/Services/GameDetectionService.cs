
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;

namespace Fusion.Installer.Services;

public class GameDetectionService
{
    public Task<string?> DetectAsync()
    {
        return Task.Run(FindGeometryDash);
    }

    public string? FindGeometryDash()
    {
        var steamPath = GetSteamInstallPath();

        if (string.IsNullOrWhiteSpace(steamPath))
            return null;

        var libraries = GetSteamLibraryPaths(steamPath);

        foreach (var library in libraries)
        {
            var gamePath = Path.Combine(
                library,
                "steamapps",
                "common",
                "Geometry Dash"
            );

            var executable = Path.Combine(
                gamePath,
                "GeometryDash.exe"
            );

            if (File.Exists(executable))
                return executable;
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
            {
                continue;
            }

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
