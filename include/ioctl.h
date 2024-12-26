#ifndef IOCTL_H
#define IOCTL_H

#include <io.h>

int ioctl (fd_t fd, unsigned long request, void * argp);

#define TIOCSCTTY 0x540E
#define TIOCNOTTY 0x5422

#endif
