using System.Net;
using System.Security.Cryptography;
using System.Text.Json;
using Fusion.Shared;

Console.WriteLine("Fusion Updater");
var gameDir = AppContext.BaseDirectory.TrimEnd(Path.DirectorySeparatorChar);
var dataDir = Path.Combine(gameDir, FusionPaths.DataDirectoryName);
Directory.CreateDirectory(dataDir);
var sourceFile = Path.Combine(dataDir, "update-source.txt");
var manifestUrl = File.Exists(sourceFile) ? (await File.ReadAllTextAsync(sourceFile)).Trim() : "";
if (!Uri.TryCreate(manifestUrl, UriKind.Absolute, out var uri) || uri.Scheme != Uri.UriSchemeHttps)
{
    Console.WriteLine("No HTTPS update manifest URL is configured.");
    Console.WriteLine($"Add the official manifest URL to: {sourceFile}");
    Console.WriteLine("No game files were changed.");
    return 2;
}
using var http = new HttpClient(new HttpClientHandler { AutomaticDecompression = DecompressionMethods.All });
try
{
    var json = await http.GetStringAsync(uri);
    var manifest = JsonSerializer.Deserialize<ReleaseManifest>(json, new JsonSerializerOptions { PropertyNameCaseInsensitive = true })
        ?? throw new InvalidDataException("The update manifest is empty.");
    if (manifest.Files.Count == 0) throw new InvalidDataException("The update manifest contains no files.");
    var installedManifest = Path.Combine(dataDir, FusionPaths.InstalledFilesName);
    var installed = File.Exists(installedManifest)
        ? (JsonSerializer.Deserialize<List<string>>(await File.ReadAllTextAsync(installedManifest)) ?? [])
        : new List<string>();
    foreach (var file in manifest.Files)
    {
        // The updater only manages files under Fusion\. It never replaces GeometryDash.exe or other game files.
        var relative = file.Path.Replace('\\', '/');
        if (!relative.StartsWith("Fusion/", StringComparison.OrdinalIgnoreCase) ||
            relative.Equals("Fusion/installed-files.json", StringComparison.OrdinalIgnoreCase) ||
            relative.Equals("Fusion/update-source.txt", StringComparison.OrdinalIgnoreCase) ||
            relative.EndsWith(".fusion-new", StringComparison.OrdinalIgnoreCase))
            throw new InvalidDataException($"Refusing to update a protected or non-Fusion path: {file.Path}");
        if (!Uri.TryCreate(file.Url, UriKind.Absolute, out var fileUri) || fileUri.Scheme != Uri.UriSchemeHttps)
            throw new InvalidDataException($"Invalid HTTPS download URL for {file.Path}");
        if (file.Sha256.Length != 64 || !file.Sha256.All(Uri.IsHexDigit))
            throw new InvalidDataException($"Invalid SHA-256 value for {file.Path}");
        var destination = FusionPaths.SafeCombine(gameDir, relative);
        Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
        var bytes = await http.GetByteArrayAsync(fileUri);
        var hash = Convert.ToHexString(SHA256.HashData(bytes));
        if (!hash.Equals(file.Sha256, StringComparison.OrdinalIgnoreCase))
            throw new InvalidDataException($"Hash verification failed for {file.Path}");
        var temporary = destination + ".fusion-new";
        await File.WriteAllBytesAsync(temporary, bytes);
        File.Move(temporary, destination, true);
        if (!installed.Contains(relative, StringComparer.OrdinalIgnoreCase)) installed.Add(relative);
        Console.WriteLine($"Updated {relative}");
    }
    await File.WriteAllTextAsync(installedManifest, JsonSerializer.Serialize(installed, new JsonSerializerOptions { WriteIndented = true }));
    Console.WriteLine($"Fusion {manifest.Version} update completed.");
    return 0;
}
catch (Exception ex)
{
    Console.WriteLine($"Update failed: {ex.Message}");
    Console.WriteLine("The updater did not intentionally modify original Geometry Dash files.");
    return 1;
}
