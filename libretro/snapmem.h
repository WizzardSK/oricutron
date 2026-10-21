/*
**  Oricutron - libretro core
**
**  snapshot.c reads and writes its snapshots through stdio. In the libretro
**  build it includes this after its own includes, so the same code writes
**  save states into memory: the file name SNAPMEM_NAME opens the core's
**  memory buffer, any other name a real file, as before.
*/

#ifndef ORICUTRON_LIBRETRO_SNAPMEM_H
#define ORICUTRON_LIBRETRO_SNAPMEM_H

#include <stdio.h>
#include <stddef.h>

#define SNAPMEM_NAME "\001oricutron-state"

typedef struct snapfile SNAPFILE;

SNAPFILE *snap_fopen(const char *name, const char *mode);
size_t snap_fread(void *ptr, size_t size, size_t n, SNAPFILE *f);
size_t snap_fwrite(const void *ptr, size_t size, size_t n, SNAPFILE *f);
int snap_fseek(SNAPFILE *f, long offset, int whence);
long snap_ftell(SNAPFILE *f);
int snap_fclose(SNAPFILE *f);

/* The libretro side: what the next SNAPMEM_NAME read opens, and what the
   last SNAPMEM_NAME write left (valid until the next write) */
void snapmem_set_input(const void *data, size_t size);
const unsigned char *snapmem_output(size_t *size);

#ifdef ORICUTRON_SNAPSHOT_C
#define FILE   SNAPFILE
#define fopen  snap_fopen
#define fread  snap_fread
#define fwrite snap_fwrite
#define fseek  snap_fseek
#define ftell  snap_ftell
#define fclose snap_fclose
#endif

#endif /* ORICUTRON_LIBRETRO_SNAPMEM_H */
