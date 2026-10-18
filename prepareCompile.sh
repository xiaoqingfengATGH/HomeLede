#!/bin/sh
# HomeLede build preparation.
#
# Duplicate-package resolution normally follows feed ORDER in
# feeds.conf.default: scripts/feeds install -a grants each package name to the
# FIRST feed providing it (pwPkgs > packages > luci > ...).
# ddns-go is an explicit exception selected below from our custom feed.
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

# This unused legacy UI introduces structural StrongSwan/PPP Kconfig cycles.
# HomeVPN uses its own frontend; keep StrongSwan itself available.
./scripts/feeds uninstall luci-app-ipsec-server || exit 1

# Stale core metadata must not hide our HomePublish feed providers.
for pkg in homepublish homepublish-engines luci-app-homepublish; do
	./scripts/feeds install -f -p xiaoqingfeng "$pkg" || exit 1
done

# -f only overrides core packages; it does not replace an installed feed link.
# Remove only this source's old selection, then prefer our same-name package.
[ -f feeds/xiaoqingfeng/ddns-go/Makefile ] || {
	echo "Missing custom ddns-go package in xiaoqingfeng feed" >&2
	exit 1
}
./scripts/feeds uninstall ddns-go || exit 1
./scripts/feeds install -f -p xiaoqingfeng ddns-go || exit 1
[ "$(readlink package/feeds/xiaoqingfeng/ddns-go)" = '../../../feeds/xiaoqingfeng/ddns-go' ] || {
	echo "Custom ddns-go feed selection failed" >&2
	exit 1
}

if [ ! -f .config ]; then
	cp scripts/seed.config .config
	echo "Default .config seeded from scripts/seed.config."
fi
