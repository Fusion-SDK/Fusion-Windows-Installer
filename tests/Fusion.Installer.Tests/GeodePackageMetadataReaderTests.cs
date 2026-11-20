using System.IO.Compression;
using System.Text;
using Fusion.Installer.Services;
using Xunit;

namespace Fusion.Installer.Tests;

public sealed class GeodePackageMetadataReaderTests
{
    [Fact]
    public void TryRead_ParsesGeodeManifestWithoutExtractingPackage()
    {
        var path = CreatePackage("""
            {
              "geode": "5.10.1",
              "gd": { "win": "2.2081", "android": "2.2081" },
              "id": "sample.mod",
              "name": "Sample Mod",
              "version": "v1.2.3",
              "developer": "Fusion Test",
              "description": "A test package",
              "dependencies": { "geode.node-ids": ">=1.20.0" }
            }
            """);
        try
        {
            var success = new GeodePackageMetadataReader().TryRead(path, out var metadata, out var error);
            Assert.True(success, error);
            Assert.NotNull(metadata);
            Assert.Equal("sample.mod", metadata.Id);
            Assert.Equal("Sample Mod", metadata.Name);
            Assert.Equal("v1.2.3", metadata.Version);
            Assert.Equal("5.10.1", metadata.GeodeVersion);
            Assert.Equal("2.2081", metadata.GameVersions["win"]);
            Assert.Equal("Fusion Test", metadata.Developer);
            Assert.Equal(">=1.20.0", metadata.Dependencies["geode.node-ids"]);
        }
        finally { File.Delete(path); }
    }

    [Fact]
    public void TryRead_RejectsNonGeodeExtension()
    {
        var path = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N") + ".zip");
        File.WriteAllText(path, "not a package");
        try
        {
            Assert.False(new GeodePackageMetadataReader().TryRead(path, out _, out var error));
            Assert.Contains(".geode", error);
        }
        finally { File.Delete(path); }
    }

    [Fact]
    public void TryRead_RejectsPackageWithoutRootManifest()
    {
        var path = CreatePackage(null, manifestPath: "nested/mod.json");
        try
        {
            Assert.False(new GeodePackageMetadataReader().TryRead(path, out _, out var error));
            Assert.Contains("root-level mod.json", error);
        }
        finally { File.Delete(path); }
    }

    [Fact]
    public void TryRead_RejectsUnsafeModId()
    {
        var path = CreatePackage("""
            { "geode": "5.10.1", "gd": { "win": "2.2081" },
              "id": "../outside", "name": "Unsafe", "version": "1.0" }
            """);
        try
        {
            Assert.False(new GeodePackageMetadataReader().TryRead(path, out _, out var error));
            Assert.Contains("invalid mod ID", error);
        }
        finally { File.Delete(path); }
    }

    [Fact]
    public void TryRead_RejectsMalformedManifestJson()
    {
        var path = CreatePackage("{ invalid json }");
        try
        {
            Assert.False(new GeodePackageMetadataReader().TryRead(path, out _, out var error));
            Assert.Contains("not valid JSON", error);
        }
        finally { File.Delete(path); }
    }

    private static string CreatePackage(string? manifest, string manifestPath = "mod.json")
    {
        var path = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString("N") + ".geode");
        using var archive = ZipFile.Open(path, ZipArchiveMode.Create);
        if (manifest is not null)
        {
            var entry = archive.CreateEntry(manifestPath);
            using var writer = new StreamWriter(entry.Open(), new UTF8Encoding(false));
            writer.Write(manifest);
        }
        return path;
    }
}
