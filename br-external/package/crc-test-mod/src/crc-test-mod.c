#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>

#include "pci-crcdev-regmap.h"

#define CRC_DEV_VENDOR_ID 0x1B36
#define CRC_DEV_DEVICE_ID 0x9999

#define CRC_DEV_DRV_NAME "crc_dev"

static const struct pci_device_id crc_test_dev_ids[] = {
    {PCI_DEVICE(CRC_DEV_VENDOR_ID, CRC_DEV_DEVICE_ID)},
    {},
};
MODULE_DEVICE_TABLE(pci, crc_test_dev_ids);

static struct {
  struct list_head devices;
  spinlock_t lock;
} crc_device_list;

struct crc_dev {
  struct list_head dev_list_node;
  char name[sizeof(CRC_DEV_DRV_NAME) + sizeof(":XXXX") + 1];
  void __iomem *reg_base;
  struct pci_dev *pdev;
  u16 uid;
};

static irqreturn_t crc_test_dev_isr(int irq, void *dev_id) {

  return IRQ_HANDLED;
}

static int crc_test_dev_probe(struct pci_dev *pdev,
                              const struct pci_device_id *id) {
  int error;
  void __iomem *reg_base;
  resource_size_t reg_len;
  u32 hw_version;
  struct crc_dev *crc_dev = NULL;

  printk("dev_crc_dev probe called 0x%04X:0x%04X\n", id->vendor, id->device);

  crc_dev = kmalloc(sizeof(*crc_dev), GFP_KERNEL | __GFP_ZERO);
  if (!crc_dev) {
    pr_err("Failed to allocate crc_dev");
    error = -ENOMEM;
    goto out_err;
  }
  crc_dev->pdev = pdev;
  crc_dev->uid = pci_dev_id(pdev);

  scnprintf(crc_dev->name, sizeof(crc_dev->name), "%s:%04X", CRC_DEV_DRV_NAME,
            crc_dev->uid);

  error = pcim_enable_device(pdev);
  if (error) {
    dev_err_probe(&pdev->dev, error, "Failed to enable device");
    goto out_err;
  }

  reg_base = pcim_iomap_region(pdev, 0, CRC_DEV_DRV_NAME);
  if (IS_ERR(reg_base)) {
    return dev_err_probe(&pdev->dev, PTR_ERR(reg_base),
                         "%s Failed to map reg IO memory", crc_dev->name);
  }
  crc_dev->reg_base = reg_base;

  reg_len = pci_resource_len(pdev, 0);
  if (reg_len != CRC_DEV_REG_MAP_SIZE) {
    // Using -ENOMEM below stops output string getting to logs...
    error =
        dev_err_probe(&pdev->dev, -EINVAL,
                      "%s Unexpected reg_map length. Got:%llu, expected:%llu",
                      crc_dev->name, reg_len, CRC_DEV_REG_MAP_SIZE);
    goto out_err;
  }

  if (!(pci_resource_flags(pdev, 0) & IORESOURCE_MEM)) {
    return dev_err_probe(&pdev->dev, -EIO, "%s Regmap not memory mapped",
                         crc_dev->name);
  }

  printk("%s reg_base:%px\n", crc_dev->name, reg_base);

  // Now read version register
  hw_version = ioread32(reg_base + CRC_DEV_VERSION_REG_OFFSET);
  printk("%s hw_ver:0x%08X\n", crc_dev->name, hw_version);

  pci_set_drvdata(pdev, crc_dev);
  spin_lock(&crc_device_list.lock);
  list_add_tail(&crc_dev->dev_list_node, &crc_device_list.devices);
  spin_unlock(&crc_device_list.lock);

  return 0;

out_err:
  if (!crc_dev) {
    kfree(crc_dev);
  }

  return error;
}

static void crc_test_dev_remove(struct pci_dev *pdev) {
  struct crc_dev *crc_dev;

  printk("dev_crc_dev removed called\n");

  crc_dev = (struct crc_dev *)pci_get_drvdata(pdev);

  spin_lock(&crc_device_list.lock);
  list_del(&crc_dev->dev_list_node);
  spin_unlock(&crc_device_list.lock);

  kfree(crc_dev);
};

static struct pci_driver crc_test_dev_driver = {
    .name = CRC_DEV_DRV_NAME,
    .id_table = crc_test_dev_ids,
    .probe = crc_test_dev_probe,
    .remove = crc_test_dev_remove,
};

static int __init crc_test_dev_init(void) {
  pr_info("CRC Test Device: Module loaded into the kernel\n");

  spin_lock_init(&crc_device_list.lock);
  INIT_LIST_HEAD(&crc_device_list.devices);

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
