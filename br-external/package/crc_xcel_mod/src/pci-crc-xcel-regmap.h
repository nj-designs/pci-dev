/*
 * File name: pci-crc-xcel-regmap.h
 * Author: Neil Johnson
 * Date created: 2026-03-13 14:30:23
 * Date modified: 2026-03-13 14:42:02
 * ------
 */

#pragma once

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

struct reg_map {
  uint32_t version;
  uint32_t ctrl;
  uint64_t addr;
  uint64_t len;
  uint64_t crc;
};

#define CRC_XCEL_REG_MAP_SIZE ((uint64_t)sizeof(struct reg_map))

#define CRC_XCEL_VERSION_REG_OFFSET                                            \
  ((uint64_t)offsetof(struct reg_map, version))
#define CRC_XCEL_CTRL_REG_OFFSET ((uint64_t)offsetof(struct reg_map, ctrl))
#define CRC_XCEL_ADDR_REG_OFFSET ((uint64_t)offsetof(struct reg_map, addr))
#define CRC_XCEL_LEN_REG_OFFSET ((uint64_t)offsetof(struct reg_map, len))
#define CRC_XCEL_CRC_REG_OFFSET ((uint64_t)offsetof(struct reg_map, crc))
