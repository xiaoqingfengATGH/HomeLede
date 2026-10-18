#!/bin/sh
# Isolated feed-setup contract; never invokes the real feeds tool.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
TEST_ROOT=$(mktemp -d "${TMPDIR:-/tmp}/homelede-feed-selection.XXXXXX")
trap 'rm -rf "$TEST_ROOT"' EXIT HUP INT TERM
mkdir -p "$TEST_ROOT/scripts" "$TEST_ROOT/custom" "$TEST_ROOT/feeds/xiaoqingfeng/ddns-go" "$TEST_ROOT/package/feeds/packages" "$TEST_ROOT/package/feeds/xiaoqingfeng"
cp "$ROOT/prepareCompile.sh" "$TEST_ROOT/prepareCompile.sh"
printf 'seed\n' > "$TEST_ROOT/scripts/seed.config"
: > "$TEST_ROOT/feeds/xiaoqingfeng/ddns-go/Makefile"
printf '#!/bin/sh\nexit 0\n' > "$TEST_ROOT/custom/apply-feed-customizations.sh"
cat > "$TEST_ROOT/scripts/feeds" <<'STUB'
#!/bin/sh
set -eu
printf '%s\n' "$*" >> calls
case "$*" in
  'update -a'|'update -i') ;;
  'install -a')
    if [ ! -L package/feeds/xiaoqingfeng/ddns-go ]; then
      ln -sf ../../../feeds/packages/net/ddns-go package/feeds/packages/ddns-go
    fi ;;
  'uninstall ddns-go') rm -f package/feeds/*/ddns-go ;;
  'install -f -p xiaoqingfeng ddns-go')
    [ "${FAIL_OVERRIDE:-0}" = 0 ] || exit 7
    ln -sf ../../../feeds/xiaoqingfeng/ddns-go package/feeds/xiaoqingfeng/ddns-go ;;
  *) exit 9 ;;
esac
STUB
chmod +x "$TEST_ROOT/scripts/feeds" "$TEST_ROOT/custom/apply-feed-customizations.sh"
cd "$TEST_ROOT"
ln -s ../../../feeds/packages/net/ddns-go package/feeds/packages/ddns-go
for pass in 1 2; do
  sh ./prepareCompile.sh
  [ "$(readlink package/feeds/xiaoqingfeng/ddns-go)" = '../../../feeds/xiaoqingfeng/ddns-go' ] || { echo 'FAIL: custom ddns-go was not selected' >&2; exit 1; }
  [ ! -L package/feeds/packages/ddns-go ]
done
printf 'PASS: stale upstream selection replaced; second run stays custom\n'
FAIL_OVERRIDE=1; export FAIL_OVERRIDE
if sh ./prepareCompile.sh; then
  echo 'FAIL: override failure was ignored' >&2; exit 1
fi
printf 'PASS: failed override stops preparation\n'
unset FAIL_OVERRIDE
rm feeds/xiaoqingfeng/ddns-go/Makefile
if sh ./prepareCompile.sh; then
  echo 'FAIL: missing custom package was ignored' >&2; exit 1
fi
printf 'PASS: missing custom package stops preparation\n'
