/*
**  Oricutron - libretro core
**
**  The parts of the SDL 1.2 API the emulator uses, for the libretro build,
**  which links no SDL. system_sdl.h includes this instead of SDL.h when
**  __LIBRETRO__ is defined. Key symbols are libretro's RETROK_* values, which
**  are SDL 1.2's keysyms, so the emulator's key tables take the frontend's
**  keyboard events unchanged. Video, audio and input are driven by
**  libretro/libretro.c; the functions here are what the rest of the code
**  calls and finds nothing behind.
*/

#ifndef ORICUTRON_LIBRETRO_SDL_SHIM_H
#define ORICUTRON_LIBRETRO_SDL_SHIM_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libretro.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t  Uint8;
typedef int8_t   Sint8;
typedef uint16_t Uint16;
typedef int16_t  Sint16;
typedef uint32_t Uint32;
typedef int32_t  Sint32;
typedef uint64_t Uint64;
typedef int64_t  Sint64;

typedef enum { SDL_FALSE = 0, SDL_TRUE = 1 } SDL_bool;

#define SDL_LIL_ENDIAN 1234
#define SDL_BIG_ENDIAN 4321
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define SDL_BYTEORDER SDL_BIG_ENDIAN
#else
#define SDL_BYTEORDER SDL_LIL_ENDIAN
#endif

static inline Uint16 SDL_Swap16(Uint16 x) { return (Uint16)((x << 8) | (x >> 8)); }
static inline Uint32 SDL_Swap32(Uint32 x)
{
  return (x << 24) | ((x << 8) & 0x00ff0000u) | ((x >> 8) & 0x0000ff00u) | (x >> 24);
}

/* --- Keyboard: SDL 1.2 keysyms are libretro's RETROK_* ------------------ */
typedef int SDLKey;
typedef int SDLMod;

#define SDLK_BACKSPACE    RETROK_BACKSPACE
#define SDLK_TAB          RETROK_TAB
#define SDLK_CLEAR        RETROK_CLEAR
#define SDLK_RETURN       RETROK_RETURN
#define SDLK_PAUSE        RETROK_PAUSE
#define SDLK_ESCAPE       RETROK_ESCAPE
#define SDLK_SPACE        RETROK_SPACE
#define SDLK_EQUALS       RETROK_EQUALS
#define SDLK_0            RETROK_0
#define SDLK_1            RETROK_1
#define SDLK_2            RETROK_2
#define SDLK_3            RETROK_3
#define SDLK_4            RETROK_4
#define SDLK_5            RETROK_5
#define SDLK_6            RETROK_6
#define SDLK_7            RETROK_7
#define SDLK_8            RETROK_8
#define SDLK_9            RETROK_9
#define SDLK_a            RETROK_a
#define SDLK_s            RETROK_s
#define SDLK_DELETE       RETROK_DELETE
#define SDLK_KP0          RETROK_KP0
#define SDLK_KP1          RETROK_KP1
#define SDLK_KP2          RETROK_KP2
#define SDLK_KP3          RETROK_KP3
#define SDLK_KP4          RETROK_KP4
#define SDLK_KP5          RETROK_KP5
#define SDLK_KP6          RETROK_KP6
#define SDLK_KP7          RETROK_KP7
#define SDLK_KP8          RETROK_KP8
#define SDLK_KP9          RETROK_KP9
#define SDLK_KP_PERIOD    RETROK_KP_PERIOD
#define SDLK_KP_DIVIDE    RETROK_KP_DIVIDE
#define SDLK_KP_MULTIPLY  RETROK_KP_MULTIPLY
#define SDLK_KP_MINUS     RETROK_KP_MINUS
#define SDLK_KP_PLUS      RETROK_KP_PLUS
#define SDLK_KP_ENTER     RETROK_KP_ENTER
#define SDLK_KP_EQUALS    RETROK_KP_EQUALS
#define SDLK_UP           RETROK_UP
#define SDLK_DOWN         RETROK_DOWN
#define SDLK_RIGHT        RETROK_RIGHT
#define SDLK_LEFT         RETROK_LEFT
#define SDLK_INSERT       RETROK_INSERT
#define SDLK_HOME         RETROK_HOME
#define SDLK_END          RETROK_END
#define SDLK_PAGEUP       RETROK_PAGEUP
#define SDLK_PAGEDOWN     RETROK_PAGEDOWN
#define SDLK_F1           RETROK_F1
#define SDLK_F2           RETROK_F2
#define SDLK_F3           RETROK_F3
#define SDLK_F4           RETROK_F4
#define SDLK_F5           RETROK_F5
#define SDLK_F6           RETROK_F6
#define SDLK_F7           RETROK_F7
#define SDLK_F8           RETROK_F8
#define SDLK_F9           RETROK_F9
#define SDLK_F10          RETROK_F10
#define SDLK_F11          RETROK_F11
#define SDLK_F12          RETROK_F12
#define SDLK_RSHIFT       RETROK_RSHIFT
#define SDLK_LSHIFT       RETROK_LSHIFT
#define SDLK_RCTRL        RETROK_RCTRL
#define SDLK_LCTRL        RETROK_LCTRL
#define SDLK_RALT         RETROK_RALT
#define SDLK_LALT         RETROK_LALT
#define SDLK_HELP         RETROK_HELP
#define SDLK_PRINT        RETROK_PRINT

#define KMOD_NONE   0x0000
#define KMOD_LSHIFT 0x0001
#define KMOD_RSHIFT 0x0002
#define KMOD_SHIFT  (KMOD_LSHIFT|KMOD_RSHIFT)

typedef struct SDL_keysym
{
  Uint8  scancode;
  SDLKey sym;
  SDLMod mod;
  Uint16 unicode;
} SDL_keysym;

/* --- Events ------------------------------------------------------------- */
enum
{
  SDL_NOEVENT = 0,
  SDL_ACTIVEEVENT,
  SDL_KEYDOWN,
  SDL_KEYUP,
  SDL_MOUSEMOTION,
  SDL_MOUSEBUTTONDOWN,
  SDL_MOUSEBUTTONUP,
  SDL_JOYAXISMOTION,
  SDL_JOYBALLMOTION,
  SDL_JOYHATMOTION,
  SDL_JOYBUTTONDOWN,
  SDL_JOYBUTTONUP,
  SDL_QUIT,
  SDL_MULTIGESTURE,
  SDL_WINDOWEVENT
};

#define SDL_BUTTON_LEFT   1
#define SDL_BUTTON_MIDDLE 2
#define SDL_BUTTON_RIGHT  3

#define SDL_HAT_CENTERED 0x00
#define SDL_HAT_UP       0x01
#define SDL_HAT_RIGHT    0x02
#define SDL_HAT_DOWN     0x04
#define SDL_HAT_LEFT     0x08

#define SDL_DEFAULT_REPEAT_DELAY    500
#define SDL_DEFAULT_REPEAT_INTERVAL 30

typedef struct { Uint8 type; Uint8 gain; Uint8 state; } SDL_ActiveEvent;
typedef struct { Uint8 type; Uint8 which; Uint8 state; SDL_keysym keysym; } SDL_KeyboardEvent;
typedef struct { Uint8 type; Uint8 which; Uint8 state; Uint16 x, y; Sint16 xrel, yrel; } SDL_MouseMotionEvent;
typedef struct { Uint8 type; Uint8 which; Uint8 button; Uint8 state; Uint16 x, y; } SDL_MouseButtonEvent;
typedef struct { Uint8 type; Uint8 which; Uint8 axis; Sint16 value; } SDL_JoyAxisEvent;
typedef struct { Uint8 type; Uint8 which; Uint8 hat; Uint8 value; } SDL_JoyHatEvent;
typedef struct { Uint8 type; Uint8 which; Uint8 button; Uint8 state; } SDL_JoyButtonEvent;
typedef struct { Uint32 type; Uint16 numFingers; } SDL_MultiGestureEvent;

typedef union SDL_Event
{
  Uint8 type;
  SDL_ActiveEvent active;
  SDL_KeyboardEvent key;
  SDL_MouseMotionEvent motion;
  SDL_MouseButtonEvent button;
  SDL_JoyAxisEvent jaxis;
  SDL_JoyHatEvent jhat;
  SDL_JoyButtonEvent jbutton;
} SDL_Event;

typedef int (*SDL_EventFilter)(const SDL_Event *event);

static inline int SDL_PollEvent(SDL_Event *event) { (void)event; return 0; }
static inline int SDL_WaitEvent(SDL_Event *event) { (void)event; return 0; }

/* --- Video -------------------------------------------------------------- */
typedef struct SDL_Color { Uint8 r, g, b, unused; } SDL_Color;
typedef struct SDL_Rect { Sint16 x, y; Uint16 w, h; } SDL_Rect;
typedef struct SDL_PixelFormat { Uint8 BitsPerPixel; Uint8 BytesPerPixel; } SDL_PixelFormat;
typedef struct SDL_Surface
{
  Uint32 flags;
  SDL_PixelFormat *format;
  int w, h;
  Uint16 pitch;
  void *pixels;
} SDL_Surface;
typedef struct SDL_VideoInfo { int current_w, current_h; SDL_PixelFormat *vfmt; } SDL_VideoInfo;

#define SDL_SWSURFACE  0x00000000
#define SDL_HWSURFACE  0x00000001
#define SDL_HWPALETTE  0x20000000
#define SDL_FULLSCREEN 0x80000000
#define SDL_OPENGL     0x00000002
#define SDL_LOGPAL     0x01
#define SDL_PHYSPAL    0x02
#define SDL_MUSTLOCK(s) 0

static inline Uint32 SDL_MapRGB(const SDL_PixelFormat *fmt, Uint8 r, Uint8 g, Uint8 b)
{
  (void)fmt;
  return ((Uint32)r << 16) | ((Uint32)g << 8) | b;
}
static inline int SDL_FillRect(SDL_Surface *dst, SDL_Rect *rect, Uint32 color)
{ (void)dst; (void)rect; (void)color; return 0; }
static inline int SDL_LockSurface(SDL_Surface *s) { (void)s; return 0; }
static inline void SDL_UnlockSurface(SDL_Surface *s) { (void)s; }
static inline SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int w, int h, int depth,
  Uint32 rm, Uint32 gm, Uint32 bm, Uint32 am)
{ (void)flags; (void)w; (void)h; (void)depth; (void)rm; (void)gm; (void)bm; (void)am; return NULL; }
static inline void SDL_FreeSurface(SDL_Surface *s) { (void)s; }
static inline SDL_Surface *SDL_LoadBMP(const char *file) { (void)file; return NULL; }
static inline int SDL_WM_ToggleFullScreen(SDL_Surface *s) { (void)s; return 0; }
static inline void SDL_WM_SetCaption(const char *title, const char *icon) { (void)title; (void)icon; }

/* --- Audio: the frontend pulls a frame's samples through ay_callback ---- */
#define AUDIO_S16SYS 0x8010
#define AUDIO_S16LSB 0x8010

typedef struct SDL_AudioSpec
{
  int freq;
  Uint16 format;
  Uint8 channels;
  Uint8 silence;
  Uint16 samples;
  Uint16 padding;
  Uint32 size;
  void (*callback)(void *userdata, Uint8 *stream, int len);
  void *userdata;
} SDL_AudioSpec;

typedef struct SDL_AudioCVT
{
  int needed;
  Uint8 *buf;
  int len;
  int len_cvt;
  int len_mult;
} SDL_AudioCVT;

int  SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained);
static inline void SDL_CloseAudio(void) {}
static inline void SDL_PauseAudio(int pause_on) { (void)pause_on; }
static inline void SDL_LockAudio(void) {}
static inline void SDL_UnlockAudio(void) {}
static inline int SDL_BuildAudioCVT(SDL_AudioCVT *cvt, Uint16 sf, Uint8 sc, int sr, Uint16 df, Uint8 dc, int dr)
{
  (void)sf; (void)sc; (void)sr; (void)df; (void)dc; (void)dr;
  memset(cvt, 0, sizeof(*cvt));
  cvt->len_mult = 1;
  return 0;
}
static inline int SDL_ConvertAudio(SDL_AudioCVT *cvt) { (void)cvt; return 0; }

/* --- Joysticks: the frontend's pads come in as Oric keys ---------------- */
typedef struct SDL_Joystick SDL_Joystick;
static inline SDL_Joystick *SDL_JoystickOpen(int index) { (void)index; return NULL; }
static inline void SDL_JoystickClose(SDL_Joystick *j) { (void)j; }
static inline const char *SDL_JoystickName(int index) { (void)index; return ""; }
static inline int SDL_JoystickEventState(int state) { (void)state; return 0; }
static inline int SDL_NumJoysticks(void) { return 0; }

/* --- Files -------------------------------------------------------------- */
typedef struct SDL_RWops { FILE *fp; } SDL_RWops;

static inline SDL_RWops *SDL_RWFromFile(const char *file, const char *mode)
{
  SDL_RWops *rw;
  FILE *fp = fopen(file, mode);
  if(!fp) return NULL;
  rw = (SDL_RWops *)malloc(sizeof(SDL_RWops));
  if(!rw) { fclose(fp); return NULL; }
  rw->fp = fp;
  return rw;
}
static inline size_t SDL_RWread(SDL_RWops *rw, void *ptr, size_t size, size_t n)
{ return fread(ptr, size, n, rw->fp); }
static inline size_t SDL_RWwrite(SDL_RWops *rw, const void *ptr, size_t size, size_t n)
{ return fwrite(ptr, size, n, rw->fp); }
static inline long SDL_RWseek(SDL_RWops *rw, long offset, int whence)
{ return fseek(rw->fp, offset, whence) == 0 ? ftell(rw->fp) : -1; }
static inline long SDL_RWtell(SDL_RWops *rw) { return ftell(rw->fp); }
static inline int SDL_RWclose(SDL_RWops *rw) { int r = fclose(rw->fp); free(rw); return r; }

/* --- Init, time, errors -------------------------------------------------- */
#define SDL_INIT_TIMER    0x00000001
#define SDL_INIT_AUDIO    0x00000010
#define SDL_INIT_VIDEO    0x00000020
#define SDL_INIT_JOYSTICK 0x00000200

static inline int SDL_InitSubSystem(Uint32 flags) { (void)flags; return 0; }
static inline void SDL_Quit(void) {}
static inline const char *SDL_GetError(void) { return "not available in the libretro core"; }
Uint32 SDL_GetTicks(void);
static inline void SDL_Delay(Uint32 ms) { (void)ms; }

#ifdef __cplusplus
}
#endif

#endif /* ORICUTRON_LIBRETRO_SDL_SHIM_H */
