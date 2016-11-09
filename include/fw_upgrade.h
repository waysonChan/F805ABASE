#ifndef _FW_UPGRADE_H
#define _FW_UPGRADE_H

#include "ap_connect.h"
#include "config.h"

#define FILE_TYPE_R2000				0x00	/* R2000's Firmware */
#define FILE_TYPE_LINUX_FIRMWARE	0x01	/* Reader's Programs(include user space app and driver) */
#define FILE_TYPE_LINUX_FILE		0x02	/* Reader's configure file and linux system configure file */
#define MAX_FILE_NAME_LEN	64
#define MAX_FILE_LEN		(512*1024)
#define MAX_UPGRADE_CMD_LEN	128

uint32_t fast_crc32(uint32_t sum, uint8_t *p, uint32_t len);
int upgrade_f860(ap_connect_t *A, const char *file_name);
int upgrade_linux_file(const char *file_name);

#endif	/* _FW_UPGRADE_H */
