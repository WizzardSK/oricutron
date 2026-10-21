/*
**  Oricutron - libretro core
**
**  The memory stream behind libretro/snapmem.h.
*/

#include <stdlib.h>
#include <string.h>

#include "snapmem.h"

struct snapfile
{
  FILE *fp;                  /* a real file, or NULL for the memory buffer */
  int writing;
  long pos;
};

static unsigned char *out_buf;
static size_t out_size, out_cap;
static const unsigned char *in_buf;
static size_t in_size;

void snapmem_set_input(const void *data, size_t size)
{
  in_buf = (const unsigned char *)data;
  in_size = size;
}

const unsigned char *snapmem_output(size_t *size)
{
  *size = out_size;
  return out_buf;
}

SNAPFILE *snap_fopen(const char *name, const char *mode)
{
  SNAPFILE *f = (SNAPFILE *)calloc(1, sizeof(*f));
  if(!f) return NULL;
  f->writing = strchr(mode, 'w') != NULL;
  if(strcmp(name, SNAPMEM_NAME) != 0)
  {
    f->fp = fopen(name, mode);
    if(!f->fp) { free(f); return NULL; }
    return f;
  }
  if(f->writing)
    out_size = 0;
  else if(!in_buf)
  {
    free(f);
    return NULL;
  }
  return f;
}

size_t snap_fread(void *ptr, size_t size, size_t n, SNAPFILE *f)
{
  size_t want, avail;
  if(f->fp) return fread(ptr, size, n, f->fp);
  if(f->writing || size == 0) return 0;
  want = size * n;
  avail = f->pos < (long)in_size ? in_size - (size_t)f->pos : 0;
  if(want > avail) n = avail / size, want = size * n;
  memcpy(ptr, in_buf + f->pos, want);
  f->pos += (long)want;
  return n;
}

size_t snap_fwrite(const void *ptr, size_t size, size_t n, SNAPFILE *f)
{
  size_t len = size * n, end;
  if(f->fp) return fwrite(ptr, size, n, f->fp);
  if(!f->writing) return 0;
  end = (size_t)f->pos + len;
  if(end > out_cap)
  {
    size_t cap = out_cap ? out_cap : 65536;
    unsigned char *nb;
    while(cap < end) cap *= 2;
    nb = (unsigned char *)realloc(out_buf, cap);
    if(!nb) return 0;
    out_buf = nb;
    out_cap = cap;
  }
  memcpy(out_buf + f->pos, ptr, len);
  f->pos = (long)end;
  if(end > out_size) out_size = end;
  return n;
}

int snap_fseek(SNAPFILE *f, long offset, int whence)
{
  long base, size;
  if(f->fp) return fseek(f->fp, offset, whence);
  size = (long)(f->writing ? out_size : in_size);
  base = whence == SEEK_SET ? 0 : whence == SEEK_CUR ? f->pos : size;
  if(base + offset < 0) return -1;
  f->pos = base + offset;
  return 0;
}

long snap_ftell(SNAPFILE *f)
{
  if(f->fp) return ftell(f->fp);
  return f->pos;
}

int snap_fclose(SNAPFILE *f)
{
  int r = 0;
  if(f->fp) r = fclose(f->fp);
  free(f);
  return r;
}
