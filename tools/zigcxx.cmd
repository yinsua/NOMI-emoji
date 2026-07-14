@echo off
"%~dp0..\.tools\ziglang\zig.exe" c++ -target x86_64-windows-gnu %*

