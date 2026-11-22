/*
 * test_dev.c - user space test program for chr_df_mod
 *
 * Opens and closes /dev/myDev several times and prints a message
 * after every open / close, so we know the entry points were called.
 * The kernel side messages (with the statistics) can be seen with dmesg.
 *
 * usage: ./test_dev [device] [times]
 *        default device: /dev/myDev, default times: 3
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int main (int argc, char *argv[])
{
  const char *path = (argc > 1) ? argv[1] : "/dev/myDev";
  int times = (argc > 2) ? atoi (argv[2]) : 3;
  int i, fd;

  for (i = 1; i <= times; i++) {
    fd = open (path, O_RDONLY);
    if (fd < 0) {
      fprintf (stderr, "open %s failed: %s\n", path, strerror (errno));
      return 1;
    }
    printf ("[%d] open  %s succeeded (fd = %d)\n", i, path, fd);

    if (close (fd) < 0) {
      fprintf (stderr, "close %s failed: %s\n", path, strerror (errno));
      return 1;
    }
    printf ("[%d] close %s succeeded\n", i, path);
  }

  printf ("Done: %d opens and %d closes. Run 'dmesg | tail' to see the "
          "driver statistics.\n", times, times);
  return 0;
}
