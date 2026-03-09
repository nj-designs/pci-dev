# crc module
#
CRC_TEST_MOD_VERSION = 0.1
CRC_TEST_MOD_SITE = $(BR2_EXTERNAL_PCI_DEV_PATH)/package/crc-test-mod/src
CRC_TEST_MOD_SITE_METHOD = local

$(eval $(kernel-module))
$(eval $(generic-package))
