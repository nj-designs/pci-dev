/*
* File name: crc-xcel.h
* Author: Neil Johnson
* Date created: 2026-03-16 09:30:09
* Date modified: 2026-03-16 09:33:07
* ------
*/

#pragma once

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

struct crc_xcel_version_info {
  uint32_t hw_version;
  uint32_t drv_version;
};

struct crc_xcel_gen_buffer_crc {
  uint64_t addr;
  uint32_t len;
};

#ifdef CRC_XCEL_BLD_INTERNAL
#define CRC_XCEL_IOCTL_MAGIC 'C'
#define CRC_XCEL_GET_VERSION_INFO _IOR(CRC_XCEL_IOCTL_MAGIC, 1, struct crc_xcel_version_info)
#define CRC_XCEL_RESET_CRC _IO(CRC_XCEL_IOCTL_MAGIC, 2)
#define CRC_XCEL_READ_CRC _IOR(CRC_XCEL_IOCTL_MAGIC, 3, uint64_t)
#define CRC_XCEL_CRC_BUFFER _IOW(CRC_XCEL_IOCTL_MAGIC, 4, struct crc_xcel_gen_buffer_crc)
#endif
