#!/bin/sh
# Isolated feed-setup contract; never invokes the real feeds tool.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
TEST_ROOT=$(mktemp -d "${TMPDIR:-/tmp}/homelede-feed-selection.XXXXXX")
trap 'rm -rf "$TEST_ROOT"' EXIT HUP INT TERM
mkdir -p "$TEST_ROOT/scripts" "$TEST_ROOT/custom" "$TEST_ROOT/package/feeds/packages" "$TEST_ROOT/package/feeds/xiaoqingfeng"
cp "$ROOT/prepareCompile.sh" "$TEST_ROOT/prepareCompile.sh"
printf '#!/bin/sh\nexit 0\n' > "$TEST_ROOT/custom/apply-image-customizations.sh"
chmod +x "$TEST_ROOT/custom/apply-image-customizations.sh"
printf 'seed\n' > "$TEST_ROOT/scripts/seed.config"
printf '#!/bin/sh\nexit 0\n' > "$TEST_ROOT/custom/apply-feed-customizations.sh"
cat > "$TEST_ROOT/scripts/feeds" <<'STUB'
#!/bin/sh
set -eu
printf '%s\n' "$*" >> calls
case "$*" in
  'update -a'|'update -i') ;;
  'install -a')
    for pkg in ddns-go open-vm-tools; do
      if [ ! -L "package/feeds/xiaoqingfeng/$pkg" ]; then
        ln -sf "../../../feeds/packages/net/$pkg" "package/feeds/packages/$pkg"
      fi
    done ;;
  'uninstall luci-app-ipsec-server') ;;
  'install -f -p xiaoqingfeng homepublish'|'install -f -p xiaoqingfeng homepublish-engines'|'install -f -p xiaoqingfeng luci-app-homepublish') ;;
  'uninstall ddns-go'|'uninstall open-vm-tools') rm -f package/feeds/*/"$2" ;;
  'install -f -p xiaoqingfeng ddns-go'|'install -f -p xiaoqingfeng open-vm-tools')
    pkg=$5
    if [ "${FAULT_PKG:-}" = "$pkg" ]; then
      case "${FAULT_MODE:-}" in
        install) exit 7 ;;
        link)
          ln -sf "../../../feeds/packages/net/$pkg" "package/feeds/xiaoqingfeng/$pkg"
          exit 0 ;;
      esac
    fi
    ln -sf "../../../feeds/xiaoqingfeng/$pkg" "package/feeds/xiaoqingfeng/$pkg" ;;
  *) exit 9 ;;
esac
STUB
chmod +x "$TEST_ROOT/scripts/feeds" "$TEST_ROOT/custom/apply-feed-customizations.sh"
cd "$TEST_ROOT"
fail() { echo "FAIL: $*" >&2; exit 1; }
reset_fixture() {
  rm -f .config calls package/feeds/packages/* package/feeds/xiaoqingfeng/*
  for pkg in ddns-go open-vm-tools; do
    mkdir -p "feeds/xiaoqingfeng/$pkg"
    : > "feeds/xiaoqingfeng/$pkg/Makefile"
    ln -s "../../../feeds/packages/net/$pkg" "package/feeds/packages/$pkg"
  done
}
expected_calls() {
  printf '%s\n' 'update -a' 'update -i' 'install -a' 'uninstall luci-app-ipsec-server' \
    'install -f -p xiaoqingfeng homepublish' \
    'install -f -p xiaoqingfeng homepublish-engines' \
    'install -f -p xiaoqingfeng luci-app-homepublish'
  for selected in ddns-go open-vm-tools; do
    if [ "${FAULT_PKG:-}" = "$selected" ] && [ "${FAULT_MODE:-}" = missing ]; then
      return
    fi
    printf '%s\n' "uninstall $selected" "install -f -p xiaoqingfeng $selected"
    if [ "${FAULT_PKG:-}" = "$selected" ]; then
      return
    fi
  done
}
assert_calls() {
  expected_calls > expected
  cmp -s expected calls || {
    diff -u expected calls >&2 || :
    fail 'unexpected feed calls (failure must stop immediately)'
  }
}
reset_fixture
for pass in 1 2; do
  : > calls
  sh ./prepareCompile.sh
  for pkg in ddns-go open-vm-tools; do
    [ "$(readlink "package/feeds/xiaoqingfeng/$pkg")" = "../../../feeds/xiaoqingfeng/$pkg" ] || fail "custom $pkg was not selected"
    [ ! -e "package/feeds/packages/$pkg" ] && [ ! -L "package/feeds/packages/$pkg" ] || fail "old $pkg link remains"
  done
  assert_calls
  cmp -s scripts/seed.config .config || fail 'config was not seeded'
  printf 'PASS: both custom providers selected; stale links absent (run %s)\n' "$pass"
done
for FAULT_PKG in ddns-go open-vm-tools; do
  for FAULT_MODE in missing install link; do
    reset_fixture
    export FAULT_PKG FAULT_MODE
    if [ "$FAULT_MODE" = missing ]; then
      rm "feeds/xiaoqingfeng/$FAULT_PKG/Makefile"
    fi
    if sh ./prepareCompile.sh > output 2>&1; then
      fail "$FAULT_PKG $FAULT_MODE fault was ignored"
    fi
    assert_calls
    [ ! -e .config ] || fail 'preparation continued to config after fault'
    case "$FAULT_MODE" in
      missing) grep -F "Missing custom $FAULT_PKG package in xiaoqingfeng feed" output >/dev/null || fail 'missing-source diagnostic absent' ;;
      link) grep -F "Custom $FAULT_PKG feed selection failed" output >/dev/null || fail 'wrong-link diagnostic absent' ;;
    esac
    printf 'PASS: %s %s fault stops preparation immediately\n' "$FAULT_PKG" "$FAULT_MODE"
  done
done
