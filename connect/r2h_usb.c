#include "r2h_connect.h"
#include "connect.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#define USB_NONBLOCK		0

/*
 * >0	读到的字节数(EAGAIN)	
 * 0	EOF
 * -1	除EAGAIN的错误
 */
static ssize_t r2h_usb_recv(r2h_connect_t *C, uint8_t *buf, size_t nbytes)
{
	ssize_t ret = 0;
	uint32_t n_read = 0;

	while (n_read < nbytes) {
		ret = read(C->r2h[R2H_USB].fd, buf+n_read, nbytes-n_read);
		if (ret <= 0)
			break;
		n_read += ret;
	}

	if (ret == 0) {	/* EOF */
#if USB_NONBLOCK
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

ssize_t r2h_usb_send(r2h_connect_t *C, uint8_t *buf, size_t nbytes)
{
	ssize_t ret;
	uint32_t n_written = 0;

	while (n_written < nbytes) {
		ret = write(C->r2h[R2H_USB].fd, buf+n_written, nbytes-n_written);
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

static int r2h_usb_open(r2h_connect_t *C, int baud_rate)
{
	C->r2h[R2H_USB].fd = rs232_init("/dev/ttyGS0", baud_rate);
	if (C->r2h[R2H_USB].fd < 0) {
		log_msg("r2h_usb_open error");
		return -1;
	}

	int err = 0;
#if USB_NONBLOCK
	err = fcntl(C->r2h[R2H_USB].fd, F_SETFL, O_NONBLOCK);
	if (err < 0) {
		log_ret("r2h_usb_open: fcntl error");
		return err;
	}
#endif
	return err;
}

static int r2h_usb_close_client(r2h_connect_t *C)
{
	rs232_flush(C->r2h[R2H_USB].fd);
	close(C->r2h[R2H_USB].fd);

	return r2h_usb_open(C, R2H_USB_BAUD_RATE);
}

int r2h_usb_init(r2h_connect_t *C, int baud_rate)
{
	C->r2h[R2H_USB].open = r2h_usb_open;
	C->r2h[R2H_USB].close_client = r2h_usb_close_client;
	C->r2h[R2H_USB].recv = r2h_usb_recv;
	C->r2h[R2H_USB].send = r2h_usb_send;

	return r2h_usb_open(C, baud_rate);
}
