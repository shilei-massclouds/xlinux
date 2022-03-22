#ifndef _LINUX_CONSOLE_H_
#define _LINUX_CONSOLE_H_

struct console {
    char    name[16];
    void    (*write)(struct console *, const char *, unsigned);
    /*
    int (*read)(struct console *, char *, unsigned);
    struct tty_driver *(*device)(struct console *, int *);
    void    (*unblank)(void);
    int (*setup)(struct console *, char *);
    int (*exit)(struct console *);
    int (*match)(struct console *, char *name, int idx, char *options);
    short   flags;
    */
    short   index;
    /*
    int cflag;
    void    *data;
    struct   console *next;
    */
};

#endif /* _LINUX_CONSOLE_H */
