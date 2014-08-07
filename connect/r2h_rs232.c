#include "r2h_connect.h"
#include "connect.h"

#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#define RS232_NONBLOCK		0

/*
 * >0	读到的字节数(EAGAIN)	
 * 0	EOF
 * -1	除EAGAIN的错误
 */
static ssize_t _r2h_rs232_recv(r2h_connect_t *C, uint8_t *buf, size_t nbytes)
{
	ssize_t ret = 0;
	uint32_t n_read = 0;

	while (n_read < nbytes) {
		ret = read(C->r2h[R2H_RS232].fd, buf+n_read, nbytes-n_read);
		if (ret <= 0)
			break;
		n_read += ret;
	}

	if (ret == 0) {	/* EOF */
#if RS232_NONBLOCK
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

/*
 * 问题: 上位机发过来的 0x03 可能和后面的 0x55 指令连在一起，这将导致退出 WIFI 透传模式失败
 * 解决: 判断第一个字节是否是 0x03; 如果是，先执行退出透传，再将 buf 中的第1个字节去掉后解析
 *       数据
 */
#define WIFI_TRANS_EXIT_MARK	0x03
static ssize_t r2h_rs232_recv(r2h_connect_t *C, uint8_t *buf, size_t nbytes)
{
	int ret = _r2h_rs232_recv(C, buf, nbytes);

	if (C->wifi_transparent_mode && ret > 0) {
		if (buf[0] == WIFI_TRANS_EXIT_MARK) {
			log_msg("recieve EXIT MARK");
			C->wifi_transparent_mode = false;

			if (ret != 1) {
				int i;
				for (i = 0; i < ret-1; i++) {
					buf[i] = buf[i+1];
				}
				return ret - 1;
			}
#if 0
			uint8_t cmd[] = "AT+ENTM\r";
			r2h_wifi_send(C, cmd, sizeof(cmd)-1);
#endif
		} else {
			r2h_wifi_send(C, buf, ret);
		}

		ret = 0;
	}

	return ret;
}

ssize_t r2h_rs232_send(r2h_connect_t *C, uint8_t *buf, size_t nbytes)
{
	ssize_t ret;
	uint32_t n_written = 0;

	while (n_written < nbytes) {
		ret = write(C->r2h[R2H_RS232].fd, buf+n_written, nbytes-n_written);
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

static int r2h_rs232_open(r2h_connect_t *C, int baud_rate)
{
	C->r2h[R2H_RS232].fd = rs232_init("/dev/ttyS3", baud_rate);
	if (C->r2h[R2H_RS232].fd < 0) {
		log_msg("r2h_rs232_open error");
		return -1;
	}

	int err = 0;
#if RS232_NONBLOCK
	err = fcntl(C->r2h[R2H_RS232].fd, F_SETFL, O_NONBLOCK);
	if (err < 0) {
		log_ret("r2h_rs232_open: fcntl error");
		return err;
	}
#endif
	return err;
}

static int r2h_rs232_close_client(r2h_connect_t *C)
{
	rs232_flush(C->r2h[R2H_RS232].fd);
	return 0;
}

int r2h_rs232_init(r2h_connect_t *C, int baud_rate, int upload_mode)
{
	C->r2h[R2H_RS232].open = r2h_rs232_open;
	C->r2h[R2H_RS232].close_client = r2h_rs232_close_client;
	C->r2h[R2H_RS232].send = r2h_rs232_send;

	if (upload_mode == UPLOAD_MODE_WIFI) {
		C->r2h[R2H_RS232].recv = r2h_rs232_recv;
	} else {
		C->r2h[R2H_RS232].recv = _r2h_rs232_recv;
	}

	return r2h_rs232_open(C, baud_rate);
}
