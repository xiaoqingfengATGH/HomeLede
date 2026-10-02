#!/bin/sh
# HomeLede build preparation.
#
# Duplicate-package resolution is handled purely by feed ORDER in
# feeds.conf.default: scripts/feeds install -a grants each package name to the
# FIRST feed providing it (pwPkgs > packages > luci > ...).
# Do NOT reintroduce rm -rf suppression lists here: feeds update -a does a
# git pull per feed and resurrects deleted dirs, silently undoing them.
#
# Configuration source (defconfig is RETIRED):
#   - Software selection lives in native OpenWrt locations:
#       target/linux/x86/Makefile  DEFAULT_PACKAGES  (per-target defaults)
#       include/target.mk          DEFAULT_PACKAGES.router (global profile)
#   - Non-package build options that have no native default location
#     (LUCI language, image formats, kernel trimming, package feature
#     sub-options like passwall2_INCLUDE_*, build flavor of mbedtls/nginx/
#     sing-box/...) are seeded from scripts/seed.config (standard seed ->
#     make defconfig flow, same mechanism as upstream OpenWrt CI).

./scripts/feeds update -a

# Re-apply our in-tree customizations to the freshly pulled feeds.
# feeds update git-resets in-place edits, so the overview-page block
# registration is replayed here every build.
./custom/apply-feed-customizations.sh || exit 1

./scripts/feeds update -i
./scripts/feeds install -a

if [ ! -f .config ]; then
	cp scripts/seed.config .config
	echo "Default .config seeded from scripts/seed.config."
fi
