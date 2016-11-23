#include "utility.h"
#include "r2h_frame.h"

#include <stdlib.h>
#include <string.h>

inline int msec_between(struct timeval start, struct timeval end)
{
	int sec = end.tv_sec - start.tv_sec;
	int usec = end.tv_usec - start.tv_usec;
	return sec * 1000 + usec / 1000;
}

inline int convert_dec(uint8_t val)
{
	return (val / 16) * 10 + (val % 16);
}

inline uint8_t convert_hex(int val)
{
	return (val / 10) * 16 + (val % 10);
}

size_t replace_keyword(uint8_t *buf, size_t sz)
{
	int i;
	int len = sz;
	uint8_t	tmp_buf[1024];
	uint8_t	*ptr = tmp_buf;

	for (i = 0; i < sz; i++) {
		switch(buf[i]) {
		case FRAME_HEADER:
			*ptr++ = SUBSTITUTE_DATA_56;
			*ptr++ = SUBSTITUTE_DATA_56;
			len++;
			break;
		case SUBSTITUTE_DATA_56:
			*ptr++ = SUBSTITUTE_DATA_56;
			*ptr++ = SUBSTITUTE_DATA_57;
			len++;
			break;
		default:
			*ptr++ = buf[i];
		}
	}

	memcpy(buf, tmp_buf, len);/* �滻 buffer */
	return len;
}

int set_gprs_apn(const char *new_apn)
{
	char line[1024] = {0};

	FILE *rfp = fopen("/etc/ppp/gprs-connect-chat", "r");
	if (rfp == NULL) {
		log_msg("fopen error");
		return -1;
	}

	FILE *wfp = fopen("/f806/tmp", "w+");
	if (rfp == NULL) {
		log_msg("fopen error");
		return -1;
	}

	while (fgets(line, sizeof(line), rfp) != NULL) {
		char *ret = strstr(line, "AT+CGDCONT=1");
		if (ret) {
			log_msg("origin APN: %s", line);
			char rep_line[256] = {0};
			snprintf(rep_line, sizeof(rep_line), "OK AT+CGDCONT=1,\"IP\",\"%s\"\n", new_apn);
			fputs(rep_line, wfp);
		} else {
			fputs(line, wfp);
		}
	}

	fclose(rfp);
	fclose(wfp);
	rename("/f806/tmp", "/etc/ppp/gprs-connect-chat");
	return 0;
}


int set_chap_secrets(const char *username, const char *passwd){
	FILE *wfp = fopen("/f806/tmp", "w+");
	char rep_line[256] = {0};
	snprintf(rep_line, sizeof(rep_line), "%s       *      %s       *",username, passwd);
	fputs(rep_line, wfp);
	fclose(wfp);
	rename("/f806/tmp", "/etc/ppp/chap-secrets");
	log_msg("set_chap_secrets ok");
	return 0;
}


int set_gprs_wave(const char *username)
{
	char line[1024] = {0};

	FILE *rfp = fopen("/etc/ppp/peers/gprs-wave", "r");
	if (rfp == NULL) {
		log_msg("fopen error");
		return -1;
	}

	FILE *wfp = fopen("/f806/tmp", "w+");
	if (rfp == NULL) {
		log_msg("fopen error");
		return -1;
	}

	while (fgets(line, sizeof(line), rfp) != NULL) {
		char *ret = strstr(line, "user");
		if (ret) {
			char rep_line[256] = {0};
			snprintf(rep_line, sizeof(rep_line), "user %s\n", username);
			fputs(rep_line, wfp);
		} else {
			fputs(line, wfp);
		}
	}

	fclose(rfp);
	fclose(wfp);
	rename("/f806/tmp", "/etc/ppp/peers/gprs-wave");
	return 0;
}


/*---------------------------------------------------------------------
 * CRC: CRC-16
 *--------------------------------------------------------------------*/
static const uint16_t crc_16_table[256] = {
    0x0000, 0x8005, 0x800f, 0x000a, 0x801b, 0x001e, 0x0014, 0x8011,
    0x8033, 0x0036, 0x003c, 0x8039, 0x0028, 0x802d, 0x8027, 0x0022,
    0x8063, 0x0066, 0x006c, 0x8069, 0x0078, 0x807d, 0x8077, 0x0072,
    0x0050, 0x8055, 0x805f, 0x005a, 0x804b, 0x004e, 0x0044, 0x8041,
    0x80c3, 0x00c6, 0x00cc, 0x80c9, 0x00d8, 0x80dd, 0x80d7, 0x00d2,
    0x00f0, 0x80f5, 0x80ff, 0x00fa, 0x80eb, 0x00ee, 0x00e4, 0x80e1,
    0x00a0, 0x80a5, 0x80af, 0x00aa, 0x80bb, 0x00be, 0x00b4, 0x80b1,
    0x8093, 0x0096, 0x009c, 0x8099, 0x0088, 0x808d, 0x8087, 0x0082,
    0x8183, 0x0186, 0x018c, 0x8189, 0x0198, 0x819d, 0x8197, 0x0192,
    0x01b0, 0x81b5, 0x81bf, 0x01ba, 0x81ab, 0x01ae, 0x01a4, 0x81a1,
    0x01e0, 0x81e5, 0x81ef, 0x01ea, 0x81fb, 0x01fe, 0x01f4, 0x81f1,
    0x81d3, 0x01d6, 0x01dc, 0x81d9, 0x01c8, 0x81cd, 0x81c7, 0x01c2,
    0x0140, 0x8145, 0x814f, 0x014a, 0x815b, 0x015e, 0x0154, 0x8151,
    0x8173, 0x0176, 0x017c, 0x8179, 0x0168, 0x816d, 0x8167, 0x0162,
    0x8123, 0x0126, 0x012c, 0x8129, 0x0138, 0x813d, 0x8137, 0x0132,
    0x0110, 0x8115, 0x811f, 0x011a, 0x810b, 0x010e, 0x0104, 0x8101,
    0x8303, 0x0306, 0x030c, 0x8309, 0x0318, 0x831d, 0x8317, 0x0312,
    0x0330, 0x8335, 0x833f, 0x033a, 0x832b, 0x032e, 0x0324, 0x8321,
    0x0360, 0x8365, 0x836f, 0x036a, 0x837b, 0x037e, 0x0374, 0x8371,
    0x8353, 0x0356, 0x035c, 0x8359, 0x0348, 0x834d, 0x8347, 0x0342,
    0x03c0, 0x83c5, 0x83cf, 0x03ca, 0x83db, 0x03de, 0x03d4, 0x83d1,
    0x83f3, 0x03f6, 0x03fc, 0x83f9, 0x03e8, 0x83ed, 0x83e7, 0x03e2,
    0x83a3, 0x03a6, 0x03ac, 0x83a9, 0x03b8, 0x83bd, 0x83b7, 0x03b2,
    0x0390, 0x8395, 0x839f, 0x039a, 0x838b, 0x038e, 0x0384, 0x8381,
    0x0280, 0x8285, 0x828f, 0x028a, 0x829b, 0x029e, 0x0294, 0x8291,
    0x82b3, 0x02b6, 0x02bc, 0x82b9, 0x02a8, 0x82ad, 0x82a7, 0x02a2,
    0x82e3, 0x02e6, 0x02ec, 0x82e9, 0x02f8, 0x82fd, 0x82f7, 0x02f2,
    0x02d0, 0x82d5, 0x82df, 0x02da, 0x82cb, 0x02ce, 0x02c4, 0x82c1,
    0x8243, 0x0246, 0x024c, 0x8249, 0x0258, 0x825d, 0x8257, 0x0252,
    0x0270, 0x8275, 0x827f, 0x027a, 0x826b, 0x026e, 0x0264, 0x8261,
    0x0220, 0x8225, 0x822f, 0x022a, 0x823b, 0x023e, 0x0234, 0x8231,
    0x8213, 0x0216, 0x021c, 0x8219, 0x0208, 0x820d, 0x8207, 0x0202
};

inline uint16_t crc_16_byte(uint8_t byte, uint16_t last_crc)
{
	uint32_t tbl_idx;

        tbl_idx = ((last_crc >> 8) ^ byte) & 0xff;
        last_crc = (crc_16_table[tbl_idx] ^ (last_crc << 8)) & 0xffff;
	return last_crc & 0xffff;
}

inline uint16_t crc_16_buf(const uint8_t *data, uint16_t len)
{
	uint32_t tbl_idx;
	uint16_t crc = 0;

	while (len--) {
		tbl_idx = ((crc >> 8) ^ *data) & 0xff;
		crc = (crc_16_table[tbl_idx] ^ (crc << 8)) & 0xffff;

		data++;
	}
	return crc & 0xffff;
}

/*---------------------------------------------------------------------
 * CRC: CCITT
 *--------------------------------------------------------------------*/
static const uint16_t crc_ccitt_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

inline uint16_t crc_ccitt_byte(uint8_t byte, uint16_t last_crc)
{
	uint32_t tbl_idx;

	tbl_idx = ((last_crc >> 8) ^ byte) & 0xff;
	last_crc = (crc_ccitt_table[tbl_idx] ^ (last_crc << 8)) & 0xffff;
	return last_crc & 0xffff;
}

inline uint16_t crc_ccitt_buf(const uint8_t *data, uint16_t len)
{
	uint32_t tbl_idx;
	uint16_t crc = 0xffff;

	while (len--) {
		tbl_idx = ((crc >> 8) ^ *data) & 0xff;
		crc = (crc_ccitt_table[tbl_idx] ^ (crc << 8)) & 0xffff;

		data++;
	}
	return crc & 0xffff;
}

/*---------------------------------------------------------------------
 * �����ֽ���
 *--------------------------------------------------------------------*/
inline uint16_t swap16(uint16_t x)
{
	return ((x & 0x00FF) << 8) | 
		((x & 0xFF00) >> 8);
}

inline uint32_t swap32(uint32_t x)
{
	return ((x & 0x000000FF) << 24) |
    		((x & 0x0000FF00) << 8) |
    		((x & 0x00FF0000) >> 8) |
    		((x & 0xFF000000) >> 24);
}
