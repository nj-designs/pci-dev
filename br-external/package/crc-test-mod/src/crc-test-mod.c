#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>

#include "pci-crcdev-regmap.h"

#define CRC_DEV_VENDOR_ID 0x1B36
#define CRC_DEV_DEVICE_ID 0x9999
#define CRC_DEV_MAX_DEVICE_CNT 4

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
  int irq;
  struct cdev cdev;
};

static dev_t crc_dev_number;
static int crc_dev_open(struct inode *inode, struct file *file) { return 0; }

static int crc_dev_release(struct inode *inode, struct file *file) { return 0; }

static irqreturn_t crc_test_dev_isr(int irq, void *dev_id) {

  return IRQ_HANDLED;
}

static struct file_operations crc_dev_fops = {
    .owner = THIS_MODULE,
    .open = crc_dev_open,
    .release = crc_dev_release,

};

static int crc_test_dev_probe(struct pci_dev *pdev,
                              const struct pci_device_id *id) {
  int error;
  void __iomem *reg_base;
  resource_size_t reg_len;
  u32 hw_version;
  struct crc_dev *crc_dev = NULL;

  pr_info("%s probe device:0x%04X:0x%04X\n", CRC_DEV_DRV_NAME, id->vendor,
          id->device);

  crc_dev = kmalloc(sizeof(*crc_dev), GFP_KERNEL | __GFP_ZERO);
  if (!crc_dev) {
    pr_err("Failed to allocate crc_dev");
    error = -ENOMEM;
    goto out_free_dev_priv;
  }

  crc_dev->pdev = pdev;
  crc_dev->uid = pci_dev_id(pdev);
  scnprintf(crc_dev->name, sizeof(crc_dev->name), "%s:%04X", CRC_DEV_DRV_NAME,
            crc_dev->uid);

  error = pcim_enable_device(pdev);
  if (error) {
    dev_err_probe(&pdev->dev, error, "Failed to enable device");
    goto out_free_dev_priv;
  }

  reg_base = pcim_iomap_region(pdev, 0, CRC_DEV_DRV_NAME);
  if (IS_ERR(reg_base)) {
    error = dev_err_probe(&pdev->dev, PTR_ERR(reg_base),
                          "%s Failed to map reg IO memory", crc_dev->name);
    goto out_free_dev_priv;
  }
  crc_dev->reg_base = reg_base;
  pr_info("%s reg_base:%px\n", crc_dev->name, reg_base);

  reg_len = pci_resource_len(pdev, 0);
  if (reg_len != CRC_DEV_REG_MAP_SIZE) {
    // Using -ENOMEM below stops output string getting to logs...
    error =
        dev_err_probe(&pdev->dev, -EINVAL,
                      "%s Unexpected reg_map length. Got:%llu, expected:%llu",
                      crc_dev->name, reg_len, CRC_DEV_REG_MAP_SIZE);
    goto out_free_dev_priv;
  }

  if (!(pci_resource_flags(pdev, 0) & IORESOURCE_MEM)) {
    error = dev_err_probe(&pdev->dev, -EIO, "%s Regmap not memory mapped",
                          crc_dev->name);
    goto out_free_dev_priv;
  }

  error = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_ALL_TYPES);
  if (error != 1) {
    // We're only expecting one
    error = dev_err_probe(&pdev->dev, -ENOSPC,
                          "%s failed to allocate irq vectors", crc_dev->name);
    goto out_free_dev_priv;
  }

  crc_dev->irq = pci_irq_vector(pdev, 0);
  pr_info("%s irq:%d\n", crc_dev->name, crc_dev->irq);

  error = request_irq(crc_dev->irq, crc_test_dev_isr, 0, crc_dev->name,
                      (void *)crc_dev);
  if (error) {
    dev_err_probe(&pdev->dev, error, "%s failed to request IRQ:%d\n",
                  crc_dev->name, crc_dev->irq);
    goto out_free_irq;
  }

  cdev_init(&crc_dev->cdev, &crc_dev_fops);
  crc_dev->cdev.owner = THIS_MODULE;

  // Now read version register
  hw_version = ioread32(reg_base + CRC_DEV_VERSION_REG_OFFSET);
  pr_info("%s hw_ver:0x%08X\n", crc_dev->name, hw_version);

  pci_set_drvdata(pdev, crc_dev);

  spin_lock(&crc_device_list.lock);
  list_add_tail(&crc_dev->dev_list_node, &crc_device_list.devices);
  spin_unlock(&crc_device_list.lock);

  return 0;

out_free_irq:
  pci_free_irq_vectors(pdev);
out_free_dev_priv:
  if (!crc_dev) {
    kfree(crc_dev);
  }

  return error;
}

static void crc_test_dev_remove(struct pci_dev *pdev) {
  struct crc_dev *crc_dev = (struct crc_dev *)pci_get_drvdata(pdev);

  pr_info("%s remove\n", crc_dev->name);

  spin_lock(&crc_device_list.lock);
  list_del(&crc_dev->dev_list_node);
  spin_unlock(&crc_device_list.lock);

  free_irq(crc_dev->irq, (void *)crc_dev);
  pci_free_irq_vectors(pdev);

  pci_set_drvdata(pdev, NULL);

  kfree(crc_dev);
};

static struct pci_driver crc_test_dev_driver = {
    .name = CRC_DEV_DRV_NAME,
    .id_table = crc_test_dev_ids,
    .probe = crc_test_dev_probe,
    .remove = crc_test_dev_remove,
};

static int __init crc_test_dev_init(void) {
  int error;

  spin_lock_init(&crc_device_list.lock);
  INIT_LIST_HEAD(&crc_device_list.devices);

  error = alloc_chrdev_region(&crc_dev_number, 0, CRC_DEV_MAX_DEVICE_CNT,
                              CRC_DEV_DRV_NAME);
  if (error) {
    pr_err("%s failed to alloc chrdevs\n", CRC_DEV_DRV_NAME);
    goto out;
  }

  error = pci_register_driver(&crc_test_dev_driver);
  if (error) {
    pr_err("%s failed to register driver\n", CRC_DEV_DRV_NAME);
    goto out;
  }

  pr_info("%s register driver\n", CRC_DEV_DRV_NAME);
out:
  return error;
}

static void __exit crc_test_dev_exit(void) {
  pr_info("CRC Test Device: Module removed\n");
  pci_unregister_driver(&crc_test_dev_driver);
  unregister_chrdev_region(crc_dev_number, CRC_DEV_MAX_DEVICE_CNT);
}

module_init(crc_test_dev_init);
module_exit(crc_test_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Neil Johnson");
MODULE_DESCRIPTION("Kernel driver for CRC test PCI device");
