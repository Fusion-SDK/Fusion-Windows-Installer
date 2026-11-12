
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using Fusion.Installer.Models;

namespace Fusion.Installer.Services;

public class GameDetectionService
{
    public Task<GameInstall?> DetectAsync()
    {
        return Task.Run(FindGeometryDash);
    }

    public GameInstall? FindGeometryDash()
    {
        string? steamPath = GetSteamInstallPath();

        if (string.IsNullOrWhiteSpace(steamPath))
            return null;

        foreach (string library in GetSteamLibraryPaths(steamPath))
        {
            string gameDirectory = Path.Combine(
                library,
                "steamapps",
                "common",
                "Geometry Dash"
            );

            string executable = Path.Combine(
                gameDirectory,
                "GeometryDash.exe"
            );

            if (File.Exists(executable))
            {
                return new GameInstall(
                    gameDirectory,
                    executable
                );
            }
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

        string vdfPath = Path.Combine(
            steamPath,
            "steamapps",
            "libraryfolders.vdf"
        );

        if (!File.Exists(vdfPath))
            return libraries;

        foreach (string line in File.ReadLines(vdfPath))
        {
            string trimmed = line.Trim();

            if (!trimmed.StartsWith(
                "\"path\"",
                StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            string[] parts = trimmed.Split('"');

            if (parts.Length < 4)
                continue;

            string libraryPath = parts[3].Replace(@"\\", @"\");

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
