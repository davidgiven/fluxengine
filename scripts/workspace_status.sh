#!/bin/sh
# Workspace status for Bazel stamping — invoked via
#   bazel build --stamp --workspace_status_command="bash scripts/workspace_status.sh"
# Uses bash from Git on Windows (C:\Program Files\Git\bin\bash.exe) as well as
# Linux/macOS. Produces STABLE_VERSION for jpackage and -Dfluxengine.version.
#
# Format YY.MM.N (per-minute UTC) where N is minutes since start of month:
#   N = (DD - 1) * 1440 + HH * 60 + MM, 0 <= N <= 44639
# e.g. 26.09.19 14:30 UTC -> N = 18*1440 + 14*60 + 30 = 26790 -> 26.9.26790
# Three components satisfy macOS CFBundleVersion (1-3 integers, 18 chars,
# jpackage on JDK 21 rejects >3), MSI ProductVersion 255.255.65535 (first
# <256, second <256, third <65535), Debian upstream_version and RPM Version,
# while preserving per-minute monotonicity.
YY=$(date -u +%y)
MM=$(date -u +%m)
DD=$(date -u +%d)
HH=$(date -u +%H)
MIN=$(date -u +%M)
N=$(( (10#$DD - 1) * 1440 + 10#$HH * 60 + 10#$MIN ))
echo "STABLE_VERSION $((10#$YY)).$((10#$MM)).$N"
echo "BUILD_TIMESTAMP $(date -u +%Y-%m-%dT%H:%M:%SZ)"
