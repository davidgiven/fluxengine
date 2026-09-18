@echo off
set BAZEL_SH=c:\msys64\usr\bin\bash.exe
bazelisk --output_user_root=C:/tmp/bazelcache %*
