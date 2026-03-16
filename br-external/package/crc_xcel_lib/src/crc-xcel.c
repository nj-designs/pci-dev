/*
 * File name: crc-xcel.c
 * Author: Neil Johnson
 * Date created: 2026-03-13 17:31:11
 * Date modified: 2026-03-13 17:32:07
 * ------
 */
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define CRC_XCEL_BLD_INTERNAL
#include "crc-xcel.h"

static int _get_version(struct crc_xcel_device *device,
                        struct crc_xcel_version_info *version_info);

int crc_xcel_open(const char *df, struct crc_xcel_device *device) {
  assert(df != NULL);
  assert(device != NULL);

  memset(device, 0, sizeof(*device));

  device->fd = open(df, O_RDWR);
  if (device->fd < 0) {
    return device->fd;
  }

  device->get_version = _get_version;

  return 0;
};

int crc_xcel_close(struct crc_xcel_device *device) {
  assert(device != NULL);

  close(device->fd);
  device->fd = -1;

  return 0;
}

static int _get_version(struct crc_xcel_device *device,
                        struct crc_xcel_version_info *version_info) {
  assert(device != NULL);
  assert(version_info != NULL);

  return ioctl(device->fd, CRC_XCEL_GET_VERSION_INFO, version_info);
}
