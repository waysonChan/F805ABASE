#ifndef _UTILITY_H
#define _UTILITY_H

#include <sys/time.h>

#include "config.h"

inline int msec_between(struct timeval start, struct timeval end);
inline int convert_dec(uint8_t val);
inline uint8_t convert_hex(int val);
int set_gprs_apn(const char *new_apn);
int set_apn_user(const char *username);
int set_apn_passwd(const char *passwd);

int set_gprs_wave(const char *username);

size_t replace_keyword(uint8_t *buf, size_t sz);
inline uint16_t crc_16_byte(uint8_t byte, uint16_t last_crc);
inline uint16_t crc_16_buf(const uint8_t *data, uint16_t len);
inline uint16_t crc_ccitt_byte(uint8_t byte, uint16_t last_crc);
inline uint16_t crc_ccitt_buf(const uint8_t *data, uint16_t len);

inline uint16_t swap16(uint16_t x);
inline uint32_t swap32(uint32_t x);

#define MIN(a,b) (((a)>(b))?(b):(a))
#define MAX(a,b) (((a)>(b))?(a):(b))

#endif
