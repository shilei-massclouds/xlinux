#ifndef _LINUX_SERIAL_8250_H
#define _LINUX_SERIAL_8250_H

struct uart_8250_port {
};

void
serial8250_console_write(struct uart_8250_port *up,
                         const char *s, unsigned int count);

#endif /* _LINUX_SERIAL_8250_H */
