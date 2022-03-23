// SPDX-License-Identifier: GPL-2.0+

#include <bug.h>
#include <serial.h>

void
serial8250_console_write(struct uart_8250_port *up,
                         const char *s, unsigned int count)
{
    panic("%s: !", __func__);
}

int
serial8250_console_setup(struct uart_port *port, char *options,
                         bool probe)
{
    return 0;
}
