# crc library
#
CRC_XCEL_LIB_VERSION = 0.1
CRC_XCEL_LIB_SITE = $(BR2_EXTERNAL_PCI_DEV_PATH)/package/crc_xcel_lib/src
CRC_XCEL_LIB_SITE_METHOD = local
CRC_XCEL_LIB_INSTALL_STAGING = YES

define CRC_XCEL_LIB_BUILD_CMDS
	$(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) all
endef

define CRC_XCEL_LIB_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0755 $(@D)/libcrc-xcel.so $(STAGING_DIR)/usr/lib
	$(INSTALL) -D -m 0755 $(@D)/crc-xcel.h $(STAGING_DIR)/usr/include
	touch $(STAGING_DIR)/njd1.txt
endef

define CRC_XCEL_LIB_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/libcrc-xcel.so $(TARGET_DIR)/usr/lib
	$(INSTALL) -D -m 0755 $(@D)/crc-xcel-test $(TARGET_DIR)/usr/bin
endef

$(eval $(generic-package))
