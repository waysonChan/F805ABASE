#include "connect.h"

#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

int rs232_config(int fd, int speed, int databits, int stopbits, int parity)
{
	struct termios tio;
	if (tcgetattr(fd, &tio) != 0) {
		log_ret("tcgetattr error");
		return -1;
	}
	
	/* 波特率 */
	int baudrate;
	switch (speed) {	    
	case 4800:
		baudrate=B4800;
		break;
	case 9600:
		baudrate=B9600;
		break;
	case 19200:
		baudrate=B19200;
		break;
	case 38400:
		baudrate=B38400;
		break;
	case 57600:
		baudrate=B57600;
		break;
	case 115200:
		baudrate=B115200;
		break;
	default:
		log_msg("invalid baudrate");
		return -1;
	}
	
	if (-1 == cfsetispeed(&tio, baudrate)) {
		log_ret("cfsetispeed error");
		return -1;
	}
	
	if (-1 == cfsetospeed(&tio, baudrate)) {
		log_ret("cfsetospeed error");
		return -1;
	}
	
	/* 数据长度 */
	tio.c_cflag &= ~CSIZE;
	switch (databits) {
	case 7:
		tio.c_cflag |= CS7;
		break;
	case 8:
		tio.c_cflag |= CS8;
		break;
	default:
		log_msg("invalid databits as %d", databits);
		return -1;
	}
	
	/* 奇偶校验 */
	switch (parity) {
	case 'n':
	case 'N':
		tio.c_cflag &= ~PARENB;
		break;
	case 'o':
	case 'O':
		tio.c_cflag |= (PARODD | PARENB);
		break;
	case 'e':
	case 'E':
		tio.c_cflag |= PARENB;
		break;
	default:
		log_msg("unsupported parity");
		return -1;
	}
	
	/* 停止位 */
	switch (stopbits) {
	case 1:
		tio.c_cflag &= ~CSTOPB;
		break;
	case 2:
		tio.c_cflag |= CSTOPB;
		break;
	default:
		log_msg("unsupported stop bits");
		return -1;
	}

	tio.c_iflag = IGNPAR;
	tio.c_oflag = 0;
 	tio.c_lflag = 0;
#if 0
	tio.c_cc[VTIME] = 0;	/* NOTE: 如果设为10则会阻塞1秒 */
	tio.c_cc[VMIN] = 1;
#else
	tio.c_cc[VTIME] = 1;	/* 延时100ms */
	tio.c_cc[VMIN] = 0;	
#endif
	tcflush(fd, TCIFLUSH);
	if (tcsetattr(fd, TCSANOW, &tio) != 0) {
		log_ret("tcsetattr error");
		return -1;
	}

	return 0;
}

int rs232_flush(int fd)
{
	return tcflush(fd, TCIOFLUSH);
}

/*
 * >0	读到的字节数(EAGAIN)	
 * 0	EOF
 * -1	其他错误
 * -2	EAGAIN
 */
ssize_t rs232_read(int fd, uint8_t *buf, size_t nbytes)
{
	ssize_t ret = 0;
	uint32_t n_read = 0;

	while (n_read < nbytes) {
		ret = read(fd, buf+n_read, nbytes-n_read);
		if (ret <= 0)
			break;
		n_read += ret;
	}

	if (ret == 0) {	/* EOF */
		//return 0;
		return n_read;
	} else if (ret < 0) {
		if (errno == EAGAIN)
			return -2;
		return -1;
	}

	assert(n_read != 0);
	return n_read;
}

ssize_t rs232_write(int fd, uint8_t *buf, size_t nbytes)
{
	ssize_t ret;
	uint32_t n_written = 0;

	while (n_written < nbytes) {
		ret = write(fd, buf+n_written, nbytes-n_written);
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

int rs232_init(char *tty_name, int baudrate)
{
	int fd = open(tty_name, O_RDWR | O_NOCTTY);
	if (fd < 0) {
		log_ret("open error");
		return fd;
	}
#if 0	/* TODO */
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
		log_ret("fcntl error");
		return -1;
	}
#endif
	if (rs232_config(fd, baudrate, 8, 1, 'N') < 0) {
		close(fd);
		return fd;
	}

	return fd;
}
