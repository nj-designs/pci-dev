/*
 * File name: crc-xcel-test.c
 * Author: Neil Johnson
 * Date created: 2026-03-16 14:17:48
 * Date modified: 2026-03-16 14:17:52
 * ------
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "crc-xcel.h"

#define BUFFER_SIZE 8000

int main(int argc, char** argv) {
  int                    error  = EXIT_FAILURE;
  struct crc_xcel_device device = {
      0,
  };
  struct crc_xcel_version_info   version_info;
  const char*                    df = NULL;
  uint64_t                       crc_val;
  struct crc_xcel_gen_buffer_crc gen_buffer;
  uint8_t*                       buffer = NULL;

  if (argc < 2) {
    fprintf(stderr, "Usage: crc-xcel-test <dev file>\n");
    goto out;
  }

  buffer = aligned_alloc(getpagesize(), BUFFER_SIZE);
  assert(buffer != NULL);

  buffer[4096] = 0xE4;

  df = argv[1];

  if (crc_xcel_open(df, &device)) {
    fprintf(stderr, "Failed to open '%s'\n", argv[1]);
    goto out;
  }

  if (device.get_version(&device, &version_info)) {
    fprintf(stderr, "Failed to get version\n");
    goto out;
  }

  printf("HW version:0x%08X Drv Version:0x%08X\n", version_info.hw_version, version_info.drv_version);

  if (device.reset(&device)) {
    fprintf(stderr, "Failed to reset device\n");
    goto out;
  }

  if (device.read_crc(&device, &crc_val)) {
    fprintf(stderr, "Failed to read crc val\n");
    goto out;
  }

  printf("crc_val: 0x%lX\n", crc_val);

  gen_buffer.addr = (uint64_t)buffer;
  gen_buffer.len  = BUFFER_SIZE;
  if (device.crc_buffer(&device, &gen_buffer)) {
    fprintf(stderr, "Failed to gen crc\n");
    goto out;
  }

  error = EXIT_SUCCESS;

out:
  if (device.fd > 0) {
    crc_xcel_close(&device);
  }

  if (buffer) {
    free(buffer);
  }

  return error;
}
