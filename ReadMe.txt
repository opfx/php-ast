php ast utilities

<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="14.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug_TS|x64'">
    <LocalDebuggerCommand>$(PhpSrcDir)\$(PlatformName)\$(Configuration)\php.exe</LocalDebuggerCommand>
    <LocalDebuggerCommandArguments>-n -dextension_dir=$(TargetDir) -dextension=$(TargetFileName) -f index.php</LocalDebuggerCommandArguments>
    <LocalDebuggerWorkingDirectory>$(ProjectDir)/php</LocalDebuggerWorkingDirectory>
    <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <LocalDebuggerCommand>$(PhpSrcDir)\$(PlatformName)\$(Configuration)\php.exe</LocalDebuggerCommand>
    <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
    <LocalDebuggerCommandArguments>-n -dextension_dir=$(TargetDir) -dextension=$(TargetFileName) -f index.php</LocalDebuggerCommandArguments>
    <LocalDebuggerWorkingDirectory>$(ProjectDir)/php</LocalDebuggerWorkingDirectory>
  </PropertyGroup>
</Project>