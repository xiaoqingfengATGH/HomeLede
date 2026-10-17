#!/bin/sh
# Replay after feeds update; patch only package integration, not daemon source.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
PKG="$ROOT/feeds/packages/utils/ttyd"
PATCH="$ROOT/custom/patches/ttyd-interface-rebind.patch"
[ -f "$PKG/files/ttyd.init" ] || { echo 'FAIL: ttyd package missing' >&2; exit 1; }
# A reverse dry run verifies the complete patched hunk, not just a marker.
if patch --force --fuzz=0 --dry-run -R -p1 -d "$PKG" < "$PATCH" >/dev/null 2>&1; then
    sh -n "$PKG/files/ttyd.init"
    echo 'PASS: ttyd interface rebind patch present'
    exit 0
fi
[ "${1:-}" != --check ] || { echo 'FAIL: ttyd interface rebind patch missing or changed' >&2; exit 1; }
patch --batch --fuzz=0 --dry-run --forward -p1 -d "$PKG" < "$PATCH" >/dev/null || {
    echo 'FAIL: upstream ttyd init changed; review interface rebind patch' >&2; exit 1;
}
patch --batch --fuzz=0 --forward --no-backup-if-mismatch -p1 -d "$PKG" < "$PATCH"
sh -n "$PKG/files/ttyd.init"
"$0" --check
