#!/bin/sh
# Exercise actual init functions with UCI/procd stubs; no production networking.
set -eu
INIT=${1:?init path required}
# Remove only rc.common shebang; sourcing defines functions without starting ttyd.
. "$INIT"
config_get_bool() { eval "$1=\${ENABLED:-1}"; }
config_get() { eval "$1=\${IFACE:-}"; }
config_load() { [ "$1" = ttyd ]; }
config_foreach() { "$1" cfg; }
procd_add_reload_trigger() { :; }
procd_add_interface_trigger() { printf '%s|%s|%s|%s\n' "$1" "$2" "$3" "$4"; }
ENABLED=1 IFACE=@lan
[ "$(service_triggers)" = 'interface.up|lan|/etc/init.d/ttyd|restart' ]
IFACE=@guest
[ "$(service_triggers)" = 'interface.up|guest|/etc/init.d/ttyd|restart' ]
ENABLED=0
[ -z "$(service_triggers)" ]
ENABLED=1 IFACE=br-lan
[ -z "$(service_triggers)" ]
IFACE=''
[ -z "$(service_triggers)" ]
printf 'PASS: logical LAN/guest restart triggers; disabled/raw/unbound excluded\n'
