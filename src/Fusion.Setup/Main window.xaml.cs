using System.Diagnostics;
using System.IO;
using System.Windows;

namespace Fusion.Setup;

public partial class MainWindow : Window
{
    private readonly string _defaultPath = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "Programs", "Fusion Installer");

    public MainWindow()
    {
        InitializeComponent();
        InstallPathBox.Text = _defaultPath;
        StatusText.Text = "Fusion Setup will install for the current Windows user. No administrator access is required.";
    }

    private void DefaultPath_Click(object sender, RoutedEventArgs e) =>
        InstallPathBox.Text = _defaultPath;

    private void Cancel_Click(object sender, RoutedEventArgs e) => Close();

    private async void Install_Click(object sender, RoutedEventArgs e)
    {
        var target = InstallPathBox.Text.Trim();
        if (string.IsNullOrWhiteSpace(target))
        {
            StatusText.Text = "Choose a valid installation folder.";
            return;
        }

        var payload = Path.Combine(AppContext.BaseDirectory, "payload");
        if (!Directory.Exists(payload) || !Directory.EnumerateFiles(payload, "*.exe", SearchOption.AllDirectories).Any())
        {
            StatusText.Text = "Installer payload is missing. Build with scripts/build-windows.ps1 first.";
            return;
        }

        try
        {
            Directory.CreateDirectory(target);
            await Task.Run(() => CopyDirectory(payload, target));
            CreateStartMenuShortcut(target);
            StatusText.Text = "Fusion Installer was installed successfully.";
            StepTitle.Text = "Installation complete";
            StepDescription.Text = "Fusion Installer is ready. You can launch it from your Start Menu.";
            MessageBox.Show("Fusion Installer has been installed.", "Fusion Setup",
                MessageBoxButton.OK, MessageBoxImage.Information);
            Close();
        }
        catch (Exception ex)
        {
            StatusText.Text = $"Installation failed: {ex.Message}";
        }
    }

    private static void CopyDirectory(string source, string destination)
    {
        Directory.CreateDirectory(destination);
        foreach (var file in Directory.GetFiles(source))
            File.Copy(file, Path.Combine(destination, Path.GetFileName(file)), true);
        foreach (var directory in Directory.GetDirectories(source))
            CopyDirectory(directory, Path.Combine(destination, Path.GetFileName(directory)));
    }

    private static void CreateStartMenuShortcut(string installPath)
    {
        var exe = Path.Combine(installPath, "Fusion.Installer.exe");
        if (!File.Exists(exe)) return;

        var startMenu = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.StartMenu),
            "Programs", "Fusion Circle");
        Directory.CreateDirectory(startMenu);
        var shortcut = Path.Combine(startMenu, "Fusion Installer.url");
        File.WriteAllText(shortcut, $"[InternetShortcut]{Environment.NewLine}URL=file:///{exe.Replace('\\', '/')}{Environment.NewLine}");
    }
}
