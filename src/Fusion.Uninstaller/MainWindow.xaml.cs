using System.Diagnostics;
using System.IO;
using System.Net.Http;
using System.Reflection;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Windows;
using Fusion.Shared;

namespace Fusion.Uninstaller;

public partial class MainWindow : Window
{
    private const string ReleasesApi = "https://api.github.com/repos/Fusion-SDK/Fusion-Windows-Installer/releases/latest";
    private const string SetupAssetName = "Fusion-Setup.exe";
    private static readonly HttpClient Http = CreateHttpClient();

    public MainWindow()
    {
        InitializeComponent();
        GamePathBox.Text = AppContext.BaseDirectory.TrimEnd(Path.DirectorySeparatorChar);
        RefreshInstallationStatus();
    }

    private static HttpClient CreateHttpClient()
    {
        var client = new HttpClient();
        client.DefaultRequestHeaders.UserAgent.ParseAdd("Fusion-Manager/1.0");
        client.Timeout = TimeSpan.FromMinutes(5);
        return client;
    }

    private void Detect_Click(object sender, RoutedEventArgs e)
    {
        var candidate = AppContext.BaseDirectory.TrimEnd(Path.DirectorySeparatorChar);
        if (File.Exists(Path.Combine(candidate, "GeometryDash.exe")))
        {
            GamePathBox.Text = candidate;
            StatusText.Text = "Geometry Dash folder detected.";
            RefreshInstallationStatus();
        }
        else
        {
            StatusText.Text = "Manager is not in a Geometry Dash folder. Paste the folder containing GeometryDash.exe.";
        }
    }

    private void RefreshInstallationStatus()
    {
        var root = GamePathBox.Text.Trim();
        var manifest = Path.Combine(root, FusionPaths.DataDirectoryName, FusionPaths.InstalledFilesName);
        var installed = File.Exists(Path.Combine(root, "GeometryDash.exe")) && File.Exists(manifest);
        InstallStatusText.Text = installed ? "Fusion is installed" : "Fusion not detected";
        VersionText.Text = installed
            ? "Installation manifest found. Check for updates to get the latest release."
            : "Select your Geometry Dash folder or run Fusion Setup first.";
    }

    private async void Update_Click(object sender, RoutedEventArgs e) => await LaunchLatestSetupAsync("update");
    private async void Repair_Click(object sender, RoutedEventArgs e) => await LaunchLatestSetupAsync("repair");

    private async Task LaunchLatestSetupAsync(string action)
    {
        var root = GamePathBox.Text.Trim();
        if (!File.Exists(Path.Combine(root, "GeometryDash.exe")))
        {
            StatusText.Text = "Choose the folder containing GeometryDash.exe first.";
            return;
        }

        UpdateButton.IsEnabled = false;
        StatusText.Text = "Checking the latest official Fusion release…";
        try
        {
            using var response = await Http.GetAsync(ReleasesApi);
            response.EnsureSuccessStatusCode();
            await using var stream = await response.Content.ReadAsStreamAsync();
            using var release = await JsonDocument.ParseAsync(stream);
            var tag = release.RootElement.TryGetProperty("tag_name", out var tagElement)
                ? tagElement.GetString() ?? "latest"
                : "latest";
            var assets = release.RootElement.GetProperty("assets");
            var setupUrl = assets.EnumerateArray()
                .Where(a => a.TryGetProperty("name", out var name) && name.GetString() == SetupAssetName)
                .Select(a => a.GetProperty("browser_download_url").GetString())
                .FirstOrDefault(url => Uri.TryCreate(url, UriKind.Absolute, out var parsed) &&
                                       parsed.Scheme == Uri.UriSchemeHttps &&
                                       parsed.Host.EndsWith("github.com", StringComparison.OrdinalIgnoreCase));

            if (setupUrl is null)
                throw new InvalidDataException($"The latest release ({tag}) does not contain {SetupAssetName}.");

            StatusText.Text = $"Downloading Fusion {tag}…";
            var bytes = await Http.GetByteArrayAsync(setupUrl);
            if (bytes.Length < 1024 * 1024)
                throw new InvalidDataException("The release installer is unexpectedly small; refusing to run it.");

            var tempDirectory = Path.Combine(Path.GetTempPath(), "FusionManager");
            Directory.CreateDirectory(tempDirectory);
            var installerPath = Path.Combine(tempDirectory, $"Fusion-Setup-{Guid.NewGuid():N}.exe");
            await File.WriteAllBytesAsync(installerPath, bytes);

            var escapedInstaller = installerPath.Replace("\"", "\"\"");
            var command = $"/c timeout /t 2 /nobreak > nul & start \"\" \"{escapedInstaller}\"";
            Process.Start(new ProcessStartInfo("cmd.exe", command)
            {
                CreateNoWindow = true,
                UseShellExecute = false,
                WorkingDirectory = tempDirectory
            });
            StatusText.Text = $"Fusion {tag} downloaded. Setup will open to {action} the installation.";
            Close();
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Could not prepare the latest release: {ex.Message}";
            UpdateButton.IsEnabled = true;
        }
    }

    private void Remove_Click(object sender, RoutedEventArgs e)
    {
        var root = GamePathBox.Text.Trim();
        if (!File.Exists(Path.Combine(root, "GeometryDash.exe")))
        {
            StatusText.Text = "Choose the folder containing GeometryDash.exe.";
            return;
        }
        if (MessageBox.Show("Remove Fusion's managed files from this folder? Original Geometry Dash files and unrelated files will be kept.",
                "Uninstall Fusion", MessageBoxButton.YesNo, MessageBoxImage.Warning) != MessageBoxResult.Yes)
            return;

        try
        {
            var dataDir = Path.Combine(root, FusionPaths.DataDirectoryName);
            var manifest = Path.Combine(dataDir, FusionPaths.InstalledFilesName);
            if (!File.Exists(manifest))
                throw new FileNotFoundException("Fusion's installed-files manifest was not found. No files were removed.", manifest);

            var files = JsonSerializer.Deserialize<List<string>>(File.ReadAllText(manifest)) ?? [];
            foreach (var relative in files.Where(x =>
                         !x.Equals("XInput1_4.dll.fusion-backup", StringComparison.OrdinalIgnoreCase) &&
                         !x.Equals("Fusion.Manager.exe", StringComparison.OrdinalIgnoreCase) &&
                         !x.Equals("Fusion.Uninstaller.exe", StringComparison.OrdinalIgnoreCase) &&
                         !x.Equals("Fusion.Updater.exe", StringComparison.OrdinalIgnoreCase)))
            {
                var path = FusionPaths.SafeCombine(root, relative);
                if (File.Exists(path)) File.Delete(path);
            }

            var proxyBackup = Path.Combine(root, "XInput1_4.dll.fusion-backup");
            if (File.Exists(proxyBackup))
            {
                var proxy = Path.Combine(root, "XInput1_4.dll");
                if (File.Exists(proxy)) File.Delete(proxy);
                File.Move(proxyBackup, proxy, true);
            }

            foreach (var metadataName in new[] { FusionPaths.InstalledFilesName, "update-source.txt" })
            {
                var metadataPath = Path.Combine(dataDir, metadataName);
                if (File.Exists(metadataPath)) File.Delete(metadataPath);
            }
            if (Directory.Exists(dataDir) && !Directory.EnumerateFileSystemEntries(dataDir).Any())
                Directory.Delete(dataDir);

            var manager = Path.Combine(root, "Fusion.Manager.exe");
            var legacyTools = new[] { Path.Combine(root, "Fusion.Uninstaller.exe"), Path.Combine(root, "Fusion.Updater.exe") };
            var deleteCommands = string.Join(" & ", legacyTools.Append(manager)
                .Select(path => $"if exist \"{path}\" del /f /q \"{path}\""));
            var command = $"/c timeout /t 2 /nobreak > nul & {deleteCommands}";
            Process.Start(new ProcessStartInfo("cmd.exe", command) { CreateNoWindow = true, UseShellExecute = false });
            StatusText.Text = "Fusion files were removed. The manager will close and delete itself; Geometry Dash and unrelated files were preserved.";
            MessageBox.Show("Fusion has been removed. Geometry Dash files were left untouched.", "Fusion Manager",
                MessageBoxButton.OK, MessageBoxImage.Information);
            Close();
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Uninstall failed: {ex.Message}";
        }
    }
}