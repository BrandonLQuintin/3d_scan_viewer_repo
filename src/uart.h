#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

int uart_open(const char *device, int baud);
ssize_t uart_read(int file_descriptor, uint8_t *buffer, size_t length);
ssize_t uart_write(int file_descriptor, const uint8_t *data, size_t length);
void uart_close(int file_descriptor);

#endif
