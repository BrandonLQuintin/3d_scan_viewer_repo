#include "uart.h"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

int uart_open(const char *device, int baud) {
    int file_descriptor = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (file_descriptor < 0) return -1;

    struct termios options;
    if (tcgetattr(file_descriptor, &options) != 0) {
        close(file_descriptor);
        return -1;
    }

    cfsetospeed(&options, baud);
    cfsetispeed(&options, baud);

    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag &= ~CRTSCTS;
    options.c_cflag |= CREAD | CLOCAL;

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOCTL | ISIG);
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    options.c_oflag &= ~(OPOST | ONLCR);

    options.c_cc[VMIN]  = 0;
    options.c_cc[VTIME] = 1;

    if (tcsetattr(file_descriptor, TCSANOW, &options) != 0) {
        close(file_descriptor);
        return -1;
    }

    tcflush(file_descriptor, TCIFLUSH);
    return file_descriptor;
}

ssize_t uart_read(int file_descriptor, uint8_t *buffer, size_t length) {
    return read(file_descriptor, buffer, length);
}

void uart_close(int file_descriptor) {
    if (file_descriptor >= 0) close(file_descriptor);
}
