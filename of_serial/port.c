// SPDX-License-Identifier: GPL-2.0+

#include <bug.h>
#include <errno.h>
#include <ioport.h>
#include <serial.h>
#include <ioremap.h>

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
    int ret;
    int baud = 9600;
    int bits = 8;
    int parity = 'n';
    int flow = 'n';

    if (!port->iobase && !port->membase)
        return -ENODEV;

    if (options)
        panic("NOT options parse func!");
    else if (probe)
        panic("NOT support for probe!");

    ret = uart_set_options(port, port->cons, baud, parity, bits, flow);
    if (ret)
        return ret;

    return 0;
}

static unsigned int serial8250_port_size(struct uart_8250_port *pt)
{
    if (pt->port.mapsize)
        return pt->port.mapsize;

    panic("%s: !", __func__);
}

static int serial8250_request_std_resource(struct uart_8250_port *up)
{
    int ret = 0;
    struct uart_port *port = &up->port;
    unsigned int size = serial8250_port_size(up);

    switch (port->iotype) {
    case UPIO_AU:
    case UPIO_TSI:
    case UPIO_MEM32:
    case UPIO_MEM16:
    case UPIO_MEM:
        if (!port->mapbase)
            break;

        if (!request_mem_region(port->mapbase, size, "serial")) {
            ret = -EBUSY;
            break;
        }

        if (port->flags & UPF_IOREMAP) {
            port->membase = ioremap(port->mapbase, size);
            if (!port->membase) {
                panic("ioremap error!");
                ret = -ENOMEM;
            }
        }

        panic("%s: 1", __func__);
        break;
    default:
        panic("%s: iotype(%d)!", __func__, port->iotype);
    }

    return ret;

}

static void serial8250_config_port(struct uart_port *port, int flags)
{
    int ret;
    struct uart_8250_port *up = up_to_u8250p(port);

    printk("%s: 1\n", __func__);

    /*
     * Find the region that we can probe for.  This in turn
     * tells us whether we can probe for the type of port.
     */
    ret = serial8250_request_std_resource(up);
    if (ret < 0)
        panic("bad resource!");

    printk("%s: 2\n", __func__);
}

static const struct uart_ops serial8250_pops = {
    .config_port    = serial8250_config_port,
};

void serial8250_init_port(struct uart_8250_port *up)
{
    struct uart_port *port = &up->port;

    printk("%s: 1\n", __func__);
    port->ops = &serial8250_pops;
}
