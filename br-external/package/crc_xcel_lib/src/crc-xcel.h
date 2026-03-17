/*
* File name: crc-xcel.h
* Author: Neil Johnson
* Date created: 2026-03-13 17:31:59
* Date modified: 2026-03-13 17:32:20
* ------
*/

#pragma once

#include <stdint.h>

#include <crc-xcel-drv.h>

struct crc_xcel_device {
  int fd;


  int (*get_version)(struct crc_xcel_device* device, struct crc_xcel_version_info* version_info);
  int (*reset)(struct crc_xcel_device* device);
  int (*read_crc)(struct crc_xcel_device* device, uint64_t* crc_val);
  int (*crc_buffer)(struct crc_xcel_device* device, struct crc_xcel_gen_buffer_crc *gen_buffer);
};

#ifdef __cplusplus
extern "C" {
#endif

int crc_xcel_open(const char* df, struct crc_xcel_device* device);
int crc_xcel_close(struct crc_xcel_device* device);


#ifdef __cplusplus
}
#endif
