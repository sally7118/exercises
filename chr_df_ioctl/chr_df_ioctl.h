/*
 * chr_df_ioctl.h - common "language" between the driver and user programs
 *
 * Included by:
 *   - chr_df_mod.c  (kernel module)
 *   - test_ioctl.c  (user space test program)
 *
 * Holds the ioctl command macros and the argument type.
 */
#ifndef CHR_DF_IOCTL_H
#define CHR_DF_IOCTL_H

#include <linux/ioctl.h>   /* _IO, _IOR, ... (usable from kernel and user) */

/* Driver statistics - the argument of CHR_STATSGET */
struct chr_stats {
  int open_cnt;      /* how many times the device was opened  */
  int close_cnt;     /* how many times the device was closed  */
  int read_bytes;    /* how many bytes were read from device  */
  int write_bytes;   /* how many bytes were written to device */
};

#define CHR_TYPE      'C'   /* unique character for all our commands */

/* copy the statistics INTO arg (driver -> user)      */
#define CHR_STATSGET  _IOR(CHR_TYPE, 1, struct chr_stats)
/* clear the statistics - arg is ignored (pass 0)      */
#define CHR_STATSCLR  _IO(CHR_TYPE, 2)

#endif /* CHR_DF_IOCTL_H */
