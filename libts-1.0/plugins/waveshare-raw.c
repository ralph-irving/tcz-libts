/*
 * tslib driver for WaveShare touchscreens
 * Copyright (C) 2015 Peter Vicman
 * Copyright (C) 2015 Phillip Camp
 * inspiration from derekhe: https://github.com/derekhe/wavesahre-7inch-touchscreen-driver
 *
 * This file is placed under the LGPL.  Please see the file COPYING for more
 * details.
 *
 * usb 6-2: New USB device found, idVendor=0eef, idProduct=0005
 * usb 6-2: New USB device strings: Mfr=1, Product=2, SerialNumber=3
 * usb 6-2: Product: By ZH851
 * usb 6-2: Manufacturer: RPI_TOUCH
 *
 * Touch events consists of ~25, one example is
 * aa 01 03 1b 01 d2 bb 03 01 68 02 cc 00 5d 01 ef 01 5f 01 fe 00 fb 02 37 cc
 * Offset:
 *     0 : Start byte (aa)
 *     1 : Any touch (0=off,1=on)
 *   2-3 : First touch X
 *   4-5 : First touch Y
 *     6 : Multi-touch start (bb)
 *     7 : Bitmask for all touches (bit 0-4 (first-fifth), 0=off, 1=on)
 *   8-9 : Second touch X
 * 10-11 : Second touch Y
 * 12-13 : Third touch X
 * 14-15 : Third touch Y
 * 16-17 : Fourth touch X
 * 18-19 : Fourth touch Y
 * 20-21 : Fifth touch X
 * 22-23 : Fifth touch Y
 *    24 : End byte (cc or 00)
 *
 * Seen screens with 19 and 22 bytes...
 * Driver default allows up to 7 point touch (29bytes) - bit field limit in protocol
 * We only ever process the first touch atm.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/hidraw.h>
#include <stdbool.h>

#include "config.h"
#include "tslib-private.h"

#define DEBUG

struct tslib_input {
  struct tslib_module_info module;
  int len;
};

static int waveshare_read(struct tslib_module_info *inf, struct ts_sample *samp, int nr)
{
  struct tslib_input *i = (struct tslib_input *) inf;
  struct tsdev *ts = inf->dev;
  char *buf;
  int ret, rs, count = 0;

  buf = alloca(i->len * nr);
  ret = read(ts->fd, buf, i->len * nr);
  rs = ret / nr;

  if(ret >= 0) {
    while( ret >= 13 && buf[0] == 0xaa && buf[6] == 0xbb ) {
      /*
        0000271: aa01 00e4 0139 bb01 01e0 0320 01e0 0320 01e0 0320 01e0 0320 cc  .....9..... ... ... ... .

        "aa" is start of the command, "01" means clicked while "00" means unclicked.
        "00e4" and "0139" is the X,Y position (HEX).
        "bb" is start of multi-touch, and the following bytes are the position of each point.
	FIXME check for end byte but we read multiple records here...
	((rs == 22 || rs == 25  && (buf[rs-1] == 00 || buf[rs-1] == 0xcc))
      */
      samp->pressure = buf[1] & 0xff;
      samp->x = ((buf[2] & 0xff) << 8) | (buf[3] & 0xff);
      samp->y = ((buf[4] & 0xff) << 8) | (buf[5] & 0xff);
      gettimeofday(&samp->tv, NULL);

#ifdef DEBUG
      fprintf(stderr, "waveshare: size %x/%x %d\n", ret, rs, nr);
      fprintf(stderr, "waveshare: raw %x %x %x %x %x\n", buf[1], buf[2], buf[3], buf[4], buf[5]);
      fprintf(stderr, "waveshare: sample %dx%d p %d\n", samp->x, samp->y, samp->pressure);
#endif
      samp++;
      count++;
      buf += rs;
      ret -= rs;
    }
  } else {
    if (errno == EAGAIN) {
	//break;
	return count;
    } else if (errno == EINTR) {
	//continue;
	return count;
    }
#ifdef DEBUG
    fprintf(stderr, "waveshare: error %d %d\n", ret, errno);
#endif
    return -1;
  }
#ifdef DEBUG
  fprintf(stderr, "waveshare: count %d/%d\n", count , nr);
#endif
  return count;
}

static int waveshare_fini(struct tslib_module_info *inf)
{
	free(inf);
	return 0;
}

static const struct tslib_ops waveshare_ops =
{
  .read = waveshare_read,
  .fini = waveshare_fini,
};

static int parse_len(struct tslib_module_info *inf, char *str, void *data)
{
  struct tslib_input *i = (struct tslib_input *)inf;
  int v;
  int err = errno;

  v = atoi(str);

  if (v < 0)
    return -1;

  errno = err;
  switch ((int) data) {
    case 1:
      i->len = v;
      fprintf(stderr, "waveshare raw data len: %d bytes\n", i->len);
      break;
    default:
      return -1;
  }
  return 0;
}

static const struct tslib_vars raw_vars[] =
{
  { "len", (void *) 1, parse_len },
};

#define NR_VARS (sizeof(raw_vars) / sizeof(raw_vars[0]))

TSAPI struct tslib_module_info *waveshare_mod_init(struct tsdev *dev, const char *params)
{
  struct tslib_input *i;

  (void) dev;

  i = malloc(sizeof(struct tslib_input));
  if (i == NULL)
    return NULL;

  i->module.ops = &waveshare_ops;
  i->len = 29;

  if (tslib_parse_vars(&i->module, raw_vars, NR_VARS, params)) {
    free(i);
    return NULL;
  }

  return &(i->module);
}

#ifndef TSLIB_STATIC_WAVESHARE_MODULE
  TSLIB_MODULE_INIT(waveshare_mod_init);
#endif
