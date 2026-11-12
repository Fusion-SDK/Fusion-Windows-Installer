namespace Fusion.Installer.Models;

public sealed class ModEntry
{
    public string Id { get; set; } = "";
    public string Name { get; set; } = "Unnamed mod";
    public string Description { get; set; } = "";
    public string? Author { get; set; }
    public string? LatestVersion { get; set; }
}
