using System.IO.Compression;
using System.Text.Json;
using Fusion.Installer.Models;

namespace Fusion.Installer.Services;

/// <summary>
/// Reads the root mod.json from a Geode .geode archive without extracting or
/// loading any package files. This is inspection only, not compatibility proof.
/// </summary>
public sealed class GeodePackageMetadataReader
{
    private const int MaxArchiveEntries = 4096;
    private const long MaxArchiveBytes = 256L * 1024 * 1024;
    private const long MaxManifestBytes = 1024L * 1024;

    public bool TryRead(string? packagePath, out GeodePackageMetadata? metadata, out string error)
    {
        metadata = null;
        error = string.Empty;

        if (string.IsNullOrWhiteSpace(packagePath))
        {
            error = "A Geode package path is required.";
            return false;
        }

        try
        {
            var fullPath = Path.GetFullPath(packagePath.Trim());
            if (!string.Equals(Path.GetExtension(fullPath), ".geode", StringComparison.OrdinalIgnoreCase))
            {
                error = "The selected file is not a .geode package.";
                return false;
            }

            var file = new FileInfo(fullPath);
            if (!file.Exists)
            {
                error = "The Geode package does not exist.";
                return false;
            }

            if (file.Length <= 0 || file.Length > MaxArchiveBytes)
            {
                error = "The Geode package size is outside the supported inspection limit.";
                return false;
            }

            using var archive = ZipFile.OpenRead(fullPath);
            if (archive.Entries.Count > MaxArchiveEntries)
            {
                error = "The Geode package contains too many entries to inspect safely.";
                return false;
            }

            var manifests = archive.Entries
                .Where(entry => string.Equals(entry.FullName, "mod.json", StringComparison.Ordinal))
                .ToArray();

            if (manifests.Length != 1)
            {
                error = "The package must contain exactly one root-level mod.json file.";
                return false;
            }

            var manifestEntry = manifests[0];
            if (manifestEntry.Length <= 0 || manifestEntry.Length > MaxManifestBytes)
            {
                error = "The package manifest is empty or exceeds the 1 MiB inspection limit.";
                return false;
            }

            using var stream = manifestEntry.Open();
            using var buffer = new MemoryStream((int)manifestEntry.Length);
            stream.CopyTo(buffer);
            if (buffer.Length > MaxManifestBytes)
            {
                error = "The package manifest exceeds the 1 MiB inspection limit.";
                return false;
            }

            using var document = JsonDocument.Parse(buffer.ToArray(), new JsonDocumentOptions
            {
                MaxDepth = 32,
                AllowTrailingCommas = false,
                CommentHandling = JsonCommentHandling.Disallow
            });

            var root = document.RootElement;
            if (root.ValueKind != JsonValueKind.Object)
            {
                error = "The package manifest must be a JSON object.";
                return false;
            }

            var id = RequiredString(root, "id");
            var name = RequiredString(root, "name");
            var version = RequiredString(root, "version");
            var geodeVersion = RequiredString(root, "geode");

            if (!IsSafeId(id))
            {
                error = "The package manifest contains an invalid mod ID.";
                return false;
            }

            var gameVersions = ReadStringMap(root, "gd");
            if (gameVersions.Count == 0)
            {
                error = "The package manifest does not declare any supported game versions.";
                return false;
            }

            var dependencies = ReadStringMap(root, "dependencies");
            var developer = OptionalString(root, "developer");
            var description = OptionalString(root, "description");

            metadata = new GeodePackageMetadata(
                id, name, version, geodeVersion, gameVersions, developer, description, dependencies);
            return true;
        }
        catch (Exception ex) when (
            ex is ArgumentException or NotSupportedException or PathTooLongException or
            IOException or InvalidDataException or UnauthorizedAccessException or JsonException)
        {
            error = ex is JsonException
                ? "The package mod.json file is not valid JSON."
                : $"Could not inspect the Geode package: {ex.Message}";
            return false;
        }
    }

    private static string RequiredString(JsonElement root, string propertyName)
    {
        if (!root.TryGetProperty(propertyName, out var value) ||
            value.ValueKind != JsonValueKind.String ||
            string.IsNullOrWhiteSpace(value.GetString()))
            throw new InvalidDataException($"The package manifest is missing required string '{propertyName}'.");

        return value.GetString()!;
    }

    private static string? OptionalString(JsonElement root, string propertyName)
    {
        if (!root.TryGetProperty(propertyName, out var value) || value.ValueKind == JsonValueKind.Null)
            return null;
        if (value.ValueKind != JsonValueKind.String)
            throw new InvalidDataException($"The package manifest property '{propertyName}' must be a string.");
        return value.GetString();
    }

    private static IReadOnlyDictionary<string, string> ReadStringMap(JsonElement root, string propertyName)
    {
        if (!root.TryGetProperty(propertyName, out var value) || value.ValueKind == JsonValueKind.Null)
            return new Dictionary<string, string>(StringComparer.Ordinal);
        if (value.ValueKind != JsonValueKind.Object)
            throw new InvalidDataException($"The package manifest property '{propertyName}' must be an object.");

        var result = new Dictionary<string, string>(StringComparer.Ordinal);
        foreach (var property in value.EnumerateObject())
        {
            if (property.Value.ValueKind != JsonValueKind.String ||
                string.IsNullOrWhiteSpace(property.Value.GetString()))
                throw new InvalidDataException(
                    $"The package manifest property '{propertyName}.{property.Name}' must be a non-empty string.");
            result[property.Name] = property.Value.GetString()!;
        }
        return result;
    }

    private static bool IsSafeId(string id)
    {
        if (id.Length is < 1 or > 128 || id is "." or ".." || id.Contains("..", StringComparison.Ordinal))
            return false;
        return id.All(character =>
            char.IsAsciiLetterOrDigit(character) || character is '.' or '_' or '-');
    }
}
