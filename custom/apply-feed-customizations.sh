#!/bin/sh
# HomeLede: re-apply customizations to third-party feeds after `./scripts/feeds update`.
#
# Why this exists: `./scripts/feeds update -a` does a `git pull` on every feed and will
# silently discard any in-place edit made to a feed's source tree.  Such edits live
# here as idempotent operations that can be replayed any number of times (驾驶舱
# overview blocks / status title, cgroupfs-mount's cgroup v2 boot mount).
#
# Idempotency: each operation asserts its end state (entry present / entry absent) and
# only writes when that state differs, so it is safe to run repeatedly.
#
# Usage:  ./custom/apply-feed-customizations.sh          # apply + verify
#         ./custom/apply-feed-customizations.sh --check   # verify only, no writes
#
# Invoked from prepareCompile.sh right after `./scripts/feeds update -a`.

set -u

MARK='HOMELEDE-CUSTOM'
CHECK_ONLY=0
[ "${1:-}" = "--check" ] && CHECK_ONLY=1

TOPDIR="$(cd "$(dirname "$0")/.." && pwd)" || exit 1

# ---------------------------------------------------------------- target 1 ----
# luci-mod-status: drop the two homestatus blocks from the 驾驶舱 include list.
#
# The 驾驶舱 page renders its own 关键应用 and 存储 cards in the main area. The
# stock status include list also carries 25_storage plus our two 95_homestatus*
# blocks, which land in the collapsed 经典信息视图 drawer and duplicate the main
# cards (same backend, same data). Remove the duplicates so each datum has one
# home; the drawer keeps the stock sections that have no main-area equivalent
# (系统 / 内存 / 网络 / DHCP 租约 / 无线).
#
# Upstream openwrt/luci discovers overview blocks by *scanning* the include
# directory at runtime (fs.list on /www/luci-static/resources/view/status/include)
# so deleting the files would be enough there.  coolsnowwolf/luci rewrote the
# loader into a hard-coded `includeModules` array, so the entries have to be
# dropped from that array explicitly -- and `apply_status_include` used to add
# the two 95_* ones, so this also has to clean up a tree that was patched
# before this change.
ST_TARGET='feeds/luci/modules/luci-mod-status/htdocs/luci-static/resources/view/status/index.js'
ST_DROP='95_homestatus_disks
95_homestatus_apps
25_storage'

apply_status_include() {
	_t="$TOPDIR/$ST_TARGET"

	if [ ! -f "$_t" ]; then
		echo "  [FAIL] $ST_TARGET not found -- run './scripts/feeds update -a' first" >&2
		return 1
	fi

	# How many entries still need removing (and, for a tree patched before this
	# change, how many marker comments come with them).
	_n=0
	for _m in $ST_DROP; do
		grep -q "include\\.$_m" "$_t" && _n=$((_n + 1))
	done

	if [ "$_n" -eq 0 ]; then
		echo "  [ ok ] luci-mod-status: drawer duplicates already dropped"
		return 0
	fi

	if [ "$CHECK_ONLY" = "1" ]; then
		echo "  [MISS] luci-mod-status: ${_n}x duplicate block still registered"
		return 1
	fi

	# Drop each entry together with its separator so the array stays valid, in
	# a single pass over the includeModules array: buffer the block, filter it,
	# then re-emit with the separator restored on every entry but the last.
	#
	# Doing this as one pass rather than "delete line, then un-comma the last"
	# matters: repairing the trailing comma afterwards cannot tell the array's
	# own closing bracket from the nested ones in the render body below, and
	# would strip a legitimate comma there.
	#
	# Line endings are normalised to LF -- a CRLF copy (this tree is edited from
	# Windows too) would otherwise survive into the emitted JavaScript.
	awk -v drops="$ST_DROP" -v mark="$MARK" '
		function wanted(name,   n, arr, i) {
			n = split(drops, arr, "\n")

			for (i = 1; i <= n; i++)
				if (arr[i] != "" && index(name, "include." arr[i]) > 0)
					return 1

			return 0
		}
		{
			line = $0
			sub(/\r$/, "", line)

			if (!in_array) {
				print line

				if (line ~ /includeModules[[:space:]]*=[[:space:]]*\[/) {
					in_array = 1
					kept = 0
					last = 0
				}

				next
			}

			# Inside the array. Only entry lines are buffered, so that the
			# separators can be re-emitted without touching anything else.
			if (line ~ /^[[:space:]]*\];/) {
				if (last > 0)
					sub(/,[[:space:]]*$/, "", kept_lines[last])

				for (i = 1; i <= last; i++)
					print kept_lines[i]

				in_array = 0
				print line
				next
			}

			if (line ~ /^[[:space:]]*\{ name:/) {
				if (wanted(line)) {
					dropped++
					next
				}

				kept_lines[++last] = line
				next
			}

			print line
		}
		END {
			if (dropped == 0)
				exit 3

			if (in_array)
				exit 4
		}
	' "$_t" > "$_t.homelede-new" || {
		rc=$?
		rm -f "$_t.homelede-new"
		case "$rc" in
			3) echo "  [FAIL] luci-mod-status: no duplicate entry matched -- loader rewritten upstream?" >&2 ;;
			4) echo "  [FAIL] luci-mod-status: includeModules array not closed -- refusing to write" >&2 ;;
			*) echo "  [FAIL] luci-mod-status: failed to rewrite the include list" >&2 ;;
		esac
		return 1
	}

	mv "$_t.homelede-new" "$_t"
	echo "  [done] luci-mod-status: dropped ${_n}x duplicate drawer block"
	return 0
}

# ---------------------------------------------------------------- target 2 ----
# luci-mod-status: page heading follows the dispatched menu title.
#
# The heading is hard-coded to _('Status'), which is wrong for this fork: we
# rename admin/status/overview ("Overview" -> 驾驶舱) through a menu.d override,
# so the tab, the browser title and the heading would otherwise disagree.
# Binding the heading to `dispatched.title` keeps the name in exactly one
# place -- the menu entry -- so a future rename needs no second edit here.
#
# `dispatched` is set by the dispatcher's own render path (runtime.env.dispatched)
# and is already in scope in this template: the theme's header.ut uses it for
# the <title> element.
#
# The template is Lua- and ucode-side the same file: admin_status/index.ut is
# installed from ucode/template/ under /usr/share/ucode/luci/template/.
UT_TARGET='feeds/luci/modules/luci-mod-status/ucode/template/admin_status/index.ut'
UT_FROM="{{ _('Status') }}"
UT_TO="{{ _(dispatched.title) }}"

apply_status_title() {
	_t="$TOPDIR/$UT_TARGET"

	if [ ! -f "$_t" ]; then
		echo "  [FAIL] $UT_TARGET not found -- run './scripts/feeds update -a' first" >&2
		return 1
	fi

	if grep -qF "$UT_TO" "$_t"; then
		echo "  [ ok ] luci-mod-status: page heading already follows the menu title"
		return 0
	fi

	if ! grep -qF "$UT_FROM" "$_t"; then
		echo "  [FAIL] luci-mod-status: heading anchor '$UT_FROM' not found" >&2
		return 1
	fi

	if [ "$CHECK_ONLY" = "1" ]; then
		echo "  [MISS] luci-mod-status: page heading is still hard-coded"
		return 1
	fi

	# In-place, LF-safe: the tree is edited from Windows as well, and a CR
	# left inside the expression would break the template.
	sed -i "s|$UT_FROM|$UT_TO|" "$_t"
	echo "  [done] luci-mod-status: page heading follows the menu title"
	return 0
}

# ---------------------------------------------------------------- target 3 ----
# cgroupfs-mount: mount the unified cgroup v2 hierarchy at boot.
#
# Kernels from the 6.1x line retire the cgroup v1 controller interfaces
# (CONFIG_MEMCG_V1 / CONFIG_CPUSETS_V1 default to unset) while the v2
# cores stay enabled.  The 2020-era cgroupfs-mount in this feed mounts a
# v1 hierarchy at S01; on such a kernel the v1 tree has no memory or
# cpuset controllers, dockerd lands on it anyway, and every container
# resource limit (--memory/--cpus/--cpuset-cpus/--pids-limit) is
# silently unenforced while docker info prints five WARNINGs.
#
# Rewrite the init script to mount cgroup2 on /sys/fs/cgroup and arm the
# controllers docker uses at the root.  Verified live on the target
# router: dockerd then reports "Cgroup Version: 2" and the limit
# warnings disappear; only the meaningless no-swap notice remains on a
# swap-less machine.
#
# Selection note: nothing depends on cgroupfs-mount and dockerd cannot
# start without a cgroup mount, so target/linux/x86/Makefile carries it
# in DEFAULT_PACKAGES.
CG_TARGET='feeds/packages/utils/cgroupfs-mount/files/cgroupfs-mount.init'

apply_cgroup_v2() {
	_t="$TOPDIR/$CG_TARGET"

	if [ ! -f "$_t" ]; then
		echo "  [FAIL] $CG_TARGET not found -- run './scripts/feeds update -a' first" >&2
		return 1
	fi

	if grep -q "$MARK" "$_t"; then
		echo "  [ ok ] cgroupfs-mount: v2 boot mount already applied"
		return 0
	fi

	if [ "$CHECK_ONLY" = "1" ]; then
		echo "  [MISS] cgroupfs-mount: still mounts the v1 hierarchy"
		return 1
	fi

	# Full replacement, LF-only (the tree is edited from Windows too).
	# The marker comment doubles as the idempotency probe above.
	cat > "$_t" <<'HOMELEDE_CGROUP2_EOF'
#!/bin/sh /etc/rc.common

# HOMELEDE-CUSTOM: mount the unified cgroup v2 hierarchy at boot.
#
# This kernel (6.1x+) has the cgroup v1 controller interfaces compiled
# out (CONFIG_MEMCG_V1 / CONFIG_CPUSETS_V1 unset), so a v1 hierarchy
# carries no memory/cpuset controllers and dockerd landing on it loses
# every container resource limit.  Mount cgroup2 at the canonical path
# dockerd probes and arm the controllers docker uses at the root; the
# root cgroup is exempt from the no-internal-process rule, so arming
# works at S01 with every boot process still living in the root.

START=01

boot() {
	if grep -qs ' /sys/fs/cgroup cgroup2 ' /proc/mounts; then
		: # already up -- nothing to do
	else
		# procd stacks a plain tmpfs over /sys/fs/cgroup; mounting
		# cgroup2 on top of it is fine and is what dockerd probes.
		if ! mount -t cgroup2 none /sys/fs/cgroup; then
			echo "cgroupfs-mount: failed to mount cgroup2 on /sys/fs/cgroup" >&2
			return 1
		fi
	fi

	echo '+cpuset +cpu +io +memory +pids' > /sys/fs/cgroup/cgroup.subtree_control 2>/dev/null || \
		echo "cgroupfs-mount: warning: could not arm cgroup controllers" >&2
}
HOMELEDE_CGROUP2_EOF

	echo "  [done] cgroupfs-mount: mounts cgroup v2 at boot"
	return 0
}

# ---------------------------------------------------------------- verify ------
verify() {
	rc=0

	# target 1: the duplicate drawer blocks must be gone from the include list.
	_t="$TOPDIR/$ST_TARGET"
	_n=0
	for _m in $ST_DROP; do
		if grep -q "include\\.$_m" "$_t" 2>/dev/null; then
			echo "  [FAIL] loader still registers $_m" >&2
			rc=1
		else
			_n=$((_n + 1))
		fi
	done
	[ "$_n" -gt 0 ] && echo "  [ ok ] loader no longer registers the ${_n}x duplicate block"

	# Syntax-check the patched loader. This has caught a real break: emitting
	# array entries without separators produces valid-looking text that fails
	# to parse, and the page then renders nothing at all.
	#
	# node is a native binary on some build hosts (Windows), where an MSYS
	# style path like /z/... cannot be opened -- it would report a bogus syntax
	# error. Run it from the file's own directory with a bare filename.
	if command -v node >/dev/null 2>&1; then
		if (cd "$(dirname "$_t")" && node --check "$(basename "$_t")") >/dev/null 2>&1; then
			echo "  [ ok ] patched loader parses"
		else
			echo "  [FAIL] patched loader has a syntax error (node --check)" >&2
			rc=1
		fi
	else
		echo "  [warn] node not available - skipped loader syntax check" >&2
	fi

	# The block files were removed with the duplicates; the loader entries are
	# gone too, so nothing should reference them any more.
	for _f in 95_homestatus_disks 95_homestatus_apps; do
		_b="$TOPDIR/feeds/xiaoqingfeng/luci-app-homestatus/htdocs/luci-static/resources/view/status/include/$_f.js"
		if [ -f "$_b" ]; then
			echo "  [FAIL] superseded block still present: $_f.js" >&2
			rc=1
		else
			echo "  [ ok ] superseded block removed: $_f.js"
		fi
	done

	# target 2: the page heading must follow the dispatched menu title.
	_ut="$TOPDIR/$UT_TARGET"
	if grep -qF "$UT_TO" "$_ut" 2>/dev/null; then
		echo "  [ ok ] page heading follows the menu title"
	else
		echo "  [FAIL] page heading is still hard-coded to _('Status')" >&2
		rc=1
	fi

	# The overview rename lives in our own feed package as a menu.d override
	# that must sort last (the dispatcher merges menu.d files in glob order,
	# last write wins for a given path).
	_ov="$TOPDIR/feeds/xiaoqingfeng/luci-app-homestatus/root/usr/share/luci/menu.d/zz-homelede-overview.json"
	if [ -f "$_ov" ]; then
		echo "  [ ok ] overview rename override present: $(basename "$_ov")"
	else
		echo "  [FAIL] overview rename override missing ($_ov)" >&2
		rc=1
	fi

	# target 3: the cgroup boot mount must be the v2 unified hierarchy.
	_cg="$TOPDIR/$CG_TARGET"
	if grep -q 'mount -t cgroup2' "$_cg" 2>/dev/null; then
		echo "  [ ok ] cgroupfs-mount mounts the unified v2 hierarchy"

		# Parse-check the rewritten init like the patched loader above:
		# a heredoc with broken quoting would otherwise install a
		# boot-time syntax error on every fresh feed.
		if sh -n "$_cg" 2>/dev/null; then
			echo "  [ ok ] rewritten cgroupfs-mount parses"
		else
			echo "  [FAIL] rewritten cgroupfs-mount has a syntax error (sh -n)" >&2
			rc=1
		fi
	else
		echo "  [FAIL] cgroupfs-mount does not mount cgroup2" >&2
		rc=1
	fi

	return $rc
}

echo "HomeLede feed customizations ($([ "$CHECK_ONLY" = 1 ] && echo check || echo apply)):"

# In check mode a still-unapplied target reports MISS and returns 1; that is
# the expected answer, not a failure to abort on. verify() is the authority
# either way, so let every target report and then let verify set the exit code.
apply_status_include || [ "$CHECK_ONLY" = 1 ] || { echo "aborting" >&2; exit 1; }
apply_status_title   || [ "$CHECK_ONLY" = 1 ] || { echo "aborting" >&2; exit 1; }
apply_cgroup_v2      || [ "$CHECK_ONLY" = 1 ] || { echo "aborting" >&2; exit 1; }
verify || exit 1
exit 0
