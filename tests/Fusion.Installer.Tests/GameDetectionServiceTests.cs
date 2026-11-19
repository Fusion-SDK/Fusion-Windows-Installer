using Fusion.Installer.Services;
using Xunit;

namespace Fusion.Installer.Tests;

public sealed class GameDetectionServiceTests
{
    [Fact]
    public void TryGetGameInstall_AcceptsFolderContainingGameExecutable()
    {
        var directory = CreateTemporaryDirectory();
        try
        {
            var executable = Path.Combine(directory, "GeometryDash.exe");
            File.WriteAllBytes(executable, []);

            var result = new GameDetectionService().TryGetGameInstall(directory);

            Assert.NotNull(result);
            Assert.Equal(Path.GetFullPath(directory), result.GameDirectory);
            Assert.Equal(executable, result.ExecutablePath);
        }
        finally
        {
            Directory.Delete(directory, recursive: true);
        }
    }

    [Fact]
    public void TryGetGameInstall_RejectsFolderWithoutGameExecutable()
    {
        var directory = CreateTemporaryDirectory();
        try
        {
            var result = new GameDetectionService().TryGetGameInstall(directory);

            Assert.Null(result);
        }
        finally
        {
            Directory.Delete(directory, recursive: true);
        }
    }

    [Theory]
    [InlineData(null)]
    [InlineData("")]
    [InlineData("   ")]
    public void TryGetGameInstall_RejectsEmptyPath(string? path)
    {
        var result = new GameDetectionService().TryGetGameInstall(path);

        Assert.Null(result);
    }

    [Fact]
    public void TryGetGameInstall_RejectsMissingDirectory()
    {
        var path = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N"));

        var result = new GameDetectionService().TryGetGameInstall(path);

        Assert.Null(result);
    }

    private static string CreateTemporaryDirectory()
    {
        var path = Path.Combine(Path.GetTempPath(), "FusionTests", Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(path);
        return path;
    }
}
