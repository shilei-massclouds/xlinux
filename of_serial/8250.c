// SPDX-License-Identifier: GPL-2.0+

#include <bug.h>
#include <tty.h>
#include <slab.h>
#include <errno.h>
#include <major.h>
#include <config.h>
#include <device.h>
#include <export.h>
#include <printk.h>
#include <serial.h>
#include <console.h>
#include <platform.h>

#define UART_NR CONFIG_SERIAL_8250_NR_UARTS

struct of_serial_info {
    int type;
    int line;
};

bool serial_ready;
EXPORT_SYMBOL(serial_ready);

static unsigned int nr_uarts = CONFIG_SERIAL_8250_RUNTIME_UARTS;

static struct uart_8250_port serial8250_ports[UART_NR];

static int
univ8250_console_match(struct console *co,
                       char *name, int idx, char *options)
{
    return -ENODEV;
}

static int
univ8250_console_setup(struct console *co, char *options)
{
    int retval;
    struct uart_port *port;

    /*
     * Check whether an invalid uart number has been specified, and
     * if so, search for the first available port that does have
     * console support.
     */
    if (co->index >= nr_uarts)
        co->index = 0;
    port = &serial8250_ports[co->index].port;
    /* link port to console */
    port->cons = co;

    retval = serial8250_console_setup(port, options, false);
    if (retval != 0)
        port->cons = NULL;
    return retval;
}

static struct uart_driver serial8250_reg;

static struct console univ8250_console = {
    .name  = "ttyS",
    .setup = univ8250_console_setup,
    .match = univ8250_console_match,
    .index = -1,
    .data  = &serial8250_reg,
};

#define SERIAL8250_CONSOLE  (&univ8250_console)

static struct uart_driver serial8250_reg = {
    .driver_name    = "serial",
    .dev_name       = "ttyS",
    .major          = TTY_MAJOR,
    .minor          = 64,
    .cons           = SERIAL8250_CONSOLE,
};

/*
 * This "device" covers _all_ ISA 8250-compatible serial devices listed
 * in the table in include/asm/serial.h
 */
static struct platform_device *serial8250_isa_devs;

static void
uart_configure_port(struct uart_driver *drv, struct uart_state *state,
                    struct uart_port *port)
{
    unsigned int flags;

    printk("%s: 1 mapbase(%lx)\n", __func__, port->mapbase);

    /*
     * If there isn't a port here, don't do anything further.
     */
    if (!port->iobase && !port->mapbase && !port->membase)
        return;

    /*
     * Now do the auto configuration stuff.  Note that config_port
     * is expected to claim the resources and map the port for us.
     */
    flags = 0;

    printk("%s: 1\n", __func__);
    if (port->flags & UPF_BOOT_AUTOCONF) {
        if (!(port->flags & UPF_FIXED_TYPE)) {
            port->type = PORT_UNKNOWN;
            flags |= UART_CONFIG_TYPE;
        }
        printk("%s: 2\n", __func__);
        port->ops->config_port(port, flags);
        printk("%s: 3\n", __func__);
    }

    if (port->type != PORT_UNKNOWN) {

        /*
         * If this driver supports console, and it hasn't been
         * successfully registered yet, try to re-register it.
         * It may be that the port was not available.
         */
        if (port->cons && !(port->cons->flags & CON_ENABLED)) {
            register_console(port->cons);
        }
    }
}

int
uart_add_one_port(struct uart_driver *drv, struct uart_port *uport)
{
    struct tty_port *port;
    struct uart_state *state;

    state = drv->state + uport->line;
    port = &state->port;

    uport->cons = drv->cons;
    printk("%s: 2.0 drv->cons(%lx)\n", __func__, drv->cons);
    printk("%s: mapbase(%lx)\n", __func__, uport->mapbase);

    uart_configure_port(drv, state, uport);

    printk("%s: 2.1 uport(%lx)\n", __func__, uport->cons);
    port->console = uart_console(uport);
    printk("%s: 3\n", __func__);

    /*
     * Ensure UPF_DEAD is not set.
     */
    uport->flags &= ~UPF_DEAD;
    return 0;
}

static void
serial8250_register_ports(struct uart_driver *drv, struct device *dev)
{
    int i;

    for (i = 0; i < nr_uarts; i++) {
        struct uart_8250_port *up = &serial8250_ports[i];
        printk("%s: mapbase(%lx)\n", __func__, up->port.mapbase);

        if (up->port.type == PORT_8250_CIR)
            continue;

        if (up->port.dev)
            continue;

        up->port.dev = dev;

        uart_add_one_port(drv, &up->port);
    }
}

static struct uart_8250_port *
serial8250_find_match_or_unused(struct uart_port *port)
{
    int i;

    /* try line number first if still available */
    i = port->line;
    if (i < nr_uarts && serial8250_ports[i].port.type == PORT_UNKNOWN &&
            serial8250_ports[i].port.iobase == 0)
        return &serial8250_ports[i];

    panic("%s: !", __func__);
}

int serial8250_register_8250_port(struct uart_8250_port *up)
{
    struct uart_8250_port *uart;
    int ret = -ENOSPC;

    uart = serial8250_find_match_or_unused(&up->port);
    if (uart && uart->port.type != PORT_8250_CIR) {

        uart->port.iobase       = up->port.iobase;
        uart->port.membase      = up->port.membase;
        uart->port.irq          = up->port.irq;
        uart->port.iotype       = up->port.iotype;
        uart->port.flags        = up->port.flags | UPF_BOOT_AUTOCONF;
        uart->port.mapbase      = up->port.mapbase;
        uart->port.mapsize      = up->port.mapsize;

        if (up->port.flags & UPF_FIXED_TYPE)
            uart->port.type = up->port.type;

        if (uart->port.type != PORT_8250_CIR) {
            ret = uart_add_one_port(&serial8250_reg, &uart->port);
            if (ret)
                panic("add one port error!");

            ret = uart->port.line;
        } else {
            printk("skipping CIR port at 0x%lx / 0x%lx, IRQ %d\n",
                   uart->port.iobase,
                   (unsigned long long)uart->port.mapbase,
                   uart->port.irq);

            ret = 0;
        }
        panic("%s: 1", __func__);
    }
    panic("%s: !", __func__);
}

static const struct of_device_id of_platform_serial_table[] = {
    { .compatible = "ns8250",   .data = (void *)PORT_8250, },
    { .compatible = "ns16450",  .data = (void *)PORT_16450, },
    { .compatible = "ns16550a", .data = (void *)PORT_16550A, },
    { .compatible = "ns16550",  .data = (void *)PORT_16550, },
    { .compatible = "ns16750",  .data = (void *)PORT_16750, },
    { .compatible = "ns16850",  .data = (void *)PORT_16850, },
    { /* end of list */ },
};

/*
 * Fill a struct uart_port for a given device node
 */
static int
of_platform_serial_setup(struct platform_device *ofdev,
                         int type, struct uart_8250_port *up,
                         struct of_serial_info *info)
{
    int ret;
    int irq;
    struct resource resource;
    struct uart_port *port = &up->port;
    struct device_node *np = ofdev->dev.of_node;

    ret = of_address_to_resource(np, 0, &resource);
    if (ret)
        panic("invalid address\n");

    port->flags = UPF_SHARE_IRQ | UPF_BOOT_AUTOCONF | UPF_FIXED_PORT |
                  UPF_FIXED_TYPE;

    if (resource_type(&resource) == IORESOURCE_IO) {
        port->iotype = UPIO_PORT;
        port->iobase = resource.start;
    } else {
        port->mapbase = resource.start;
        port->mapsize = resource_size(&resource);

        port->iotype = UPIO_MEM;
        port->flags |= UPF_IOREMAP;
    }

    port->type = type;
    return 0;
}

/*
 * Try to register a serial port
 */
static int
of_platform_serial_probe(struct platform_device *ofdev)
{
    int ret;
    unsigned int port_type;
    struct of_serial_info *info;
    struct uart_8250_port port8250;

    port_type = (unsigned long)of_device_get_match_data(&ofdev->dev);
    if (port_type == PORT_UNKNOWN)
        return -EINVAL;

    info = kzalloc(sizeof(*info), GFP_KERNEL);
    if (info == NULL)
        return -ENOMEM;

    memset(&port8250, 0, sizeof(port8250));
    ret = of_platform_serial_setup(ofdev, port_type, &port8250, info);
    if (ret)
        panic("serial setup error!");

    printk("%s: 2\n", __func__);
    ret = serial8250_register_8250_port(&port8250);
    if (ret < 0)
        panic("bad port!");

    printk("%s: 3\n", __func__);
    info->type = port_type;
    info->line = ret;
    platform_set_drvdata(ofdev, info);
    panic("%s: !", __func__);
    return 0;
}

static struct platform_driver of_platform_serial_driver = {
    .driver = {
        .name = "of_serial",
        .of_match_table = of_platform_serial_table,
    },
    .probe = of_platform_serial_probe,
};

int
uart_set_options(struct uart_port *port, struct console *co,
                 int baud, int parity, int bits, int flow)
{
    return 0;
}

static void serial8250_isa_init_ports(void)
{
    int i;
    static int first = 1;

    if (!first)
        return;
    first = 0;

    if (nr_uarts > UART_NR)
        nr_uarts = UART_NR;

    printk("%s: 1\n", __func__);
    for (i = 0; i < nr_uarts; i++) {
        struct uart_8250_port *up = &serial8250_ports[i];
        struct uart_port *port = &up->port;

        port->line = i;
        serial8250_init_port(up);
    }
}

static void serial8250_init(void)
{
    serial8250_isa_init_ports();
}

static int
init_module(void)
{
    printk("module[of_serial]: init begin ...\n");

    serial8250_init();

    platform_driver_register(&of_platform_serial_driver);
    serial_ready = true;

    printk("module[of_serial]: init end!\n");
    return 0;
}
