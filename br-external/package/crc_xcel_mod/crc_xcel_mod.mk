# crc module
#
CRC_XCEL_MOD_VERSION = 0.1
CRC_XCEL_MOD_SITE = $(BR2_EXTERNAL_PCI_DEV_PATH)/package/crc_xcel_mod/src
CRC_XCEL_MOD_SITE_METHOD = local
CRC_XCEL_MOD_INSTALL_STAGING = YES

define CRC_XCEL_MOD_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0755 $(@D)/crc-xcel-drv.h $(STAGING_DIR)/usr/include/crc-xcel-drv.h
endef

$(eval $(kernel-module))
$(eval $(generic-package))
