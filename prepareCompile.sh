#/bin/sh
# HomeLede build preparation.
#
# Duplicate-package resolution is handled purely by feed ORDER in
# feeds.conf.default: scripts/feeds install -a grants each package name to the
# FIRST feed providing it (pwPkgs > packages > luci > ...).
# Do NOT reintroduce rm -rf suppression lists here: feeds update -a does a
# git pull per feed and resurrects deleted dirs, silently undoing them.

./scripts/feeds update -a

# Re-apply our in-tree customizations to the freshly pulled feeds.
# feeds update git-resets in-place edits, so the overview-page block
# registration is replayed here every build.
./custom/apply-feed-customizations.sh || exit 1

./scripts/feeds update -i
./scripts/feeds install -a

if [ ! -f .config ]; then
\tcp defconfig .config
\techo "Default .config created."
fi
