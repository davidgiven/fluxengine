import platform

windows = platform.system() == "Windows"
osx = platform.system() == "Darwin"
unix = not windows
