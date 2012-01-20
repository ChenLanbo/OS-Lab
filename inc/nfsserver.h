#ifndef NFS_H
#define NFS_H
#include <inc/lib.h>
#include <lwip/sockets.h>
#include <lwip/inet.h>

#define SERVER_PORT 128
// Max connection requests
#define BACKUP 8

#define BUFFSIZE 512
#define debug 1

#define isdigit(ch) ((ch) >= '0' && (ch) <= '9')
#define isalpha(ch) (((ch) >= 'a' && (ch) <= 'z') || ((ch) >= 'A' && (ch) <= 'Z'))
#endif
