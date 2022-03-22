// SPDX-License-Identifier: GPL-2.0+

#include <config.h>
#include <export.h>
#include <printk.h>
#include <serial.h>
#include <console.h>

#define UART_NR CONFIG_SERIAL_8250_NR_UARTS

static struct uart_8250_port serial8250_ports[UART_NR];

static void
univ8250_console_write(struct console *co,
                       const char *s,
                       unsigned int count)
{
    struct uart_8250_port *up = &serial8250_ports[co->index];

    serial8250_console_write(up, s, count);
}

static struct console univ8250_console = {
    .name       = "ttyS",
    .write      = univ8250_console_write,
    /*
    .device     = uart_console_device,
    .setup      = univ8250_console_setup,
    .exit       = univ8250_console_exit,
    .match      = univ8250_console_match,
    .flags      = CON_PRINTBUFFER | CON_ANYTIME,
    */
    .index      = -1,
    /*
    .data       = &serial8250_reg,
    */
};

static int univ8250_console_init(void)
{
    register_console(&univ8250_console);
    return 0;
}

static int
init_module(void)
{
    printk("module[serial]: init begin ...\n");

    univ8250_console_init();

    printk("module[serial]: init end!\n");
    return 0;
}
