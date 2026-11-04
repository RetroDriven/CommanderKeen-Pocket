/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef RD_DREAMS_STREAM_H
#define RD_DREAMS_STREAM_H
#include <stddef.h>
#include <stdbool.h>
typedef struct RD_GalaxyFile RD_Stream;
size_t rd_ds_read(void*,size_t,size_t,RD_Stream*);
size_t rd_ds_write(const void*,size_t,size_t,RD_Stream*);
int rd_ds_seek(RD_Stream*,long,int);
long rd_ds_tell(RD_Stream*);
int rd_ds_close(RD_Stream*);
int rd_ds_getc(RD_Stream*);
int rd_ds_putc(int,RD_Stream*);
#endif
