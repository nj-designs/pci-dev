/*
 * File name: crc-xcel-test.c
 * Author: Neil Johnson
 * Date created: 2026-03-16 14:17:48
 * Date modified: 2026-03-16 14:17:52
 * ------
 */
#include <stdio.h>

#include "crc-xcel.h"

int main(int argc, char **argv) {
  struct crc_xcel_device device;
  struct crc_xcel_version_info version_info;
  const char *df;
  uint64_t crc_val;
  struct crc_xcel_gen_buffer_crc gen_buffer;
  uint8_t buffer[128];

  if (argc < 2) {
    fprintf(stderr, "Usage: crc-xcel-test <dev file>\n");
    return 1;
  }

  df = argv[1];

  if (crc_xcel_open(df, &device)) {
    fprintf(stderr, "Failed to open '%s'\n", argv[1]);
    return -1;
  }

  if (device.get_version(&device, &version_info)) {
    fprintf(stderr, "Failed to get version\n");
    crc_xcel_close(&device);
    return -1;
  }

  printf("HW version:0x%08X Drv Version:0x%08X\n", version_info.hw_version,
         version_info.drv_version);

  if (device.reset(&device)) {
    fprintf(stderr, "Failed to reset device\n");
    crc_xcel_close(&device);
    return -1;
  }

  if (device.read_crc(&device, &crc_val)) {
    fprintf(stderr, "Failed to read crc val\n");
    crc_xcel_close(&device);
    return -1;
  }

  printf("crc_val: 0x%lX\n", crc_val);

  gen_buffer.addr = (uint64_t)buffer;
  gen_buffer.len = sizeof(buffer);
  if (device.crc_buffer(&device, &gen_buffer)) {
    fprintf(stderr, "Failed to gen crc\n");
    crc_xcel_close(&device);
    return -1;
  }
  return 0;
}
