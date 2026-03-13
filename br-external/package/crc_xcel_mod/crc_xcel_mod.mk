# crc module
#
CRC_XCEL_MOD_VERSION = 0.1
CRC_XCEL_MOD_SITE = $(BR2_EXTERNAL_PCI_DEV_PATH)/package/crc_xcel_mod/src
CRC_XCEL_MOD_SITE_METHOD = local

$(eval $(kernel-module))
$(eval $(generic-package))
