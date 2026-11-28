/*
 * test_rw.c - user space read / write test for chr_df_mod
 *
 *   1. fill buffer wbuf with X bytes
 *   2. write X bytes from wbuf to the device
 *   3. read  X bytes from the device into a different buffer rbuf
 *   4. compare wbuf and rbuf
 *
 * X must not cross the 4K buffer of the driver (1 <= X <= 4096).
 *

 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define BUF_SIZE 4096

int main (int argc, char *argv[])
{
  int x          = (argc > 1) ? atoi (argv[1]) : 100;
  const char *dv = (argc > 2) ? argv[2] : "/dev/myDev";
  char wbuf[BUF_SIZE], rbuf[BUF_SIZE];   /* two different buffers */
  int fd, i;
  ssize_t n;

  if (x <= 0 || x > BUF_SIZE) {
    fprintf (stderr, "X must be between 1 and %d\n", BUF_SIZE);
    return 1;
  }

  /*  prepare the data */
  for (i = 0; i < x; i++)
    wbuf[i] = 'A' + i % 26;
  memset (rbuf, 0, sizeof (rbuf));

  fd = open (dv, O_RDWR);
  if (fd < 0) {
    fprintf (stderr, "open %s failed: %s\n", dv, strerror (errno));
    return 1;
  }
  printf ("open %s succeeded\n", dv);

  /*  write X bytes */
  n = write (fd, wbuf, x);
  if (n < 0) {
    fprintf (stderr, "write failed: %s\n", strerror (errno));
    close (fd);
    return 1;
  }
  printf ("wrote %zd bytes\n", n);

  /*  read X bytes into the second buffer */
  n = read (fd, rbuf, x);
  if (n < 0) {
    fprintf (stderr, "read failed: %s\n", strerror (errno));
    close (fd);
    return 1;
  }
  printf ("read  %zd bytes\n", n);

  close (fd);

  /* compare the two buffers */
  if (n == x && memcmp (wbuf, rbuf, x) == 0) {
    printf ("OK - buffers are identical\n");
    return 0;
  }

  printf ("FAILED - buffers differ\n");
  return 1;
}
