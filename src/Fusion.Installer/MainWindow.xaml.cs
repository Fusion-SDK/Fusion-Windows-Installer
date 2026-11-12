using System.Diagnostics;
using System.Windows;
using System.Windows.Controls;
using Fusion.Installer.Models;
using Fusion.Installer.Services;

namespace Fusion.Installer;

public partial class MainWindow : Window
{
    private readonly SupabaseService _supabase = new();
    private readonly GameDetectionService _gameDetection = new();
    private string? _accessToken;
    private string? _userEmail;

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
            MessageBox.Show(result is null
                ? "Geometry Dash was not found in the common Steam library locations. You can add folder selection later."
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
        buttons.Children.Add(signIn);
        buttons.Children.Add(register);
        ContentPanel.Children.Add(buttons);

        signIn.Click += async (_, _) =>
        {
            status.Text = "Signing in...";
            try
            {
                var session = await _supabase.SignInAsync(email.Text.Trim(), password.Password);
                _accessToken = session.AccessToken;
                _userEmail = session.Email;
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
            var mods = await _supabase.GetApprovedModsAsync(_accessToken);
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

    private void ShowPlaceholder(string title, string subtitle, string body)
    {
        SetPage(title, subtitle);
        AddBody(body);
    }

    private void Overview_Click(object sender, RoutedEventArgs e) => ShowOverview();
    private async void Mods_Click(object sender, RoutedEventArgs e) => await ShowModsAsync();
    private void MyMods_Click(object sender, RoutedEventArgs e) =>
        ShowPlaceholder("My Mods", "Your installed mod library.", "Mod installation is not enabled in this starter build. It will be connected after Fusion Loader and signed package support are ready.");
    private void Games_Click(object sender, RoutedEventArgs e) =>
        ShowPlaceholder("Games", "Manage supported game installations.", "The first target is Geometry Dash on Steam. Other editions and platforms can be added after the Steam version is tested.");
    private void Settings_Click(object sender, RoutedEventArgs e) =>
        ShowPlaceholder("Settings", "Application preferences.", "Settings and cloud preferences will be expanded as the installer develops.");
    private void ShowAuth_Click(object sender, RoutedEventArgs e) => ShowAuth();
}
