namespace Fusion.Installer.Models;

public sealed record AuthSession(
    string AccessToken,
    string Email,
    string? RefreshToken,
    DateTimeOffset ExpiresAt);
