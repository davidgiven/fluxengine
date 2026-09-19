#!/bin/sh
# Workspace status for Bazel stamping — invoked via
#   bazel build --stamp --workspace_status_command="bash scripts/workspace_status.sh"
# Uses bash from Git on Windows (C:\Program Files\Git\bin\bash.exe) as well as
# Linux/macOS. Produces STABLE_VERSION for jpackage and -Dfluxengine.version.
#
# Format YY.MM.DD.HHMM (per-minute, UTC) e.g. 26.09.19.1430
# Each dotted component fits MSI ProductVersion 255.255.65535.65535
# (fourth field ignored for MSI upgrades but must be <65535),
# Debian upstream_version and RPM Version. macOS CFBundleVersion
# ignores anything after the third component, so minute granularity
# is lost on macOS but the package still installs correctly.
echo "STABLE_VERSION $(date -u +%y.%m.%d.%H%M)"
echo "BUILD_TIMESTAMP $(date -u +%Y-%m-%dT%H:%M:%SZ)"
