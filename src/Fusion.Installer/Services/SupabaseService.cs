using System.Net.Http;
using System.Net.Http.Headers;
using System.Net.Http.Json;
using System.Text.Json;
using Fusion.Installer.Models;

namespace Fusion.Installer.Services;

public sealed class SupabaseService
{
    private static readonly HttpClient Client = new();
    private readonly string? _url = Environment.GetEnvironmentVariable("FUSION_SUPABASE_URL")?.TrimEnd('/');
    private readonly string? _key = Environment.GetEnvironmentVariable("FUSION_SUPABASE_ANON_KEY");

    public bool IsConfigured => !string.IsNullOrWhiteSpace(_url) && !string.IsNullOrWhiteSpace(_key);

    private void EnsureConfigured()
    {
        if (!IsConfigured)
            throw new InvalidOperationException("Supabase is not configured. Set FUSION_SUPABASE_URL and FUSION_SUPABASE_ANON_KEY.");
    }

    public async Task<AuthSession> SignInAsync(string email, string password)
    {
        EnsureConfigured();
        using var request = new HttpRequestMessage(HttpMethod.Post, $"{_url}/auth/v1/token?grant_type=password");
        request.Headers.Add("apikey", _key);
        request.Content = JsonContent.Create(new { email, password });
        using var response = await Client.SendAsync(request);
        var body = await response.Content.ReadAsStringAsync();
        if (!response.IsSuccessStatusCode)
            throw new InvalidOperationException(ReadError(body));
        return ParseSession(body, email);
    }

    public async Task<AuthSession> RefreshSessionAsync(string refreshToken)
    {
        EnsureConfigured();
        if (string.IsNullOrWhiteSpace(refreshToken))
            throw new ArgumentException("A refresh token is required.", nameof(refreshToken));

        using var request = new HttpRequestMessage(HttpMethod.Post,
            $"{_url}/auth/v1/token?grant_type=refresh_token");
        request.Headers.Add("apikey", _key);
        request.Content = JsonContent.Create(new { refresh_token = refreshToken });
        using var response = await Client.SendAsync(request);
        var body = await response.Content.ReadAsStringAsync();
        if (!response.IsSuccessStatusCode)
            throw new InvalidOperationException(ReadError(body));
        return ParseSession(body, string.Empty, refreshToken);
    }

    public async Task SignOutAsync(string accessToken)
    {
        EnsureConfigured();
        if (string.IsNullOrWhiteSpace(accessToken))
            throw new ArgumentException("An access token is required.", nameof(accessToken));

        using var request = new HttpRequestMessage(HttpMethod.Post, $"{_url}/auth/v1/logout");
        request.Headers.Add("apikey", _key);
        request.Headers.Authorization = new AuthenticationHeaderValue("Bearer", accessToken);
        using var response = await Client.SendAsync(request);
        var body = await response.Content.ReadAsStringAsync();
        if (!response.IsSuccessStatusCode)
            throw new InvalidOperationException(ReadError(body));
    }

    private static AuthSession ParseSession(
        string body,
        string fallbackEmail,
        string? fallbackRefreshToken = null)
    {
        using var document = JsonDocument.Parse(body);
        var root = document.RootElement;
        var accessToken = root.GetProperty("access_token").GetString();
        if (string.IsNullOrWhiteSpace(accessToken))
            throw new InvalidOperationException("The authentication response did not include an access token.");

        var refreshToken = root.TryGetProperty("refresh_token", out var refresh)
            ? refresh.GetString()
            : fallbackRefreshToken;
        var email = fallbackEmail;
        if (root.TryGetProperty("user", out var user) &&
            user.ValueKind == JsonValueKind.Object &&
            user.TryGetProperty("email", out var emailValue))
        {
            email = emailValue.GetString() ?? fallbackEmail;
        }

        var expiresIn = root.TryGetProperty("expires_in", out var expires) &&
            expires.TryGetInt32(out var seconds) && seconds > 0
            ? seconds
            : 3600;

        return new AuthSession(
            accessToken,
            email,
            refreshToken,
            DateTimeOffset.UtcNow.AddSeconds(expiresIn));
    }

    public async Task<string> RegisterAsync(string email, string password)
    {
        EnsureConfigured();
        using var request = new HttpRequestMessage(HttpMethod.Post, $"{_url}/auth/v1/signup");
        request.Headers.Add("apikey", _key);
        request.Content = JsonContent.Create(new { email, password });
        using var response = await Client.SendAsync(request);
        var body = await response.Content.ReadAsStringAsync();
        if (!response.IsSuccessStatusCode)
            throw new InvalidOperationException(ReadError(body));
        return "Registration request accepted. Check your email if email confirmation is enabled.";
    }

    public async Task<List<ModEntry>> GetApprovedModsAsync(string accessToken)
    {
        EnsureConfigured();
        using var request = new HttpRequestMessage(HttpMethod.Get,
            $"{_url}/rest/v1/mods?select=id,name,description,author,latest_version&status=eq.approved&order=name.asc");
        request.Headers.Add("apikey", _key);
        request.Headers.Authorization = new AuthenticationHeaderValue("Bearer", accessToken);
        using var response = await Client.SendAsync(request);
        var body = await response.Content.ReadAsStringAsync();
        if (!response.IsSuccessStatusCode)
            throw new InvalidOperationException(ReadError(body));
        return JsonSerializer.Deserialize<List<ModEntry>>(body,
            new JsonSerializerOptions { PropertyNameCaseInsensitive = true }) ?? [];
    }

    private static string ReadError(string body)
    {
        try
        {
            using var doc = JsonDocument.Parse(body);
            if (doc.RootElement.TryGetProperty("msg", out var msg)) return msg.GetString() ?? "Request failed.";
            if (doc.RootElement.TryGetProperty("message", out var message)) return message.GetString() ?? "Request failed.";
            if (doc.RootElement.TryGetProperty("error_description", out var description)) return description.GetString() ?? "Request failed.";
        }
        catch { }
        return string.IsNullOrWhiteSpace(body) ? "Request failed." : body;
    }
}
