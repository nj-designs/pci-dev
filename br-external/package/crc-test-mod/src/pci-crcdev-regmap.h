/*
 * File name: pci-crcdev-regmap.h
 * Author: Neil Johnson
 * Date created: 2026-03-09 15:58:57
 * Date modified: 2026-03-09 15:59:01
 * ------
 */

#pragma once

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

struct reg_map {
  uint64_t ctrl;
  uint64_t addr;
  uint64_t len;
  uint64_t crc;
};

#define CRC_DEV_REG_MAP_SIZE ((uint64_t)sizeof(struct reg_map))

#define CRC_DEV_CTRL_REG_ADDR ((hwaddr)offsetof(struct reg_map, ctrl))
#define CRC_DEV_ADDR_REG_ADDR ((hwaddr)offsetof(struct reg_map, addr))
#define CRC_DEV_LEN_REG_ADDR ((hwaddr)offsetof(struct reg_map, len))
#define CRC_DEV_CRC_REG_ADDR ((hwaddr)offsetof(struct reg_map, crc))
