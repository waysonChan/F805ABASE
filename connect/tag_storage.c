#include "config.h"
#include "ap_connect.h"

#include <stdio.h>
#include <string.h>

#define TS_HEADER_LEN	2
#define TS_TAG_DATA_LEN	26
#define TS_TAG_MAX_NUM	5000

static FILE *fp = NULL;
static uint16_t tag_storage_total = 0;

typedef struct {
	uint8_t ant_idx;
	uint8_t tag_len;
	time_t first_time;
	uint8_t data[TS_TAG_DATA_LEN];
} tag_storage_t;

int tag_storage_write(tag_t *ptag)
{
	if (tag_storage_total >= TS_TAG_MAX_NUM) {
		log_msg("tag_storage_write: tag_storage_total >= TS_TAG_MAX_NUM");
		return -1;
	}
	
	tag_storage_t ts = {
		.ant_idx = ptag->ant_index,
		.tag_len = ptag->tag_len,
		.first_time = ptag->first_time,
	};

	ts.tag_len = (ts.tag_len > TS_TAG_DATA_LEN) ? TS_TAG_DATA_LEN : ts.tag_len;
	memcpy(ts.data, ptag->data, ts.tag_len);

	/* 1.写标签数据 */
	fseek(fp, TS_HEADER_LEN + tag_storage_total*sizeof(tag_storage_t), SEEK_SET);
	size_t sz = fwrite(&ts, sizeof(ts), 1, fp);
	if (sz != 1) {
		log_msg("fwrite error");
		return -1;
	} else {
		/* 2.更新标签数 */
		tag_storage_total++;
		fseek(fp, 0, SEEK_SET);
		sz = fwrite(&tag_storage_total, sizeof(tag_storage_total), 1, fp);
		if (sz != 1) {
			log_msg("fwrite error");
			tag_storage_total--; /* 更新标签数失败,恢复标签数 */
			return -1;
		} else {
			log_msg("tag_storage_write: tag_storage_total = %d", tag_storage_total);
		}
	}

	return 0;
}

int tag_storage_fflush(void)
{
	return fflush(fp);
}

int tag_storage_read(tag_t *ptag)
{
	if (tag_storage_total == 0)
		return -1;

	fseek(fp, TS_HEADER_LEN + (tag_storage_total-1)*sizeof(tag_storage_t), SEEK_SET);

	tag_storage_t ts;
	size_t sz = fread(&ts, sizeof(ts), 1, fp);
	if (sz != 1) {
		log_msg("fread error");
		return -1;
	}

	ptag->ant_index = ts.ant_idx;
	ptag->tag_len = ts.tag_len;
	ptag->first_time = ts.first_time;
	memcpy(ptag->data, ts.data, ts.tag_len);

	return 0;
}

int tag_storage_delete(void)
{
	size_t sz = 0;
	if (tag_storage_total == 0) {
		log_msg("tag_storage_delete: tag_storage_total == 0");
		return -1;
	} else {
		tag_storage_total--;
	}

	/* 1.更新标签数 */
	fseek(fp, 0, SEEK_SET);
	sz = fwrite(&tag_storage_total, sizeof(tag_storage_total), 1, fp);
	if (sz != 1) {
		log_msg("fwrite error");
		tag_storage_total++;	/* 更新标签数失败,恢复标签数 */
		return -1;
	} else {
		log_msg("tag_storage_delete: tag_storage_total = %d", tag_storage_total);
		if (tag_storage_total == 0) {
			tag_storage_fflush();	/* 这不是很好的策略 */
		}
	}

	return 0;
}

int tag_storage_init(void)
{
	size_t sz;
	fp = fopen("tagdata.bin", "r+");
	if (fp) {
		sz = fread(&tag_storage_total, sizeof(tag_storage_total), 1, fp);
		if (sz != 1) {
			log_msg("fread error");
			return -1;
		} else {
			log_msg("tag_storage_init: tag_storage_total = %d", tag_storage_total);
		}
	} else {
		log_msg("tagdata.bin not exist, going to create it.");
		fp = fopen("tagdata.bin", "w");	/* 创建文件 */
		if (fp == NULL) {
			log_sys("fopen error w");
			return -1;
		}

		sz = fwrite(&tag_storage_total, sizeof(tag_storage_total), 1, fp);
		if (sz != 1) {
			log_ret("frwite error, <sz = %d>", sz);
			return -1;
		}

		fclose(fp);
		fp = fopen("tagdata.bin", "r+");
		if (fp == NULL) {
			log_sys("fopen error r+");
			return -1;
		}
	}

	return 0;
}
