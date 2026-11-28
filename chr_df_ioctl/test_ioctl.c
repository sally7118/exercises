/*
 * test_ioctl.c - user space test of the ioctl commands of chr_df_mod
 *
 *   1. open the device
 *   2. do several writes and reads (so there are statistics)
 *   3. CHR_STATSGET - get the statistics and print them
 *   4. CHR_STATSCLR - clear the statistics
 *   5. CHR_STATSGET - get them again and check they are 0
 *
 
 */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>        /* ioctl() */
#include "chr_df_ioctl.h"     /* CHR_STATSGET, CHR_STATSCLR, struct chr_stats */

#define CHUNK   128   /* 4096 / 128 is whole -> writes never cross the 4K end */
#define WRITES  3
#define READS   3

static void print_stats (const char *title, const struct chr_stats *st)
{
  printf ("%s\n", title);
  printf ("   opens       : %d\n", st->open_cnt);
  printf ("   closes      : %d\n", st->close_cnt);
  printf ("   bytes read  : %d\n", st->read_bytes);
  printf ("   bytes written: %d\n", st->write_bytes);
}

int main (int argc, char *argv[])
{
  const char *dv = (argc > 1) ? argv[1] : "/dev/myDev";
  char wbuf[CHUNK], rbuf[CHUNK];
  struct chr_stats st;
  int fd, i;
  ssize_t n;

  fd = open (dv, O_RDWR);
  if (fd < 0) {
    fprintf (stderr, "open %s failed: %s\n", dv, strerror (errno));
    return 1;
  }
  printf ("open %s succeeded\n", dv);

  /* ---- several writes ---- */
  memset (wbuf, 'W', sizeof (wbuf));
  for (i = 1; i <= WRITES; i++) {
    n = write (fd, wbuf, CHUNK);
    if (n < 0) {
      fprintf (stderr, "write %d failed: %s\n", i, strerror (errno));
      close (fd);
      return 1;
    }
    printf ("write %d: %zd bytes\n", i, n);
  }

  /* ---- several reads ---- */
  for (i = 1; i <= READS; i++) {
    n = read (fd, rbuf, CHUNK);
    if (n < 0) {
      fprintf (stderr, "read %d failed: %s\n", i, strerror (errno));
      close (fd);
      return 1;
    }
    printf ("read  %d: %zd bytes\n", i, n);
  }

  /* ---- command 1: get the statistics into st ---- */
  memset (&st, 0, sizeof (st));
  if (ioctl (fd, CHR_STATSGET, &st) < 0) {
    fprintf (stderr, "ioctl CHR_STATSGET failed: %s\n", strerror (errno));
    close (fd);
    return 1;
  }
  print_stats ("statistics (CHR_STATSGET):", &st);

  /* ---- command 2: clear the statistics (no argument -> 0) ---- */
  if (ioctl (fd, CHR_STATSCLR, 0) < 0) {
    fprintf (stderr, "ioctl CHR_STATSCLR failed: %s\n", strerror (errno));
    close (fd);
    return 1;
  }
  printf ("statistics cleared (CHR_STATSCLR)\n");

  /* ---- get them again - should all be 0 ---- */
  memset (&st, 0xff, sizeof (st));   /* garbage, to see the driver really filled it */
  if (ioctl (fd, CHR_STATSGET, &st) < 0) {
    fprintf (stderr, "ioctl CHR_STATSGET failed: %s\n", strerror (errno));
    close (fd);
    return 1;
  }
  print_stats ("statistics after clear:", &st);

  close (fd);

  if (st.open_cnt == 0 && st.close_cnt == 0 &&
      st.read_bytes == 0 && st.write_bytes == 0) {
    printf ("OK - statistics were cleared\n");
    return 0;
  }
  printf ("FAILED - statistics were not cleared\n");
  return 1;
}
