Run these from the repository root in a Visual Studio 2019 developer shell after preparing the main project's dependencies:

```powershell
msbuild KApp/Tests/KPixelFormatTests.vcxproj /p:Configuration=Debug /p:Platform=x64
& KApp/Tests/bin/Debug/KPixelFormatTests.exe
msbuild KApp/Tests/KPixelFormatTests.vcxproj /p:Configuration=Release /p:Platform=x64
& KApp/Tests/bin/Release/KPixelFormatTests.exe
```

The standalone executable tests pixel sizes, compression blocks, mip chains, volume/cube/array sizes, overflow rejection, DDS hardware passthrough and software decoding, and saving RGBA8 sRGB images. It generates DDS samples in memory and removes its temporary PNG after a successful save.
