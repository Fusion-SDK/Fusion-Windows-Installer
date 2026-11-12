using System.Text.Json;

namespace Fusion.Installer.Services;

public sealed class UpdateService
{
    private static readonly HttpClient Client = new();

    public async Task<UpdateInfo?> CheckAsync(string currentVersion)
    {
        var endpoint = Environment.GetEnvironmentVariable("FUSION_UPDATES_URL");
        if (string.IsNullOrWhiteSpace(endpoint)) return null;

        using var response = await Client.GetAsync(endpoint);
        response.EnsureSuccessStatusCode();
        await using var stream = await response.Content.ReadAsStreamAsync();
        var update = await JsonSerializer.DeserializeAsync<UpdateInfo>(stream,
            new JsonSerializerOptions { PropertyNameCaseInsensitive = true });

        if (update is null || string.IsNullOrWhiteSpace(update.Version))
            throw new InvalidOperationException("The update endpoint returned invalid metadata.");

        update.IsNewer = Version.TryParse(update.Version, out var latest)
            && Version.TryParse(currentVersion, out var current)
            && latest > current;
        return update;
    }
}

public sealed class UpdateInfo
{
    public string Version { get; set; } = "";
    public string? DownloadUrl { get; set; }
    public string? ReleaseNotes { get; set; }
    public bool IsNewer { get; set; }
}
