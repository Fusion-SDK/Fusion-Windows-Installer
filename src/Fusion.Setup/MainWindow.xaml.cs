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
            : "Geometry Dash detected. Fusion will install alongside the game without replacing original game files.";
    }
    private void DefaultPath_Click(object sender, RoutedEventArgs e) => InstallPathBox.Text = FindGeometryDash() ?? "";
    private void Cancel_Click(object sender, RoutedEventArgs e) => Close();
    private async void Install_Click(object sender, RoutedEventArgs e)
    {
        var target = InstallPathBox.Text.Trim();
        if (!File.Exists(Path.Combine(target, "GeometryDash.exe"))) { StatusText.Text = "Select the Geometry Dash folder containing GeometryDash.exe."; return; }
        var payload = Path.Combine(AppContext.BaseDirectory, "payload");
        if (!Directory.Exists(payload)) { StatusText.Text = "Fusion payload is missing. Rebuild the setup package."; return; }
        var required = new[] { "Fusion.Updater.exe", "Fusion.Uninstaller.exe", Path.Combine("Fusion", "Fusion.Loader.dll") };
        var missing = required.Where(x => !File.Exists(Path.Combine(payload, x))).ToArray();
        if (missing.Length > 0) { StatusText.Text = "Package is incomplete: " + string.Join(", ", missing); return; }
        try
        {
            var fusionDir = Path.Combine(target, "Fusion"); Directory.CreateDirectory(fusionDir);
            var installed = new List<string>();
            foreach (var file in Directory.GetFiles(payload, "*", SearchOption.AllDirectories))
            {
                var relative = Path.GetRelativePath(payload, file);
                var normalized = relative.Replace('\\', '/');
                if (!normalized.StartsWith("Fusion/", StringComparison.OrdinalIgnoreCase))
                    continue; // Root-level updater and uninstaller are copied separately below.
                var destination = Path.GetFullPath(Path.Combine(target, normalized.Replace('/', Path.DirectorySeparatorChar)));
                var safeRoot = Path.GetFullPath(fusionDir) + Path.DirectorySeparatorChar;
                if (!destination.StartsWith(safeRoot, StringComparison.OrdinalIgnoreCase)) throw new InvalidDataException("Invalid package path.");
                Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
                await Task.Run(() => File.Copy(file, destination, true));
                installed.Add(normalized);
            }
            foreach (var name in new[] { "Fusion.Updater.exe", "Fusion.Uninstaller.exe" })
            {
                var source = Path.Combine(payload, name); var destination = Path.Combine(target, name);
                await Task.Run(() => File.Copy(source, destination, true));
            }
            // Record only Fusion-owned files. Never write to GeometryDash.exe or other game-owned paths.
            await File.WriteAllTextAsync(Path.Combine(fusionDir, "installed-files.json"), System.Text.Json.JsonSerializer.Serialize(installed, new System.Text.Json.JsonSerializerOptions { WriteIndented = true }));
            StatusText.Text = "Fusion components installed. Loader integration is not yet active; the game will launch normally.";
            StepTitle.Text = "Installation complete";
            StepDescription.Text = "Fusion's management tools are installed in the Geometry Dash folder.";
            MessageBox.Show("Fusion management components were installed. The Geometry Dash loader is not active in this build.", "Fusion Setup", MessageBoxButton.OK, MessageBoxImage.Information);
        }
        catch (Exception ex) { StatusText.Text = $"Installation failed: {ex.Message}"; }
    }
    private static string? FindGeometryDash()
    {
        var roots = new[] { @"C:\Program Files (x86)\Steam", @"C:\Program Files\Steam" };
        foreach (var steam in roots.Where(Directory.Exists))
        {
            var libs = new List<string> { steam }; var vdf = Path.Combine(steam, "steamapps", "libraryfolders.vdf");
            if (File.Exists(vdf)) foreach (var line in File.ReadLines(vdf))
            {
                var parts = line.Trim().Split('"');
                if (parts.Length >= 4 && parts[1] == "path") { var path = parts[3].Replace("\\\\", "\\"); if (Directory.Exists(path)) libs.Add(path); }
            }
            foreach (var lib in libs.Distinct(StringComparer.OrdinalIgnoreCase))
            { var game = Path.Combine(lib, "steamapps", "common", "Geometry Dash"); if (File.Exists(Path.Combine(game, "GeometryDash.exe"))) return game; }
        }
        return null;
    }
}
