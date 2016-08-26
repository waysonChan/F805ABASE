#include "r2h_connect.h"
#include "connect.h"

#include <sys/ioctl.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <strings.h>

#define RS485_NONBLOCK		0

/*
 * >0	读到的字节数(EAGAIN)	
 * 0	EOF
 * -1	除EAGAIN的错误
 */
static ssize_t r2h_rs485_recv(r2h_connect_t *C, uint8_t *buf, size_t nbytes)
{
	ssize_t ret = 0;
	uint32_t n_read = 0;

	while (n_read < nbytes) {
		ret = read(C->r2h[R2H_RS485].fd, buf+n_read, nbytes-n_read);
		if (ret <= 0)
			break;
		n_read += ret;
	}

	if (ret == 0) {	/* EOF */
#if RS485_NONBLOCK
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

ssize_t r2h_rs485_send(r2h_connect_t *C, uint8_t *buf, size_t nbytes)
{
	ssize_t ret;
	uint32_t n_written = 0;
	while (n_written < nbytes) {
		ret = write(C->r2h[R2H_RS485].fd, buf+n_written, nbytes-n_written);
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

#define TIOCGRS485	0x542E	
#define TIOCSRS485	0x542F

#define SER_RS485_ENABLED		(1 << 0)	/* If enabled */
#define SER_RS485_RTS_ON_SEND		(1 << 1)	/* Logical level for RTS pin when sending */
#define SER_RS485_RTS_AFTER_SEND	(1 << 2)	/* Logical level for RTS pin after sent*/
#define SER_RS485_RX_DURING_TX		(1 << 4)

struct serial_rs485 {
	uint32_t	flags;			/* RS485 feature flags */
	uint32_t	delay_rts_before_send;	/* Delay before send (milliseconds) */
	uint32_t	delay_rts_after_send;	/* Delay after send (milliseconds) */
	uint32_t	padding[5];		/* Memory is cheap, new structs are a royal PITA .. */
};

static int r2h_rs485_open(r2h_connect_t *C, int baud_rate)
{
	C->r2h[R2H_RS485].fd = rs232_init("/dev/ttyS2", baud_rate);
	if (C->r2h[R2H_RS485].fd < 0) {
		log_msg("r2h_rs485_open error");
		return -1;
	}

	int err = 0;
#if RS485_NONBLOCK
	err = fcntl(C->r2h[R2H_RS485].fd, F_SETFL, O_NONBLOCK);
	if (err < 0) {
		log_ret("r2h_rs485_open: fcntl error");
		return err;
	}
#endif
	struct serial_rs485 rs485conf;
	bzero(&rs485conf, sizeof(rs485conf));
	rs485conf.flags |= SER_RS485_ENABLED;
	rs485conf.delay_rts_before_send = 1;
	rs485conf.delay_rts_after_send = 1;

	err = ioctl(C->r2h[R2H_RS485].fd, TIOCSRS485, &rs485conf);
	if (err < 0) {
		log_ret("r2h_rs485_open: ioctl error");
		return err;
	}

	return err;
}

static int r2h_rs485_close_client(r2h_connect_t *C)
{
	rs232_flush(C->r2h[R2H_RS485].fd);
	return 0;
}

int r2h_rs485_init(r2h_connect_t *C, int baud_rate)
{
	C->r2h[R2H_RS485].open = r2h_rs485_open;
	C->r2h[R2H_RS485].close_client = r2h_rs485_close_client;
	C->r2h[R2H_RS485].recv = r2h_rs485_recv;
	C->r2h[R2H_RS485].send = r2h_rs485_send;

	return r2h_rs485_open(C, baud_rate);
}
