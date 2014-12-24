#include "r2h_connect.h"
#include "connect.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>     

#define WG_26_CMD	_IO('p', 0x01)
#define WG_34_CMD	_IO('p', 0x02)
#define PULSE_WIDTH_CMD	_IO('p', 0x03)

ssize_t wiegand_send(r2h_connect_t *C, uint8_t *buf, size_t nbytes)
{
	int cmd;
	if (nbytes == 3) {
		cmd = WG_26_CMD;
	} else if (nbytes == 4) {
		cmd = WG_34_CMD;
	} else {
		log_msg("invalid nbytes");
		return -1;
	}

	return ioctl(C->wg_fd, cmd, buf);
}

int wiegand_init(r2h_connect_t *C, int pulse_width)
{
	C->wg_fd = open("/dev/wgout", O_RDONLY | O_NONBLOCK);
	if (C->wg_fd < 0) {
		log_ret("wiegand_init error");
		return -1;
	}
#if WIEGAND_CFG_ENABLE
	if (ioctl(C->wg_fd, PULSE_WIDTH_CMD, pulse_width * 100) < 0) {
		log_ret("wiegand_init: ioctl error");
		return -1;
	}
#endif
	return 0;
}
