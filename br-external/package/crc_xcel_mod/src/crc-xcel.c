#include <linux/cdev.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/completion.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/io-64-nonatomic-lo-hi.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>

#define CRC_XCEL_BLD_INTERNAL
#include "crc-xcel-drv.h"

#define CRC_XCEL_VENDOR_ID 0x1B36
#define CRC_XCEL_DEVICE_ID 0x9999
#define CRC_XCEL_MAX_DEVICE_CNT 4

#define CRC_XCEL_DRV_NAME "crc_xcel"

/*
 **********************************************************************
 * Copied from qemu crc xcel device source
 */
struct reg_map {
  uint32_t version;
  uint32_t ctrl;
  uint32_t status;
  uint64_t addr;
  uint64_t len;
  uint64_t crc;
} __attribute__((aligned(64)));

#define CRC_XCEL_DEV_REG_MAP_SIZE ((uint64_t)sizeof(struct reg_map))

#define CRC_XCEL_DEV_VERSION_REG_OFFSET ((uint64_t)offsetof(struct reg_map, version))
#define CRC_XCEL_DEV_CTRL_REG_OFFSET ((uint64_t)offsetof(struct reg_map, ctrl))
#define CRC_XCEL_DEV_STATUS_REG_OFFSET ((uint64_t)offsetof(struct reg_map, status))
#define CRC_XCEL_DEV_ADDR_REG_OFFSET ((uint64_t)offsetof(struct reg_map, addr))
#define CRC_XCEL_DEV_LEN_REG_OFFSET ((uint64_t)offsetof(struct reg_map, len))
#define CRC_XCEL_DEV_CRC_REG_OFFSET ((uint64_t)offsetof(struct reg_map, crc))

// Status register defines
#define CRC_XCEL_DEV_STATUS_REG_RUNNING (1 << 0)
/*
 **********************************************************************
 */

static const struct pci_device_id crc_test_dev_ids[] = {
    {PCI_DEVICE(CRC_XCEL_VENDOR_ID, CRC_XCEL_DEVICE_ID)},
    {},
};
MODULE_DEVICE_TABLE(pci, crc_test_dev_ids);

struct crc_dev {
  char              name[sizeof(CRC_XCEL_DRV_NAME) + sizeof("xx") + 1];
  void __iomem*     reg_base;
  struct pci_dev*   pdev;
  int               irq;
  int               idx;
  struct cdev       cdev;
  struct completion irq_comp;
};

static struct {
  struct {
    struct crc_dev* refs[CRC_XCEL_MAX_DEVICE_CNT];
    spinlock_t      lock;
  } devices;

  dev_t crc_xcel_number;
  struct class* class;
} crc_drv_ctx;

static void crc_xcel_writeq(u64 val, volatile void __iomem* addr) {
#ifdef writeq
  // pr_info("writeq() val:%llx addr:0x%p\n", val, addr);
  writeq(val, addr);
#else
#error Port me!!
#endif
}

static u64 crc_xcel_readq(volatile void __iomem* addr) {
#ifdef readq
  return readq(addr);
#else
#error Port me!!
#endif
}

static int crc_xcel_open(struct inode* inode, struct file* file) {
  struct crc_dev* crc_dev;

  crc_dev = container_of(inode->i_cdev, struct crc_dev, cdev);

  file->private_data = crc_dev;

  pr_info("%s open called\n", crc_dev->name);

  return 0;
}

static int crc_xcel_release(struct inode* inode, struct file* file) {
  struct crc_dev* crc_dev;

  crc_dev = (struct crc_dev*)file->private_data;

  pr_info("%s release called\n", crc_dev->name);

  return 0;
}

static int crc_xcel_crc_buffer(struct crc_dev* crc_dev, const struct crc_xcel_gen_buffer_crc* gen_buffer) {
  int                 error = 0;
  int                 nr_pages, mapped_cnt, idx;
  struct page**       pages = NULL;
  long                nr_pages_pinned, pinned_page_idx;
  struct scatterlist* sg = NULL;
  dma_addr_t          dma_addr;
  size_t              dma_len;
  u32                 bytes_remaining = gen_buffer->len;

  pr_info("%s crc buffer ptr:0x%llx len:%u\n", crc_dev->name, gen_buffer->addr, gen_buffer->len);

  if (gen_buffer->addr & (PAGE_SIZE - 1)) {
    pr_err("%s buffer not not %lu byte aligned\n", crc_dev->name, PAGE_SIZE);
    error = -EFAULT;
    goto err_exit;
  }

  nr_pages = DIV_ROUND_UP(gen_buffer->len, PAGE_SIZE);

  pages = kcalloc(nr_pages, sizeof(*pages), GFP_KERNEL);
  if (!pages) {
    error = -ENOMEM;
    goto err_exit;
  }

  nr_pages_pinned = pin_user_pages_fast(gen_buffer->addr, nr_pages, FOLL_WRITE | FOLL_LONGTERM, pages);
  if (nr_pages_pinned < 0) {
    error = nr_pages_pinned;
    goto err_exit_free_pages;
  }
  if (nr_pages_pinned != nr_pages) {
    pr_err("%s wrong pinned page cnt (%ld/%d)\n", crc_dev->name, nr_pages_pinned, nr_pages);
    error = -ENOMEM;
    goto err_exit_unpin;
  }
  pr_info("%s nr_pages:%d\n", crc_dev->name, nr_pages);

  sg = kcalloc(nr_pages_pinned, sizeof(*sg), GFP_KERNEL);
  if (!sg) {
    pr_err("%s failed to allocate sg memory\n", crc_dev->name);
    error = -ENOMEM;
    goto err_exit_unpin;
  }
  sg_init_marker(sg, nr_pages_pinned);  // Do need sg_init_table() as memory already zero'd by kcalloc

  for (pinned_page_idx = 0; pinned_page_idx < nr_pages_pinned; pinned_page_idx++) {
    sg_set_page(&sg[pinned_page_idx], pages[pinned_page_idx], PAGE_SIZE, 0);
  }

  mapped_cnt = dma_map_sg(&crc_dev->pdev->dev, sg, nr_pages, DMA_TO_DEVICE);
  if (!mapped_cnt) {
    error = -EIO;
    goto err_exit_free_sg;
  }

  for (idx = 0; idx < mapped_cnt; idx++) {
    dma_addr = sg_dma_address(&sg[idx]);
    dma_len  = sg_dma_len(&sg[idx]);

    if (dma_len > bytes_remaining) {
      dma_len = bytes_remaining;
    }
    bytes_remaining -= dma_len;
    pr_info("%s dma idx:%d addr:0x%llX len:%lu\n", crc_dev->name, idx, dma_addr, dma_len);
    crc_xcel_writeq(dma_addr, crc_dev->reg_base + CRC_XCEL_DEV_ADDR_REG_OFFSET);
    crc_xcel_writeq(dma_len, crc_dev->reg_base + CRC_XCEL_DEV_LEN_REG_OFFSET);
    writel(0xBEAF, crc_dev->reg_base + CRC_XCEL_DEV_CTRL_REG_OFFSET);

    error = wait_for_completion_interruptible(&crc_dev->irq_comp);
    if (error) {
      pr_err("%s interrupted waiting for IRQ\n", crc_dev->name);
      goto err_exit_unmap;
    }
    reinit_completion(&crc_dev->irq_comp);
  }

err_exit_unmap:
  dma_unmap_sg(&crc_dev->pdev->dev, sg, nr_pages, DMA_TO_DEVICE);

err_exit_free_sg:
  if (sg) {
    kfree(sg);
  }
err_exit_unpin:
  for (pinned_page_idx = 0; pinned_page_idx < nr_pages_pinned; pinned_page_idx++) {
    unpin_user_page(pages[pinned_page_idx]);
  }
err_exit_free_pages:
  if (pages) {
    kfree(pages);
  }
err_exit:
  return error;
}

static long crc_xcel_ioctl(struct file* file, unsigned int cmd, unsigned long arg) {
  int             error   = 0;
  void __user*    argp    = (void __user*)arg;
  struct crc_dev* crc_dev = file->private_data;

  switch (cmd) {
    case CRC_XCEL_GET_VERSION_INFO: {
      struct crc_xcel_version_info version_info;

      memset(&version_info, 0, sizeof(version_info));

      version_info.hw_version  = ioread32(crc_dev->reg_base + CRC_XCEL_DEV_VERSION_REG_OFFSET);
      version_info.drv_version = 0x55;

      error = copy_to_user(argp, &version_info, sizeof(version_info));
      break;
    }

    case CRC_XCEL_RESET_CRC: {
      const uint64_t crc_default = (uint64_t)0xBDFFFFFFFA;
      crc_xcel_writeq(crc_default, crc_dev->reg_base + CRC_XCEL_DEV_CRC_REG_OFFSET);
      break;
    }

    case CRC_XCEL_READ_CRC: {
      uint64_t crc_val = crc_xcel_readq(crc_dev->reg_base + CRC_XCEL_DEV_CRC_REG_OFFSET);
      error            = copy_to_user(argp, &crc_val, sizeof(uint64_t));
      break;
    }

    case CRC_XCEL_CRC_BUFFER: {
      struct crc_xcel_gen_buffer_crc gen_buffer;
      error = copy_from_user(&gen_buffer, argp, sizeof(gen_buffer));
      if (error) {
        break;
      }
      error = crc_xcel_crc_buffer(crc_dev, &gen_buffer);
      break;
    }

    default: {
      error = -ENOTTY;
    }
  }

  return error;
}

static irqreturn_t crc_test_dev_isr(int irq, void* dev_id) {
  struct crc_dev* crc_dev = (struct crc_dev*)dev_id;

  pr_info("IRQ recevied\n");
  complete(&crc_dev->irq_comp);
  return IRQ_HANDLED;
}

static struct file_operations crc_xcel_fops = {
    .owner          = THIS_MODULE,
    .open           = crc_xcel_open,
    .unlocked_ioctl = crc_xcel_ioctl,
    .release        = crc_xcel_release,

};

static int crc_test_dev_probe(struct pci_dev* pdev, const struct pci_device_id* id) {
  int             error, idx, minor;
  void __iomem*   reg_base;
  resource_size_t reg_len;
  u32             hw_version;
  struct crc_dev* crc_dev = NULL;

  pr_info("%s probe device:0x%04X:0x%04X\n", CRC_XCEL_DRV_NAME, id->vendor, id->device);

  crc_dev = kmalloc(sizeof(*crc_dev), GFP_KERNEL | __GFP_ZERO);
  if (!crc_dev) {
    pr_err("Failed to allocate crc_dev");
    error = -ENOMEM;
    goto out_free_dev_priv;
  }

  spin_lock(&crc_drv_ctx.devices.lock);
  for (idx = 0; idx < CRC_XCEL_MAX_DEVICE_CNT; idx++) {
    if (!crc_drv_ctx.devices.refs[idx]) {
      break;
    }
  }
  spin_unlock(&crc_drv_ctx.devices.lock);

  if (idx == CRC_XCEL_MAX_DEVICE_CNT) {
    pr_err("%s too many devices\n", CRC_XCEL_DRV_NAME);
    error = -ENOSPC;
    goto out_free_dev_priv;
  }

  crc_dev->idx  = idx;
  crc_dev->pdev = pdev;
  scnprintf(crc_dev->name, sizeof(crc_dev->name), "%s%d", CRC_XCEL_DRV_NAME, crc_dev->idx);
  init_completion(&crc_dev->irq_comp);

  error = pcim_enable_device(pdev);
  if (error) {
    dev_err_probe(&pdev->dev, error, "Failed to enable device");
    goto out_free_dev_priv;
  }

  reg_base = pcim_iomap_region(pdev, 0, crc_dev->name);
  if (IS_ERR(reg_base)) {
    error = dev_err_probe(&pdev->dev, PTR_ERR(reg_base), "%s Failed to map reg IO memory", crc_dev->name);
    goto out_free_dev_priv;
  }
  crc_dev->reg_base = reg_base;
  pr_info("%s reg_base:%px\n", crc_dev->name, reg_base);

  reg_len = pci_resource_len(pdev, 0);
  if (reg_len != CRC_XCEL_DEV_REG_MAP_SIZE) {
    // Using -ENOMEM below stops output string getting to logs...
    error = dev_err_probe(&pdev->dev, -EINVAL, "%s Unexpected reg_map length. Got:%llu, expected:%llu", crc_dev->name, reg_len,
                          CRC_XCEL_DEV_REG_MAP_SIZE);
    goto out_free_dev_priv;
  }

  if (!(pci_resource_flags(pdev, 0) & IORESOURCE_MEM)) {
    error = dev_err_probe(&pdev->dev, -EIO, "%s Regmap not memory mapped", crc_dev->name);
    goto out_free_dev_priv;
  }

  error = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_ALL_TYPES);
  if (error != 1) {
    // We're only expecting one
    error = dev_err_probe(&pdev->dev, -ENOSPC, "%s failed to allocate irq vectors", crc_dev->name);
    goto out_free_dev_priv;
  }

  crc_dev->irq = pci_irq_vector(pdev, 0);
  pr_info("%s irq:%d\n", crc_dev->name, crc_dev->irq);

  error = request_irq(crc_dev->irq, crc_test_dev_isr, 0, crc_dev->name, (void*)crc_dev);
  if (error) {
    dev_err_probe(&pdev->dev, error, "%s failed to request IRQ:%d\n", crc_dev->name, crc_dev->irq);
    goto out_free_irq;
  }

  // chrdev stuff
  cdev_init(&crc_dev->cdev, &crc_xcel_fops);
  crc_dev->cdev.owner = THIS_MODULE;
  minor               = crc_drv_ctx.crc_xcel_number + crc_dev->idx;
  error               = cdev_add(&crc_dev->cdev, minor, 1);
  if (error) {
    pr_err("%s failed to add cdev\n", crc_dev->name);
    goto out_free_irq;
  }
  device_create(crc_drv_ctx.class, NULL, MKDEV(MAJOR(crc_drv_ctx.crc_xcel_number), minor), crc_dev, crc_dev->name);

  error = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
  if (error) {
    dev_err_probe(&pdev->dev, error, "%s failed to set DMA mask\n", crc_dev->name);
    goto out_free_irq;
  }

  // Now read version register
  hw_version = ioread32(reg_base + CRC_XCEL_DEV_VERSION_REG_OFFSET);
  pr_info("%s hw_ver:0x%08X\n", crc_dev->name, hw_version);

  pci_set_drvdata(pdev, crc_dev);

  crc_drv_ctx.devices.refs[crc_dev->idx] = crc_dev;

  pci_set_master(pdev);

  return 0;

out_free_irq:
  pci_free_irq_vectors(pdev);
out_free_dev_priv:
  if (!crc_dev) {
    kfree(crc_dev);
  }

  return error;
}

static void crc_test_dev_remove(struct pci_dev* pdev) {
  struct crc_dev* crc_dev = (struct crc_dev*)pci_get_drvdata(pdev);

  pr_info("%s remove\n", crc_dev->name);

  crc_drv_ctx.devices.refs[crc_dev->idx] = NULL;

  free_irq(crc_dev->irq, (void*)crc_dev);
  pci_free_irq_vectors(pdev);

  device_destroy(crc_drv_ctx.class, MKDEV(MAJOR(crc_drv_ctx.crc_xcel_number), crc_dev->idx));
  cdev_del(&crc_dev->cdev);

  pci_set_drvdata(pdev, NULL);
  kfree(crc_dev);
};

static struct pci_driver crc_test_dev_driver = {
    .name     = CRC_XCEL_DRV_NAME,
    .id_table = crc_test_dev_ids,
    .probe    = crc_test_dev_probe,
    .remove   = crc_test_dev_remove,
};

static int __init crc_test_dev_init(void) {
  int error;

  pr_info("%s module_init\n", CRC_XCEL_DRV_NAME);

  crc_drv_ctx.class = class_create(CRC_XCEL_DRV_NAME);

  error = alloc_chrdev_region(&crc_drv_ctx.crc_xcel_number, 0, CRC_XCEL_MAX_DEVICE_CNT, CRC_XCEL_DRV_NAME);
  if (error) {
    pr_err("%s failed to alloc chrdevs\n", CRC_XCEL_DRV_NAME);
    goto out;
  }

  error = pci_register_driver(&crc_test_dev_driver);
  if (error) {
    pr_err("%s failed to register driver\n", CRC_XCEL_DRV_NAME);
    goto out;
  }

  return 0;

out:
  class_destroy(crc_drv_ctx.class);
  return error;
}

static void __exit crc_test_dev_exit(void) {
  pr_info("%s module_exit\n", CRC_XCEL_DRV_NAME);

  pci_unregister_driver(&crc_test_dev_driver);
  unregister_chrdev_region(crc_drv_ctx.crc_xcel_number, CRC_XCEL_MAX_DEVICE_CNT);
  class_destroy(crc_drv_ctx.class);
}

module_init(crc_test_dev_init);
module_exit(crc_test_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Neil Johnson");
MODULE_DESCRIPTION("Kernel driver for CRC test PCI device");
