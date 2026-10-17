# ttyd interface rebind customization

The package init binds `@lan` to the current device. Recreating a bridge can
leave the listener bound to a deleted ifindex. Process status and LISTEN alone
do not detect this failure.

`prepareCompile.sh` and CI already call `custom/apply-feed-customizations.sh`
after feeds update. It calls `custom/apply-ttyd-customization.sh`, which applies
`custom/patches/ttyd-interface-rebind.patch` to the package init file. Keep these
files in the firmware repository, not just the generated feed checkout.
This is NOT a daemon-source patch under the package patches/ directory.

The patch registers a logical-interface up trigger for enabled ttyd sections
with `interface=@NAME`. It executes restart, since reload with unchanged argv
may leave the stale socket intact. It does not broaden listening scope, alter
credentials, or register triggers for raw device names or unbound services.
Network down/update alone is intentionally not a trigger: wait for the configured
network to be up. Plain raw-device bindings are outside this patch's scope.

Checks:

```sh
sh custom/apply-ttyd-customization.sh --check
sh custom/tests/ttyd-interface-rebind.sh feeds/packages/utils/ttyd/files/ttyd.init
make package/feeds/packages/ttyd/{clean,compile} -j2 V=s
```

Applying is idempotent. Upstream context changes that prevent either a strict
forward or reverse dry run stop preparation and require review. No daemon version
or package release bump is included; force clean/compile to refresh existing
same-version package caches. Fresh firmware builds naturally pick up the init.

Do not restart or recreate production LAN for testing. Fixture tests prove trigger
registration, not live netifd/procd bridge-recreation acceptance. Runtime acceptance
requires an isolated interface event test and HTTP/WebSocket checks.
