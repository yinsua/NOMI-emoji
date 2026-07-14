@echo off
"%~dp0..\.tools\ziglang\zig.exe" cc -target x86_64-windows-gnu %*

