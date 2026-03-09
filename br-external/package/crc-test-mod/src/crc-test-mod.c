#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>

#include "pci-crcdev-regmap.h"

#define CRC_TEST_DEV_VENDOR_ID 0x1B36
#define CRC_TEST_DEV_DEVICE_ID 0x9999

static struct pci_device_id crc_test_dev_ids[] = {
    {PCI_DEVICE(CRC_TEST_DEV_VENDOR_ID, CRC_TEST_DEV_DEVICE_ID)},
    {},
};
MODULE_DEVICE_TABLE(pci, crc_test_dev_ids);

static int crc_test_dev_probe(struct pci_dev *dev,
                              const struct pci_device_id *id) {
  printk("dev_crc_dev probe called\n");
  return 0;
}

static void crc_test_dev_remove(struct pci_dev *dev) {

  printk("dev_crc_dev removed called\n");
};

static struct pci_driver crc_test_dev_driver = {
    .name = "crc_test_dev",
    .id_table = crc_test_dev_ids,
    .probe = crc_test_dev_probe,
    .remove = crc_test_dev_remove,
};

static int __init crc_test_dev_init(void) {
  pr_info("CRC Test Device: Module loaded into the kernel\n");
  return pci_register_driver(&crc_test_dev_driver);
}

static void __exit crc_test_dev_exit(void) {
  pr_info("CRC Test Device: Module removed\n");
  pci_unregister_driver(&crc_test_dev_driver);
}

module_init(crc_test_dev_init);
module_exit(crc_test_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Neil Johnson");
MODULE_DESCRIPTION("Kernel driver for CRC test PCI device");
