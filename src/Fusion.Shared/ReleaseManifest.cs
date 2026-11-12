using System.Text.Json.Serialization;
namespace Fusion.Shared;
public sealed class ReleaseManifest
{
    [JsonPropertyName("version")] public string Version { get; set; } = "0.0.0";
    [JsonPropertyName("files")] public List<ReleaseFile> Files { get; set; } = [];
}
public sealed class ReleaseFile
{
    [JsonPropertyName("path")] public string Path { get; set; } = "";
    [JsonPropertyName("url")] public string Url { get; set; } = "";
    [JsonPropertyName("sha256")] public string Sha256 { get; set; } = "";
}
public static class FusionPaths
{
    public const string DataDirectoryName = "Fusion";
    public const string InstalledFilesName = "installed-files.json";
    public static string SafeCombine(string root, string relative)
    {
        if (string.IsNullOrWhiteSpace(relative) || Path.IsPathRooted(relative))
            throw new InvalidDataException("Manifest paths must be relative.");
        var fullRoot = Path.GetFullPath(root) + Path.DirectorySeparatorChar;
        var full = Path.GetFullPath(Path.Combine(root, relative.Replace('/', Path.DirectorySeparatorChar)));
        if (!full.StartsWith(fullRoot, StringComparison.OrdinalIgnoreCase))
            throw new InvalidDataException("Manifest path escapes the install directory.");
        return full;
    }
}
