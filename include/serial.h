#ifndef _LINUX_SERIAL_8250_H
#define _LINUX_SERIAL_8250_H

/*
 * These are the supported serial types.
 */
#define PORT_UNKNOWN    0
#define PORT_8250_CIR   23  /* CIR infrared port, has its own driver */

/*
 * This is the state information which is persistent across opens.
 */
struct uart_state {
};

struct uart_driver {
    /*
     * these are private; the low level driver should not
     * touch these; they should be initialised to NULL
     */
    struct uart_state   *state;
};

struct uart_port {
    unsigned int    type;       /* port type */
    struct device   *dev;       /* parent device */
    unsigned int    line;       /* port index */
    struct console  *cons;      /* struct console, if any */
};

struct uart_8250_port {
    struct uart_port    port;
};

/*
 * Allocate 8250 platform device IDs.  Nothing is implied by
 * the numbering here, except for the legacy entry being -1.
 */
enum {
    PLAT8250_DEV_LEGACY = -1,
    PLAT8250_DEV_PLATFORM,
    PLAT8250_DEV_PLATFORM1,
    PLAT8250_DEV_PLATFORM2,
    PLAT8250_DEV_FOURPORT,
    PLAT8250_DEV_ACCENT,
    PLAT8250_DEV_BOCA,
    PLAT8250_DEV_EXAR_ST16C554,
    PLAT8250_DEV_HUB6,
    PLAT8250_DEV_AU1X00,
    PLAT8250_DEV_SM501,
};

#endif /* _LINUX_SERIAL_8250_H */
