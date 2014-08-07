#ifndef _COMMAND_H
#define _COMMAND_H

#include "config.h"
#include "r2h_connect.h"

int command_answer(r2h_connect_t *C, uint8_t cmd_id, uint8_t result,
	const void *buf, size_t sz);

#endif	/* _COMMAND_H */
