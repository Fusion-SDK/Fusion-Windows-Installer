namespace Fusion.Installer.Models;

public sealed record GeodePackageMetadata(
    string Id,
    string Name,
    string Version,
    string GeodeVersion,
    IReadOnlyDictionary<string, string> GameVersions,
    string? Developer,
    string? Description,
    IReadOnlyDictionary<string, string> Dependencies);
