using System.IO;
using System.Diagnostics;
using System.Text.Json;
using System.Windows;
using Fusion.Shared;
namespace Fusion.Uninstaller;
public partial class MainWindow : Window
{
    public MainWindow() { InitializeComponent(); GamePathBox.Text = AppContext.BaseDirectory; StatusText.Text = "Only Fusion-owned files are eligible for removal. Geometry Dash files are never targeted."; }
    private void Detect_Click(object sender, RoutedEventArgs e)
    {
        var candidate = AppContext.BaseDirectory.TrimEnd(Path.DirectorySeparatorChar);
        if (File.Exists(Path.Combine(candidate, "GeometryDash.exe"))) { GamePathBox.Text = candidate; StatusText.Text = "Geometry Dash folder detected."; }
        else StatusText.Text = "The manager is located outside a Geometry Dash folder. Paste the folder containing GeometryDash.exe.";
    }
    private void Remove_Click(object sender, RoutedEventArgs e)
    {
        var root = GamePathBox.Text.Trim();
        if (!File.Exists(Path.Combine(root, "GeometryDash.exe"))) { StatusText.Text = "Choose the folder containing GeometryDash.exe."; return; }
        if (MessageBox.Show("Remove Fusion's managed files from this folder? Original Geometry Dash files will be kept.", "Remove Fusion", MessageBoxButton.YesNo, MessageBoxImage.Warning) != MessageBoxResult.Yes) return;
        try
        {
            var dataDir = Path.Combine(root, FusionPaths.DataDirectoryName);
            var manifest = Path.Combine(dataDir, FusionPaths.InstalledFilesName);
            if (File.Exists(manifest))
            {
                var files = JsonSerializer.Deserialize<List<string>>(File.ReadAllText(manifest)) ?? [];
                foreach (var relative in files)
                {
                    var path = FusionPaths.SafeCombine(root, relative);
                    if (File.Exists(path)) File.Delete(path);
                }
            }
            foreach (var metadataName in new[] { FusionPaths.InstalledFilesName, "update-source.txt" })
            {
                var metadataPath = Path.Combine(dataDir, metadataName);
                if (File.Exists(metadataPath)) File.Delete(metadataPath);
            }
            // Leave unknown files (including user mods/configuration) untouched.
            if (Directory.Exists(dataDir) && !Directory.EnumerateFileSystemEntries(dataDir).Any())
                Directory.Delete(dataDir);
            // Windows locks the running uninstaller executable. Schedule both management EXEs
            // for deletion after this process exits.
            var updater = Path.Combine(root, "Fusion.Updater.exe");
            var uninstaller = Path.Combine(root, "Fusion.Uninstaller.exe");
            var command = $"/c timeout /t 2 /nobreak > nul & if exist \"{updater}\" del /f /q \"{updater}\" & if exist \"{uninstaller}\" del /f /q \"{uninstaller}\"";
            Process.Start(new ProcessStartInfo("cmd.exe", command) { CreateNoWindow = true, UseShellExecute = false });
            StatusText.Text = "Fusion-managed files were removed. Unknown Fusion data was preserved. The management executables will be deleted when this window closes; Geometry Dash files were left untouched.";
        }
        catch (Exception ex) { StatusText.Text = $"Removal failed: {ex.Message}"; }
    }
}
