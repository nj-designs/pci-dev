# lua module
#
LUA_MOD_VERSION = 0.1
LUA_MOD_SITE = $(BR2_EXTERNAL_PCI_DEV_PATH)/package/lua_mod/src
LUA_MOD_SITE_METHOD = local
LUA_MOD_INSTALL_STAGING = YES

#define LUA_MOD_INSTALL_STAGING_CMDS
#	$(INSTALL) -D -m 0755 $(@D)/crc-xcel-drv.h $(STAGING_DIR)/usr/include/crc-xcel-drv.h
#endef

$(eval $(kernel-module))
$(eval $(generic-package))
