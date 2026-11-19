using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;
using Microsoft.Win32;
using Fusion.Installer.Models;
using Fusion.Installer.Services;

namespace Fusion.Installer;

public partial class MainWindow : Window
{
    private readonly SupabaseService _supabase = new();
    private readonly GameDetectionService _gameDetection = new();
    private string? _accessToken;
    private string? _userEmail;
    private string? _refreshToken;
    private DateTimeOffset? _expiresAt;
    private GameInstall? _selectedGameInstall;

    public MainWindow()
    {
        InitializeComponent();
        ShowOverview();
        ConnectionLabel.Text = _supabase.IsConfigured
            ? "Supabase configured"
            : "Supabase not configured";
    }

    private void SetPage(string title, string subtitle)
    {
        PageTitle.Text = title;
        PageSubtitle.Text = subtitle;
        ContentPanel.Children.Clear();
    }

    private void AddHeading(string text, double size = 20)
    {
        ContentPanel.Children.Add(new TextBlock
        {
            Text = text, FontSize = size, FontWeight = FontWeights.SemiBold,
            Margin = new Thickness(0, 12, 0, 8)
        });
    }

    private void AddBody(string text)
    {
        ContentPanel.Children.Add(new TextBlock
        {
            Text = text, TextWrapping = TextWrapping.Wrap,
            Foreground = (System.Windows.Media.Brush)FindResource("FusionMutedBrush"),
            Margin = new Thickness(0, 0, 0, 12)
        });
    }

    private void ShowOverview()
    {
        SetPage("Overview", "Your Geometry Dash modding hub.");
        AddHeading("Welcome to Fusion");
        AddBody("Discover community mods, manage your library, and keep your Fusion setup organized.");
        AddHeading("Getting started");
        AddBody("1. Sign in with your Fusion Circle account.\n2. Select your Geometry Dash Steam installation.\n3. Browse approved mods.\n4. Install mods once a compatible, verified Fusion Loader is available.");
        AddHeading("Runtime status");
        AddBody("Fusion Loader integration is not included in this starter build. Mod activation remains disabled until the loader, package format, and signature verification are ready.");
        var button = new Button { Content = "Check for Geometry Dash", Width = 220, HorizontalAlignment = HorizontalAlignment.Left };
        button.Click += async (_, _) =>
        {
            var result = await _gameDetection.DetectAsync();
            if (result is not null)
                _selectedGameInstall = result;

            MessageBox.Show(result is null
                ? "Geometry Dash was not found automatically. Open Games to select its folder manually."
                : $"Geometry Dash detected:\n{result.GameDirectory}", "Fusion Game Detection",
                MessageBoxButton.OK, MessageBoxImage.Information);
        };
        ContentPanel.Children.Add(button);
    }

    private void ShowAuth()
    {
        SetPage("Fusion Account", "Sign in or create your Fusion Circle account.");
        AddHeading("Email");
        var email = new TextBox { MaxWidth = 420, HorizontalAlignment = HorizontalAlignment.Left };
        ContentPanel.Children.Add(email);
        AddHeading("Password");
        var password = new PasswordBox { MaxWidth = 420, HorizontalAlignment = HorizontalAlignment.Left };
        ContentPanel.Children.Add(password);
        var status = new TextBlock { TextWrapping = TextWrapping.Wrap, Margin = new Thickness(0, 8, 0, 8) };
        ContentPanel.Children.Add(status);

        var buttons = new StackPanel { Orientation = Orientation.Horizontal };
        var signIn = new Button { Content = "Sign in" };
        var register = new Button { Content = "Create account" };
        var signOut = new Button
        {
            Content = "Sign out",
            IsEnabled = !string.IsNullOrWhiteSpace(_accessToken)
        };
        buttons.Children.Add(signIn);
        buttons.Children.Add(register);
        buttons.Children.Add(signOut);
        ContentPanel.Children.Add(buttons);

        signIn.Click += async (_, _) =>
        {
            status.Text = "Signing in...";
            try
            {
                var session = await _supabase.SignInAsync(email.Text.Trim(), password.Password);
                StoreSession(session);
                password.Clear();
                signOut.IsEnabled = true;
                status.Text = $"Signed in as {_userEmail}.";
                ConnectionLabel.Text = $"Signed in: {_userEmail}";
            }
            catch (Exception ex) { status.Text = ex.Message; }
        };

        register.Click += async (_, _) =>
        {
            status.Text = "Creating account...";
            try
            {
                var message = await _supabase.RegisterAsync(email.Text.Trim(), password.Password);
                status.Text = message;
            }
            catch (Exception ex) { status.Text = ex.Message; }
        };

        signOut.Click += async (_, _) =>
        {
            if (string.IsNullOrWhiteSpace(_accessToken))
                return;

            signOut.IsEnabled = false;
            status.Text = "Signing out...";
            try
            {
                var accessToken = await GetValidAccessTokenAsync();
                await _supabase.SignOutAsync(accessToken);
                ClearSession();
                status.Text = "You have been signed out.";
                ConnectionLabel.Text = _supabase.IsConfigured
                    ? "Supabase configured"
                    : "Supabase not configured";
            }
            catch (Exception ex)
            {
                status.Text = ex.Message;
                signOut.IsEnabled = true;
            }
        };
    }

    private async Task ShowModsAsync()
    {
        SetPage("Mod Browser", "Approved community mods from the Fusion catalog.");
        if (string.IsNullOrWhiteSpace(_accessToken))
        {
            AddBody("Sign in before browsing the mod catalog.");
            ContentPanel.Children.Add(new Button { Content = "Sign in", Width = 140, HorizontalAlignment = HorizontalAlignment.Left });
            return;
        }

        AddBody("Loading approved mods...");
        try
        {
            var accessToken = await GetValidAccessTokenAsync();
            var mods = await _supabase.GetApprovedModsAsync(accessToken);
            ContentPanel.Children.Clear();
            if (mods.Count == 0) AddBody("No approved mods are published yet.");
            foreach (var mod in mods)
            {
                var card = new Border
                {
                    Background = (System.Windows.Media.Brush)FindResource("FusionSurfaceBrush"),
                    CornerRadius = new CornerRadius(10),
                    Padding = new Thickness(16),
                    Margin = new Thickness(0, 0, 0, 12)
                };
                var stack = new StackPanel();
                stack.Children.Add(new TextBlock { Text = mod.Name, FontSize = 18, FontWeight = FontWeights.SemiBold });
                stack.Children.Add(new TextBlock { Text = mod.Description, TextWrapping = TextWrapping.Wrap, Foreground = (System.Windows.Media.Brush)FindResource("FusionMutedBrush"), Margin = new Thickness(0, 6, 0, 0) });
                stack.Children.Add(new TextBlock { Text = $"Version {mod.LatestVersion ?? "Unspecified"} · {mod.Author ?? "Unknown author"}", Foreground = (System.Windows.Media.Brush)FindResource("FusionMutedBrush"), Margin = new Thickness(0, 8, 0, 0) });
                card.Child = stack;
                ContentPanel.Children.Add(card);
            }
        }
        catch (Exception ex)
        {
            ContentPanel.Children.Clear();
            AddBody($"Could not load the catalog: {ex.Message}");
        }
    }


    private void StoreSession(AuthSession session)
    {
        _accessToken = session.AccessToken;
        _refreshToken = session.RefreshToken;
        _expiresAt = session.ExpiresAt;
        _userEmail = session.Email;
    }

    private void ClearSession()
    {
        _accessToken = null;
        _refreshToken = null;
        _expiresAt = null;
        _userEmail = null;
    }

    private async Task<string> GetValidAccessTokenAsync()
    {
        if (string.IsNullOrWhiteSpace(_accessToken))
            throw new InvalidOperationException("Sign in before accessing the catalog.");

        if (_expiresAt is not null &&
            _expiresAt <= DateTimeOffset.UtcNow.AddMinutes(1))
        {
            if (string.IsNullOrWhiteSpace(_refreshToken))
                throw new InvalidOperationException("Your session expired. Sign in again.");

            var refreshed = await _supabase.RefreshSessionAsync(_refreshToken);
            StoreSession(refreshed);
            ConnectionLabel.Text = $"Signed in: {_userEmail}";
        }

        return _accessToken!;
    }

    private void ShowPlaceholder(string title, string subtitle, string body)
    {
        SetPage(title, subtitle);
        AddBody(body);
    }

    private void Overview_Click(object sender, RoutedEventArgs e) => ShowOverview();
    private async void Mods_Click(object sender, RoutedEventArgs e) => await ShowModsAsync();
    private void MyMods_Click(object sender, RoutedEventArgs e) =>
        ShowPlaceholder("My Mods", "Your installed mod library.", "Mod installation is not enabled in this starter build. It will be connected after Fusion Loader and signed package support are ready.");
    private void Games_Click(object sender, RoutedEventArgs e) => ShowGames();

    private void ShowGames()
    {
        SetPage("Games", "Manage the Geometry Dash installation used by Fusion.");
        AddHeading("Geometry Dash (Steam)");
        AddBody("Fusion currently targets the Windows Steam edition. Select the folder that contains GeometryDash.exe. This only records the game location; it does not install or modify game files.");

        var pathBox = new TextBox
        {
            Text = _selectedGameInstall?.GameDirectory ?? string.Empty,
            IsReadOnly = true,
            MinWidth = 420,
            MaxWidth = 680,
            Padding = new Thickness(10),
            Margin = new Thickness(0, 0, 0, 12)
        };
        ContentPanel.Children.Add(pathBox);

        var status = new TextBlock
        {
            Text = _selectedGameInstall is null
                ? "No Geometry Dash installation selected."
                : "Geometry Dash installation selected.",
            TextWrapping = TextWrapping.Wrap,
            Margin = new Thickness(0, 0, 0, 12),
            Foreground = (System.Windows.Media.Brush)FindResource("FusionMutedBrush")
        };
        ContentPanel.Children.Add(status);

        var actions = new StackPanel { Orientation = Orientation.Horizontal };
        var detect = new Button { Content = "Detect Steam install" };
        var browse = new Button { Content = "Browse folder" };
        var open = new Button
        {
            Content = "Open folder",
            IsEnabled = _selectedGameInstall is not null
        };
        actions.Children.Add(detect);
        actions.Children.Add(browse);
        actions.Children.Add(open);
        ContentPanel.Children.Add(actions);

        detect.Click += async (_, _) =>
        {
            detect.IsEnabled = false;
            status.Text = "Searching Steam libraries...";
            try
            {
                var found = await _gameDetection.DetectAsync();
                if (found is null)
                {
                    status.Text = "Geometry Dash was not found in the detected Steam libraries. Use Browse folder to select it manually.";
                    return;
                }

                _selectedGameInstall = found;
                pathBox.Text = found.GameDirectory;
                status.Text = "Geometry Dash detected and selected.";
                open.IsEnabled = true;
            }
            catch (Exception ex)
            {
                status.Text = $"Game detection failed: {ex.Message}";
            }
            finally
            {
                detect.IsEnabled = true;
            }
        };

        browse.Click += (_, _) =>
        {
            var dialog = new OpenFolderDialog
            {
                Title = "Select the Geometry Dash folder",
                Multiselect = false
            };

            if (_selectedGameInstall is not null)
                dialog.InitialDirectory = _selectedGameInstall.GameDirectory;

            if (dialog.ShowDialog(this) != true)
                return;

            var selected = _gameDetection.TryGetGameInstall(dialog.FolderName);
            if (selected is null)
            {
                status.Text = "That folder does not contain GeometryDash.exe. Select the actual Geometry Dash game folder.";
                return;
            }

            _selectedGameInstall = selected;
            pathBox.Text = selected.GameDirectory;
            status.Text = "Geometry Dash installation selected.";
            open.IsEnabled = true;
        };

        open.Click += (_, _) =>
        {
            if (_selectedGameInstall is null)
                return;

            Process.Start(new ProcessStartInfo
            {
                FileName = _selectedGameInstall.GameDirectory,
                UseShellExecute = true
            });
        };
    }
    private void Settings_Click(object sender, RoutedEventArgs e) =>
        ShowPlaceholder("Settings", "Application preferences.", "Settings and cloud preferences will be expanded as the installer develops.");
    private void ShowAuth_Click(object sender, RoutedEventArgs e) => ShowAuth();
}
