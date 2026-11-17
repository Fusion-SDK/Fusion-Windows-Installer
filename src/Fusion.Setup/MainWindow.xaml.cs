using System.IO;
using System.Windows;

namespace Fusion.Setup;

public partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();
        var detected = FindGeometryDash();
        InstallPathBox.Text = detected ?? "";
        StatusText.Text = detected is null
            ? "Geometry Dash was not found automatically. Select the folder containing GeometryDash.exe."
            : "Geometry Dash detected. Fusion will install its bootstrap without replacing original game files.";
    }

    private void DefaultPath_Click(object sender, RoutedEventArgs e) =>
        InstallPathBox.Text = FindGeometryDash() ?? "";

    private void Cancel_Click(object sender, RoutedEventArgs e) => Close();

    private async void Install_Click(object sender, RoutedEventArgs e)
    {
        var target = InstallPathBox.Text.Trim();
        if (!File.Exists(Path.Combine(target, "GeometryDash.exe")))
        {
            StatusText.Text = "Select the Geometry Dash folder containing GeometryDash.exe.";
            return;
        }

        var payload = Path.Combine(AppContext.BaseDirectory, "payload");
        if (!Directory.Exists(payload))
        {
            StatusText.Text = "Fusion payload is missing. Rebuild the setup package.";
            return;
        }

        var required = new[]
        {
            "XInput1_4.dll",
            "Fusion.Updater.exe",
            "Fusion.Uninstaller.exe",
            Path.Combine("Fusion", "Fusion.Loader.dll"),
            Path.Combine("Fusion", "FusionBootstrap.dll")
        };
        var missing = required.Where(x => !File.Exists(Path.Combine(payload, x))).ToArray();
        if (missing.Length > 0)
        {
            StatusText.Text = "Package is incomplete: " + string.Join(", ", missing);
            return;
        }

        try
        {
            var fusionDir = Path.Combine(target, "Fusion");
            Directory.CreateDirectory(fusionDir);
            Directory.CreateDirectory(Path.Combine(fusionDir, "mods"));
            var installed = new List<string>();
            var previousManifest = Path.Combine(fusionDir, "installed-files.json");
            var wasFusionProxyInstalled = false;
            if (File.Exists(previousManifest))
            {
                try
                {
                    var previousFiles = System.Text.Json.JsonSerializer.Deserialize<List<string>>(
                        await File.ReadAllTextAsync(previousManifest)) ?? [];
                    wasFusionProxyInstalled = previousFiles.Contains("XInput1_4.dll", StringComparer.OrdinalIgnoreCase);
                }
                catch { /* A malformed old manifest must not block a fresh install. */ }
            }

            var proxyPath = Path.Combine(target, "XInput1_4.dll");
            var proxyBackup = Path.Combine(target, "XInput1_4.dll.fusion-backup");
            if (File.Exists(proxyPath) && !wasFusionProxyInstalled && !File.Exists(proxyBackup))
            {
                File.Copy(proxyPath, proxyBackup, false);
                installed.Add("XInput1_4.dll.fusion-backup");
            }

            foreach (var file in Directory.GetFiles(payload, "*", SearchOption.AllDirectories))
            {
                var relative = Path.GetRelativePath(payload, file);
                var normalized = relative.Replace('\\', '/');
                var destination = Path.GetFullPath(Path.Combine(target,
                    normalized.Replace('/', Path.DirectorySeparatorChar)));

                if (normalized.StartsWith("Fusion/", StringComparison.OrdinalIgnoreCase))
                {
                    var safeRoot = Path.GetFullPath(fusionDir) + Path.DirectorySeparatorChar;
                    if (!destination.StartsWith(safeRoot, StringComparison.OrdinalIgnoreCase))
                        throw new InvalidDataException("Invalid package path.");
                }
                else if (normalized is not "XInput1_4.dll" and not "Fusion.Updater.exe" and not "Fusion.Uninstaller.exe")
                {
                    continue;
                }

                Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
                await Task.Run(() => File.Copy(file, destination, true));
                installed.Add(normalized);
            }

            // Record the proxy and Fusion-owned files so uninstall can restore a
            // pre-existing XInput proxy rather than leaving the game without it.
            await File.WriteAllTextAsync(
                Path.Combine(fusionDir, "installed-files.json"),
                System.Text.Json.JsonSerializer.Serialize(installed,
                    new System.Text.Json.JsonSerializerOptions { WriteIndented = true }));

            StatusText.Text = "Fusion's startup proxy is installed. Launch Geometry Dash normally through Steam.";
            StepTitle.Text = "Installation complete";
            StepDescription.Text = "Fusion is configured to load inside Geometry Dash when you press Play in Steam.";
            MessageBox.Show(
                "Fusion's XInput startup proxy and native bootstrap were installed. Launch Geometry Dash normally through Steam. The in-game menu and version-specific game API are still under development.",
                "Fusion Setup", MessageBoxButton.OK, MessageBoxImage.Information);
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Installation failed: {ex.Message}";
        }
    }

    private static string? FindGeometryDash()
    {
        var roots = new[] { @"C:\Program Files (x86)\Steam", @"C:\Program Files\Steam" };
        foreach (var steam in roots.Where(Directory.Exists))
        {
            var libraries = new List<string> { steam };
            var vdf = Path.Combine(steam, "steamapps", "libraryfolders.vdf");
            if (File.Exists(vdf))
            {
                foreach (var line in File.ReadLines(vdf))
                {
                    var parts = line.Trim().Split('"');
                    if (parts.Length >= 4 && parts[1] == "path")
                    {
                        var path = parts[3].Replace("\\\\", "\\");
                        if (Directory.Exists(path)) libraries.Add(path);
                    }
                }
            }

            foreach (var library in libraries.Distinct(StringComparer.OrdinalIgnoreCase))
            {
                var game = Path.Combine(library, "steamapps", "common", "Geometry Dash");
                if (File.Exists(Path.Combine(game, "GeometryDash.exe"))) return game;
            }
        }
        return null;
    }
}
