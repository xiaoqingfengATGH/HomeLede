#!/bin/sh
# HomeLede build-only patch; never run against a router block device.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$ROOT"
PATCH=custom/patches/001-x86-complete-gpt.patch
case "${1:-}" in ''|--check) ;; *) echo "Usage: $0 [--check]" >&2; exit 2 ;; esac
if git apply --reverse --check "$PATCH" 2>/dev/null; then
    echo 'HomeLede image GPT customization already applied'
elif git apply --check "$PATCH" 2>/dev/null; then
    if [ "${1:-}" = --check ]; then
        echo 'HomeLede image GPT customization is not applied' >&2
        exit 1
    fi
    git apply "$PATCH"
    echo 'Applied HomeLede image GPT customization'
else
    echo 'GPT patch does not match upstream source; review required' >&2
    exit 1
fi
sh -n scripts/gen_image_generic.sh
