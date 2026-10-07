# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2006-2011 OpenWrt.org

include $(TOPDIR)/rules.mk

ARCH:=i386
BOARD:=x86
BOARDNAME:=x86
FEATURES:=squashfs vdi vmdk vhdx pcmcia fpu boot-part rootfs-part
SUBTARGETS:=64 generic geode legacy

KERNEL_PATCHVER:=6.18
KERNEL_TESTING_PATCHVER:=6.12

KERNELNAME:=bzImage

include $(INCLUDE_DIR)/target.mk

# HomeLede x86_64 software set -- migrated from the retired defconfig (2026-10).
# Software selection lives here natively; non-package build knobs are seeded
# from scripts/seed.config (see that file header for the split).
DEFAULT_PACKAGES += \
	partx-utils \
	mkf2fs \
	e2fsprogs \
	kmod-button-hotplug \
	kmod-fs-f2fs \
	kmod-mmc \
	kmod-sdhci \
	kmod-usb-hid \
	cfdisk \
	kmod-bnx2x \
	kmod-i40e \
	kmod-i40evf \
	kmod-iavf \
	kmod-igb \
	kmod-igbvf \
	kmod-igc \
	kmod-e1000e \
	kmod-pcnet32 \
	kmod-tulip \
	kmod-via-velocity \
	kmod-vmxnet3 \
	kmod-sound-hda-core \
	kmod-sound-hda-codec-realtek \
	kmod-sound-hda-codec-via \
	kmod-sound-via82xx \
	kmod-sound-hda-intel \
	kmod-sound-hda-codec-hdmi \
	kmod-sound-i8x0 \
	kmod-usb-net \
	kmod-usb-net-aqc111 \
	kmod-usb-net-asix-ax88179 \
	kmod-usb-net-rtl8150 \
	kmod-mlx4-core \
	kmod-mlx5-core \
	kmod-drm-i915 \
	kmod-drm-amdgpu \
	kmod-usb-audio \
	kmod-usb-net-rtl8152-vendor \
	kmod-8139cp \
	kmod-8139too \
	kmod-tg3 \
	lm-sensors \
	autocore-x86 \
	automount \
	ca-bundle \
	ca-certificates \
	dockerd \
	docker \
	cgroupfs-mount \
	open-vm-tools \
	bind-dig \
	unrar \
	unzip \
	dmesg \
	p7zip \
	luci-theme-bootstrap \
	ipv6helper \
	luci-app-accesscontrol \
	luci-app-control-timewol \
	luci-app-control-webrestriction \
	luci-app-control-weburl \
	luci-app-cifs-mount \
	luci-app-dockerman \
	luci-app-hd-idle \
	luci-app-homeredirect \
	luci-app-homestatus \
	luci-app-hometunnel \
	luci-app-mwan3 \
	luci-app-syncdial \
	luci-app-ttyd \
	luci-app-upnp \
	luci-app-vlmcsd \
	luci-app-vsftpd \
	luci-app-watchcat \
	luci-app-wol \
	ppp-mod-pppoa \
	ppp-mod-pppoe \
	ppp-mod-pppol2tp \
	ppp-mod-pptp \
	xl2tpd

# ACME (Letsencrypt client + DNS API)
DEFAULT_PACKAGES += acme-acmesh acme-acmesh-dnsapi

# system tools
DEFAULT_PACKAGES += aria2 ariang attr bash blkid coreutils coreutils-base64 coreutils-nohup coreutils-timeout dbus ddns-go dosfstools e100-firmware exfat-fsck exfat-mkfs f2fs-tools f2fsck fdisk firewall4 htop ip6tables-mod-nat iptables-mod-iprange iptables-mod-ipsec iptables-mod-socket iputils-arping jansson jq losetup lyaml mdadm miniupnpd-nftables nftables-json oniguruma openssh-keygen openssh-sftp-server openssl-util parted r8169-firmware resize2fs smartmontools usbutils wol

# file sharing (samba4 + wsdd2/avahi discovery)
DEFAULT_PACKAGES += avahi-dbus-daemon samba4-libs samba4-server wsdd2

# passwall2 companions (cores pulled by its INCLUDE options)
DEFAULT_PACKAGES += chinadns-ng geoview haproxy hysteria mosdns naiveproxy pdnsd-alt shadowsocks-rust-sslocal shadowsocks-rust-ssserver shadowsocksr-libev-ssr-local shadowsocksr-libev-ssr-redir shadowsocksr-libev-ssr-server simple-obfs-client sing-box tcping v2dat v2ray-geoip v2ray-geosite v2ray-plugin xray-core

# kernel modules
DEFAULT_PACKAGES += kmod-alx kmod-ata-ahci kmod-ata-core kmod-ata-piix kmod-ata-sil kmod-ata-sil24 kmod-ata-via-sata kmod-crypto-cbc kmod-crypto-deflate kmod-crypto-echainiv kmod-crypto-kpp kmod-crypto-lib-chacha20 kmod-crypto-lib-chacha20poly1305 kmod-crypto-lib-curve25519 kmod-crypto-lib-poly1305 kmod-e100 kmod-fs-msdos kmod-fs-nfs kmod-fs-nfs-common kmod-fs-nfs-v4 kmod-inet-diag kmod-ipsec kmod-ipsec4 kmod-ipsec6 kmod-ipt-iprange kmod-ipt-ipsec kmod-ipt-socket kmod-iptunnel6 kmod-loop kmod-md-linear kmod-md-mod kmod-md-raid0 kmod-md-raid1 kmod-md-raid10 kmod-md-raid456 kmod-netlink-diag kmod-nf-socket kmod-nft-fib kmod-nft-fullcone kmod-nft-nat kmod-nft-offload kmod-nft-socket kmod-nft-tproxy kmod-tcp-bbr kmod-nls-cp932 kmod-nls-cp936 kmod-nls-cp950 kmod-phy-marvell-10g kmod-phy-realtek kmod-r8125 kmod-r8126 kmod-r8169

# CPU microcode (both vendors shipped; the kernel loads only the matching one at boot)
DEFAULT_PACKAGES += intel-microcode amd64-microcode

# libraries
DEFAULT_PACKAGES += libaio libavahi-client libavahi-dbus-support libcares libdaemon libdbus libev libevdev libexpat libfreetype libgd-full libgmp libgnutls libiconv-full libiwinfo-lua libjpeg-turbo libltdl liblua5.3 liblua5.4 libmbedtls libminiupnpc libnatpmp libnetfilter-conntrack libnettle libnfnetlink libnftnl libparted libpng libsodium libsqlite3 libtasn1 libtiff libuci-lua libudev-zero libudns liburing libusb-1.0 libwebp libxml2 libyaml libzip-gnutls zoneinfo-asia zoneinfo-core

# LuCI applications
DEFAULT_PACKAGES += luci-app-homeacme luci-i18n-homeacme-zh-cn luci-app-aria2 luci-app-ddns-go luci-app-diskman luci-app-filebrowser luci-app-homevpn luci-app-partexp luci-app-passwall2 luci-app-pushbot luci-app-samba4

# LuCI infrastructure
DEFAULT_PACKAGES += luci-theme-infinityfreedom-ng

# nginx (https frontend)
DEFAULT_PACKAGES += nginx-ssl nginx-ssl-util nginx-util

# php8 stack (nginx fastcgi web apps)
DEFAULT_PACKAGES += php8 php8-cgi php8-fastcgi php8-fpm php8-mod-curl php8-mod-dom php8-mod-gd php8-mod-iconv php8-mod-mbstring php8-mod-mysqlnd php8-mod-opcache php8-mod-pdo php8-mod-pdo-mysql php8-mod-pdo-sqlite php8-mod-session php8-mod-sqlite3 php8-mod-xml php8-mod-xmlreader php8-mod-xmlwriter php8-mod-zip

# strongSwan IKEv2 server (EAP-MSCHAPv2, managed by luci-app-homevpn)
DEFAULT_PACKAGES += strongswan strongswan-charon strongswan-default strongswan-libtls strongswan-mod-aes strongswan-mod-attr strongswan-mod-connmark strongswan-mod-constraints strongswan-mod-des strongswan-mod-dhcp strongswan-mod-dnskey strongswan-mod-eap-identity strongswan-mod-eap-mschapv2 strongswan-mod-farp strongswan-mod-fips-prf strongswan-mod-gmp strongswan-mod-hmac strongswan-mod-kdf strongswan-mod-kernel-netlink strongswan-mod-md4 strongswan-mod-md5 strongswan-mod-mgf1 strongswan-mod-pem strongswan-mod-pgp strongswan-mod-pkcs1 strongswan-mod-pubkey strongswan-mod-random strongswan-mod-rc2 strongswan-mod-resolve strongswan-mod-revocation strongswan-mod-sha1 strongswan-mod-sha2 strongswan-mod-socket-default strongswan-mod-sshkey strongswan-mod-updown strongswan-mod-vici strongswan-mod-x509 strongswan-mod-xauth-generic strongswan-mod-xcbc strongswan-swanctl strongswan-mod-curve25519 strongswan-mod-gcm strongswan-mod-openssl strongswan-mod-pkcs8 strongswan-pki

$(eval $(call BuildTarget))

$(eval $(call $(if $(CONFIG_ISO_IMAGES),SetupHostCommand,Ignore),mkisofs, \
	Please install mkisofs. , \
	mkisofs -v 2>&1 , \
	genisoimage -v 2>&1 | grep genisoimage, \
	xorrisofs -v 2>&1 | grep xorriso \
))
