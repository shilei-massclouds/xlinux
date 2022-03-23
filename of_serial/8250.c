// SPDX-License-Identifier: GPL-2.0+

#include <bug.h>
#include <errno.h>
#include <config.h>
#include <device.h>
#include <export.h>
#include <printk.h>
#include <serial.h>
#include <console.h>
#include <platform.h>

#define UART_NR CONFIG_SERIAL_8250_NR_UARTS

static unsigned int nr_uarts = CONFIG_SERIAL_8250_RUNTIME_UARTS;

static struct uart_8250_port serial8250_ports[UART_NR];

static struct uart_driver serial8250_reg;

/*
 * This "device" covers _all_ ISA 8250-compatible serial devices listed
 * in the table in include/asm/serial.h
 */
static struct platform_device *serial8250_isa_devs;

static void
uart_configure_port(struct uart_driver *drv, struct uart_state *state,
                    struct uart_port *port)
{
    if (port->type != PORT_UNKNOWN) {

        /*
         * If this driver supports console, and it hasn't been
         * successfully registered yet, try to re-register it.
         * It may be that the port was not available.
         */
        printk("### %s: 1 type(%d)\n", __func__, port->type);
        if (port->cons && !(port->cons->flags & CON_ENABLED)) {
            printk("### %s: 2\n", __func__);
            register_console(port->cons);
        }
    }
    panic("%s: port->type(%d)!", __func__, port->type);
}

int
uart_add_one_port(struct uart_driver *drv, struct uart_port *uport)
{
    struct uart_state *state;

    state = drv->state + uport->line;

    uart_configure_port(drv, state, uport);
}

static void
serial8250_register_ports(struct uart_driver *drv, struct device *dev)
{
    int i;

    for (i = 0; i < nr_uarts; i++) {
        struct uart_8250_port *up = &serial8250_ports[i];

        if (up->port.type == PORT_8250_CIR)
            continue;

        if (up->port.dev)
            continue;

        up->port.dev = dev;

        uart_add_one_port(drv, &up->port);
    }
}

static void serial8250_init(void)
{
    int ret;

    serial8250_isa_devs = platform_device_alloc("serial8250",
                                                PLAT8250_DEV_LEGACY);
    if (!serial8250_isa_devs)
        panic("out of memory!");

    /*
    ret = platform_device_add(serial8250_isa_devs);
    if (ret)
        panic("add device error!");
        */

    serial8250_register_ports(&serial8250_reg, &serial8250_isa_devs->dev);
    panic("%s: !", __func__);
}

static int
init_module(void)
{
    printk("module[serial]: init begin ...\n");

    serial8250_init();

    printk("module[serial]: init end!\n");
    return 0;
}
