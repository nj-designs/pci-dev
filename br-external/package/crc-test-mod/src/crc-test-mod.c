#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>

#include "pci-crcdev-regmap.h"

#define CRC_TEST_DEV_VENDOR_ID 0x1B36
#define CRC_TEST_DEV_DEVICE_ID 0x9999

#define CRC_TEST_DEV_DRV_NAME "crc_test_dev"

static const struct pci_device_id crc_test_dev_ids[] = {
    {PCI_DEVICE(CRC_TEST_DEV_VENDOR_ID, CRC_TEST_DEV_DEVICE_ID)},
    {},
};
MODULE_DEVICE_TABLE(pci, crc_test_dev_ids);

static int crc_test_dev_probe(struct pci_dev *pdev,
                              const struct pci_device_id *id) {
  void __iomem *reg_base;
  resource_size_t reg_len;
  u32 hw_version;

  int error = -ENODEV;

  printk("dev_crc_dev probe called 0x%04X:0x%04X\n", id->vendor, id->device);

  error = pcim_enable_device(pdev);
  if (error) {
    return dev_err_probe(&pdev->dev, error, "Failed to enable device");
  }

  reg_base = pcim_iomap_region(pdev, 0, CRC_TEST_DEV_DRV_NAME);
  if (IS_ERR(reg_base)) {
    return dev_err_probe(&pdev->dev, PTR_ERR(reg_base),
                         "Failed to map reg IO memory");
  }

  reg_len = pci_resource_len(pdev, 0);
  if (reg_len == 0) {
    // Using -ENOMEM below stops output string getting to logs...
    return dev_err_probe(&pdev->dev, -ENOSPC, "Zero length reg IO map");
  }

  if (!(pci_resource_flags(pdev, 0) & IORESOURCE_MEM)) {
    return dev_err_probe(&pdev->dev, -EIO, "Regmap not memory mapped");
  }

  printk("dev_crc_dev reg_base:%px reg_size:%llu\n", reg_base, reg_len);

  // Now read version register
  hw_version = ioread32(reg_base + CRC_DEV_VERSION_REG_OFFSET);
  printk("dev_crc_dev hw_ver:0x%08X\n", hw_version);

  return 0;
}

static void crc_test_dev_remove(struct pci_dev *dev) {

  printk("dev_crc_dev removed called\n");
};

static struct pci_driver crc_test_dev_driver = {
    .name = CRC_TEST_DEV_DRV_NAME,
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
