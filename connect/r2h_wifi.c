#include "r2h_connect.h"
#include "connect.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#define WIFI_NONBLOCK		0

/*
 * >0	读到的字节数(EAGAIN)	
 * 0	EOF
 * -1	除EAGAIN的错误
 */
static ssize_t _r2h_wifi_recv(r2h_connect_t *C, uint8_t *buf, size_t nbytes)
{
	ssize_t ret = 0;
	uint32_t n_read = 0;

	while (n_read < nbytes) {
		ret = read(C->r2h[R2H_WIFI].fd, buf+n_read, nbytes-n_read);
		if (ret <= 0)
			break;
		n_read += ret;
	}

	if (ret == 0) {	/* EOF */
#if WIFI_NONBLOCK
		return 0;
#else
		return n_read;
#endif
	} else if (ret < 0) {
		if (errno == EAGAIN)
			return n_read;
		return -1;
	}

	assert(n_read != 0);
	return n_read;
}

ssize_t r2h_wifi_recv(r2h_connect_t *C, uint8_t *buf, size_t nbytes)
{
	int ret = _r2h_wifi_recv(C, buf, nbytes);
	
	if (C->wifi_transparent_mode && ret > 0) {
		r2h_rs232_send(C, buf, ret);
		ret = 0;
	}

	return ret;
}

ssize_t r2h_wifi_send(r2h_connect_t *C, uint8_t *buf, size_t nbytes)
{
	ssize_t ret;
	uint32_t n_written = 0;

	while (n_written < nbytes) {
		ret = write(C->r2h[R2H_WIFI].fd, buf+n_written, nbytes-n_written);
		if (ret < 0) {
			if (errno == EAGAIN)
				return n_written;
			return -1;
		}
		assert(ret != 0);
		
		n_written += ret;
	}

	return n_written;
}

static int r2h_wifi_open(r2h_connect_t *C, int baud_rate)
{
	C->r2h[R2H_WIFI].fd = rs232_init("/dev/ttyS4", baud_rate);
	if (C->r2h[R2H_WIFI].fd < 0) {
		log_msg("r2h_wifi_open error");
		return -1;
	}

	int err = 0;
#if WIFI_NONBLOCK
	err = fcntl(C->r2h[R2H_WIFI].fd, F_SETFL, O_NONBLOCK);
	if (err < 0) {
		log_ret("r2h_wifi_open: fcntl error");
		return err;
	}
#endif
	return err;
}

static int r2h_wifi_close_client(r2h_connect_t *C)
{
	return 0;
}

int r2h_wifi_init(r2h_connect_t *C, int baud_rate)
{
	C->r2h[R2H_WIFI].open = r2h_wifi_open;
	C->r2h[R2H_WIFI].close_client = r2h_wifi_close_client;
	C->r2h[R2H_WIFI].recv = r2h_wifi_recv;
	C->r2h[R2H_WIFI].send = r2h_wifi_send;

	return r2h_wifi_open(C, baud_rate);
}
