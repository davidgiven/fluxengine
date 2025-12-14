import platform
import os

if os.getenv("BUILDTYPE") == "windows":
    windows = True
    osx = False
    unix = False
else:
    windows = False
    osx = platform.system() == "Darwin"
    unix = True
