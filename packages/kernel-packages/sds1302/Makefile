#
# Copyright (C) 2007-2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=sds1302
PKG_VERSION:=2017-07-17
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/$(PKG_NAME)
    SUBMENU:=Other modules
    TITLE:=ported DS1302 driver
    FILES:= $(PKG_BUILD_DIR)/sds1302_chr.ko
    AUTOLOAD:=$(call AutoLoad,99,sds1302)
endef

define KernelPackage/$(PKG_NAME)/description
	RTC DS1302 driver
endef

MAKE_OPTS:= \
	ARCH="$(LINUX_KARCH)" \
	CROSS_COMPILE="$(TARGET_CROSS)" \
	SUBDIRS="$(PKG_BUILD_DIR)" \
	EXTRA_CFLAGS="-g $(BUILDFLAGS)"

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(MAKE) -C "$(LINUX_DIR)" \
		$(MAKE_OPTS) \
		modules
endef

$(eval $(call KernelPackage,sds1302))
