#include "config.h"
#include "ap_connect.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TS_HEADER_LEN	2
#define TS_TRIGER_MAX_NUM	5000

static FILE *triger_fp = NULL;

static uint16_t triger_status_total = 0;

int triger_status_write(char *buf)
{
	unsigned char val[9];
	memcpy(val, buf, sizeof(val));
	if (triger_status_total >= TS_TRIGER_MAX_NUM) {
		log_msg("triger_status_write: triger_status_total >= TS_TAG_MAX_NUM");
		return -1;
	}
	fseek(triger_fp, TS_HEADER_LEN + triger_status_total*sizeof(val), SEEK_SET);
	size_t sz = fwrite(val, sizeof(val), 1, triger_fp);
	if (sz != 1) {
		log_msg("fwrite error");
		return -1;
	} else {
		triger_status_total++;
		fseek(triger_fp, 0, SEEK_SET);
		sz = fwrite(&triger_status_total, sizeof(triger_status_total), 1, triger_fp);
		if (sz != 1) {
			log_msg("fwrite error");
			triger_status_total--; /* 更新标签数失败,恢复标签数 */
			return -1;
		} else {
			log_msg("triger_storage_write: triger_status_total = %d", triger_status_total);
		}
	}
	return 0;
}

int triger_status_fflush(void)
{
	return fflush(triger_fp);
}

int triger_status_read(char *buf)
{
	if (triger_status_total == 0){
		return -1;
	}
	fseek(triger_fp, TS_HEADER_LEN + (triger_status_total-1)*9, SEEK_SET);
	
	size_t sz = fread(buf, 9, 1, triger_fp);
	if (sz != 1) {
		log_msg("fread error");
		return -1;
	}
	
	//log_msg("triger_status_read");

	return 0;
}

int triger_status_delete(bool all)
{
	if (triger_status_total == 0) {
		return -1;
	}

	if (all) {
		triger_status_total = 0;
	} else {
		triger_status_total--;
	}

	/* 1.更新标签数 */
	fseek(triger_fp, 0, SEEK_SET);
	size_t sz = fwrite(&triger_status_total, sizeof(triger_status_total), 1, triger_fp);
	if (sz != 1) {
		log_msg("fwrite error");
		triger_status_total++;	/* 更新标签数失败,恢复标签数 */
		return -1;
	} else {
		log_msg("triger_status_delete: triger_status_total = %d", triger_status_total);
		if (triger_status_total == 0) {
			triger_status_fflush();	/* 这不是很好的策略 */
		}
	}

	return 0;
}

uint16_t triger_status_get_cnt(void)
{
	return triger_status_total;
}

int triger_status_init(void)
{
	size_t sz;
	triger_fp = fopen("triger_status.bin", "r+");
	if (triger_fp) {
		sz = fread(&triger_status_total, sizeof(triger_status_total), 1, triger_fp);
		if (sz != 1) {
			log_msg("fread error");
			return -1;
		}else {
			log_msg("triger_status_init: triger_status_total = %d", triger_status_total);
		}
	} else {
		log_msg("triger_status.bin not exist, going to create it.");
		triger_fp = fopen("triger_status.bin", "w");	/* 创建文件 */
		if (triger_fp == NULL) {
			log_sys("fopen error w");
			return -1;
		}

		sz = fwrite(&triger_status_total, sizeof(triger_status_total), 1, triger_fp);
		if (sz != 1) {
			log_ret("frwite error, <sz = %d>", sz);
			return -1;
		}

		fclose(triger_fp);
		triger_fp = fopen("triger_status.bin", "r+");
		if (triger_fp == NULL) {
			log_sys("fopen error r+");
			return -1;
		}
	}
	return 0;
}
