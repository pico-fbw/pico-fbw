/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "platform/defs.h"
#include "platform/stdio.h"

#include "platform/uart.h"

typedef struct UARTInstance {
    const char *device;
    int fd;
} UARTInstance;

static UARTInstance instances[MAX_UART_DEVICES];

/**
 * @return a pointer to the UART instance that uses the given pins, or NULL if no such instance exists
 */
static UARTInstance *uart_instance_from_pins(i16 tx, i16 rx) {
    for (u32 i = 0; i < MAX_UART_DEVICES; i++) {
        UARTMapping mapping = UART_MAP[i];
        if (mapping.tx == tx && mapping.rx == rx) {
            instances[i].device = mapping.device;
            return &instances[i];
        }
    }
    return NULL;
}

/**
 * Convert a baudrate to a speed_t value.
 * @param baud baud rate to convert
 * @return speed_t value, or B0 if the baudrate is not supported
 */
static speed_t baud_to_speed(u32 baud) {
    switch (baud) {
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
        case 57600:
            return B57600;
        case 115200:
            return B115200;
        default:
            return B0;
    }
}

bool uart_setup(i16 tx, i16 rx, u32 baud) {
    UARTInstance *inst = uart_instance_from_pins(tx, rx);
    if (inst == NULL) {
        return false;
    }
    inst->fd = open(inst->device, O_RDWR | O_NOCTTY | O_SYNC);
    if (inst->fd < 0) {
        return false;
    }
    struct termios tty;
    if (tcgetattr(inst->fd, &tty) != 0) {
        close(inst->fd);
        inst->fd = -1;
        return false;
    }
    speed_t speed = baud_to_speed(baud);
    if (speed == B0) {
        close(inst->fd);
        inst->fd = -1;
        return false;
    }
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
    // Configure processing (8N1, no flow control, raw input/output, non-blocking)
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10;
    if (tcsetattr(inst->fd, TCSANOW, &tty) != 0) {
        close(inst->fd);
        inst->fd = -1;
        return false;
    }
    return true;
}

char *uart_read(i16 tx, i16 rx) {
    UARTInstance *inst = uart_instance_from_pins(tx, rx);
    if (inst == NULL || inst->fd < 0) {
        return NULL;
    }
    struct pollfd pfd;
    pfd.fd = inst->fd;
    pfd.events = POLLIN;
    // Perform a non-blocking poll to check if there is data available
    if (poll(&pfd, 1, 0) <= 0) {
        return NULL;
    }
    // We have data available, read it
    char *buf = NULL;
    while (true) {
        char c;
        ssize_t n = read(inst->fd, &c, 1);
        if (n < 0) {
            if (errno == EAGAIN) {
                break;
            }
            // Error reading
            free(buf);
            return NULL;
        }
        if (n == 0) {
            // End of data
            break;
        }
        // Resize the buffer and store the character
        buf = try_realloc(buf, (strlen(buf) + 2) * sizeof(char));
        if (!buf) {
            return NULL;
        }
        buf[strlen(buf)] = c;
        buf[strlen(buf) + 1] = '\0';
    }
    return buf;
}

bool uart_write(i16 tx, i16 rx, const char *str) {
    UARTInstance *inst = uart_instance_from_pins(tx, rx);
    if (inst == NULL || inst->fd < 0) {
        return false;
    }
    return write(inst->fd, str, strlen(str)) == (ssize_t)strlen(str);
}
