#!/bin/sh
# Workspace status for Bazel stamping — invoked via
#   bazel build --stamp --workspace_status_command="bash scripts/workspace_status.sh"
# Uses bash from Git on Windows (C:\Program Files\Git\bin\bash.exe) as well as
# Linux/macOS. Produces STABLE_VERSION for jpackage and -Dfluxengine.version.
#
# Format YYYY.MMDD.HHMM (per-minute, UTC) e.g. 2026.0919.1430
# Each dotted component <65535 so valid for Windows MSI ProductVersion,
# Debian upstream_version and RPM Version.
echo "STABLE_VERSION $(date -u +%Y.%m%d.%H%M)"
echo "BUILD_TIMESTAMP $(date -u +%Y-%m-%dT%H:%M:%SZ)"
