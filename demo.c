#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "flexelf.h"

#define FLEX_BINARY "/usr/lib/debug/usr/bin/php5-cgi"
/* #define FLEX_BINARY "/proc/self/exe" */

static void _flex_elf_test(void)
{
  char *buildid = _flex_elf_get_buildid(FLEX_BINARY);
  printf("BuildID : 0x%s\n", buildid);

  int (*fp_in_shutdown)() = _flex_elf_get_sym(FLEX_BINARY, "fcgi_in_shutdown");
  void (*fp_terminate)() = _flex_elf_get_sym(FLEX_BINARY, "fcgi_terminate");

  printf("Sym: fp_in_shutdown/%p fp_terminate/%p\n", fp_in_shutdown, fp_terminate);
  printf("shutdown ? %d\n", fp_in_shutdown());
  fp_terminate();
  printf("shutdown ? %d\n", fp_in_shutdown());
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  printf("ACCEPT HOOKED!\n");

  _flex_elf_test();
  exit(1);
}
