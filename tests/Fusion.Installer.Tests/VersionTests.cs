using Xunit;

namespace Fusion.Installer.Tests;

public class VersionTests
{
    [Theory]
    [InlineData("1.0.0", true)]
    [InlineData("0.1.0", false)]
    public void VersionParsingWorks(string value, bool expected)
    {
        var parsed = Version.TryParse(value, out var version);
        Assert.Equal(expected, parsed);
        if (expected) Assert.NotNull(version);
    }
}
