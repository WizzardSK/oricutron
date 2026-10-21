/*
**  Oricutron - libretro core
**
**  The libretro frontend of the emulator, in place of main.c's SDL loop.
**  It starts the machine through main.c's own init() with the command line
**  the standalone would be given, runs one emulated frame per retro_run with
**  main.c's frameloop_normal(), hands the ULA's picture and the AY's sound to
**  the frontend, and feeds the frontend's keyboard and pads to the Oric's
**  keyboard matrix.
**
**  This program is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public License
**  as published by the Free Software Foundation, version 2
**  of the License.
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../system.h"
#include "../6502.h"
#include "../via.h"
#include "../8912.h"
#include "../gui.h"
#include "../disk.h"
#include "../monitor.h"
#include "../6551.h"
#include "../machine.h"
#include "../filereq.h"
#include "../msgbox.h"
#include "../main.h"

#include "libretro.h"

/* main.c, machine.c and 8912.c */
SDL_bool init(struct machine *oric, int argc, char *argv[]);
void frameloop_normal(struct machine *oric, SDL_bool *framedone, SDL_bool *needrender);
void once_per_frame(struct machine *oric);
void set_fileprefix(const char *prefix);
int mapkey(struct machine *oric, int key);
void ay_callback(void *dummy, Sint8 *stream, int length);
void resetoric(struct machine *oric, struct osdmenuitem *mitem, int dummy);
extern Uint8 oricpalette[];

#define SCREEN_W 240
#define SCREEN_H 224

static retro_environment_t environ_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_log_printf_t log_cb;

static struct machine oric;
static SDL_bool oric_running;
static Uint32 framebuf[SCREEN_W * SCREEN_H];
static Sint16 soundbuf[AUDIO_BUFLEN * 2];
static Uint32 ticks_ms;
static int frame_hz;

/* The Oric keys held down last frame: the frontend's keyboard and pads are
   sampled every frame and only the changes go to the keyboard matrix */
static unsigned char keys_down[RETROK_LAST];

static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
  va_list ap;
  (void)level;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}

void libretro_log_error(const char *msg)
{
  log_cb(RETRO_LOG_ERROR, "[Oricutron] %s\n", msg);
}

/* --- What the emulator calls and the core has nothing behind ------------- */

Uint32 SDL_GetTicks(void)
{
  return ticks_ms;
}

int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained)
{
  /* Nothing is opened: retro_run pulls each frame's samples through
     ay_callback, the function the emulator would have given SDL */
  if(obtained)
  {
    *obtained = *desired;
    obtained->silence = 0;
  }
  return 0;
}

int SDL_COMPAT_Init(Uint32 flags) { (void)flags; return 0; }
void SDL_COMPAT_WM_SetIcon(SDL_Surface *icon, Uint8 *mask) { (void)icon; (void)mask; }
void SDL_COMPAT_WM_SetCaption(const char *title, const char *icon) { (void)title; (void)icon; }
SDL_bool SDL_COMPAT_IsAppActive(SDL_Event *event) { (void)event; return SDL_TRUE; }
SDL_bool SDL_COMPAT_IsAppFocused(SDL_Event *event) { (void)event; return SDL_TRUE; }
int SDL_COMPAT_EnableKeyRepeat(int delay, int interval) { (void)delay; (void)interval; return 0; }
int SDL_COMPAT_EnableUNICODE(int enable) { (void)enable; return 0; }
SDL_COMPAT_KEY SDL_COMPAT_GetKeysymUnicode(SDL_COMPAT_KEYSYM keysym) { return keysym.unicode; }
SDL_COMPAT_KEY SDL_COMPAT_TranslateUnicode(SDL_COMPAT_KEYSYM keysym) { return keysym.sym; }
int SDL_COMPAT_Flip(SDL_Surface *screen) { (void)screen; return 0; }
int SDL_COMPAT_GetBitsPerPixel(void) { return 32; }
int SDL_COMPAT_WM_ToggleFullScreen(SDL_Surface *surface) { (void)surface; return 0; }
SDL_Surface *SDL_COMPAT_SetVideoMode(int width, int height, int bitsperpixel, Uint32 flags)
{
  (void)width; (void)height; (void)bitsperpixel; (void)flags;
  return NULL;
}
int SDL_COMPAT_SetPalette(SDL_Surface *surface, int flags, SDL_Color *colors, int firstcolor, int ncolors)
{
  (void)surface; (void)flags; (void)colors; (void)firstcolor; (void)ncolors;
  return 0;
}
void SDL_COMPAT_SetEventFilter(SDL_EventFilter filter) { (void)filter; }
void SDL_COMPAT_Quit(SDL_bool freeall) { (void)freeall; }
void SDL_COMPAT_TakeScreenshot(char *fname) { (void)fname; }
const char *SDL_COMPAT_JoystickName(SDL_Joystick *joystick) { (void)joystick; return ""; }

/* The standalone's native menus, file requesters and message boxes: the
   frontend has its own, and a core has no window to put them in */
SDL_bool init_gui_native(struct machine *o) { (void)o; return SDL_TRUE; }
void shut_gui_native(struct machine *o) { (void)o; }
void gui_open_url(const char *url) { (void)url; }

SDL_bool init_filerequester(struct machine *o) { (void)o; return SDL_TRUE; }
void shut_filerequester(struct machine *o) { (void)o; }
SDL_bool filerequester(struct machine *o, char *title, char *path, char *fname, int type)
{
  (void)o; (void)title; (void)path; (void)fname; (void)type;
  return SDL_FALSE;
}

SDL_bool init_msgbox(struct machine *o) { (void)o; return SDL_TRUE; }
void shut_msgbox(struct machine *o) { (void)o; }
SDL_bool msgbox(struct machine *o, int type, char *msg)
{
  (void)o;
  log_cb(RETRO_LOG_INFO, "[Oricutron] %s\n", msg);
  (void)type;
  /* Questions are answered yes, as most of them ask to go on */
  return SDL_TRUE;
}

/* --- Core options -------------------------------------------------------- */

static struct retro_core_option_v2_category option_cats[] = {
  { NULL, NULL, NULL },
};

static struct retro_core_option_v2_definition option_defs[] = {
  {
    "oricutron_machine", "Machine", NULL,
    "The computer emulated. Automatic takes the one a disk image is for, else the Atmos. Applies when the content is loaded again.",
    NULL, NULL,
    {
      { "auto", "Automatic" },
      { "atmos", "Oric Atmos" },
      { "oric1", "Oric-1" },
      { "o16k", "Oric-1 16K" },
      { "telestrat", "Telestrat" },
      { "pravetz", "Pravetz 8D" },
      { NULL, NULL },
    },
    "auto"
  },
  {
    "oricutron_drive", "Disk Drive", NULL,
    "The disk controller. Automatic takes the one a disk image is for (Microdisc when it cannot tell). Applies when the content is loaded again.",
    NULL, NULL,
    {
      { "auto", "Automatic" },
      { "microdisc", "Microdisc" },
      { "jasmin", "Jasmin" },
      { "bd500", "Byte Drive 500" },
      { "pravetz", "Pravetz" },
      { "none", "None" },
      { NULL, NULL },
    },
    "auto"
  },
  {
    "oricutron_turbotape", "Turbo Tape", NULL,
    "Load tape images instantly instead of at the speed of a cassette. Applies when the content is loaded again.",
    NULL, NULL,
    {
      { "enabled", NULL },
      { "disabled", NULL },
      { NULL, NULL },
    },
    "enabled"
  },
  { NULL, NULL, NULL, NULL, NULL, NULL, {{0}}, NULL },
};

static struct retro_core_options_v2 options_v2 = { option_cats, option_defs };

static const char *get_option(const char *key, const char *fallback)
{
  struct retro_variable var = { key, NULL };
  if(environ_cb && environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    return var.value;
  return fallback;
}

/* --- libretro API -------------------------------------------------------- */

void retro_set_environment(retro_environment_t cb)
{
  static const struct retro_controller_description pads[] = {
    { "RetroPad (cursor keys and Space)", RETRO_DEVICE_JOYPAD },
    { NULL, 0 },
  };
  static const struct retro_controller_info ports[] = {
    { pads, 1 },
    { NULL, 0 },
  };
  struct retro_log_callback logging;
  bool no_game = true;
  unsigned version = 0;

  environ_cb = cb;
  log_cb = fallback_log;
  if(cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &logging))
    log_cb = logging.log;

  cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_game);
  cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void *)ports);

  if(cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version) && version >= 2)
    cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2, &options_v2);
  else
  {
    static struct retro_variable vars[] = {
      { "oricutron_machine", "Machine; auto|atmos|oric1|o16k|telestrat|pravetz" },
      { "oricutron_drive", "Disk Drive; auto|microdisc|jasmin|bd500|pravetz|none" },
      { "oricutron_turbotape", "Turbo Tape; enabled|disabled" },
      { NULL, NULL },
    };
    cb(RETRO_ENVIRONMENT_SET_VARIABLES, vars);
  }
}

void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { (void)cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

unsigned retro_api_version(void) { return RETRO_API_VERSION; }

void retro_init(void)
{
  if(!log_cb)
    log_cb = fallback_log;
}

void retro_deinit(void)
{
}

void retro_get_system_info(struct retro_system_info *info)
{
  memset(info, 0, sizeof(*info));
  info->library_name = "Oricutron";
  info->library_version = "1.2.0";
  info->valid_extensions = "tap|ort|dsk|sna";
  info->need_fullpath = true;
  info->block_extract = false;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
  memset(info, 0, sizeof(*info));
  info->geometry.base_width = SCREEN_W;
  info->geometry.base_height = SCREEN_H;
  info->geometry.max_width = SCREEN_W;
  info->geometry.max_height = SCREEN_H;
  info->geometry.aspect_ratio = 4.0f / 3.0f;
  info->timing.fps = frame_hz ? frame_hz : 50;
  info->timing.sample_rate = AUDIO_FREQ;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
  (void)port; (void)device;
}

static void release_keys(void)
{
  int k;
  for(k = 0; k < RETROK_LAST; k++)
  {
    if(keys_down[k])
    {
      ay_keypress(&oric.ay, mapkey(&oric, k), SDL_FALSE);
      keys_down[k] = 0;
    }
  }
}

bool retro_load_game(const struct retro_game_info *game)
{
  enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_XRGB8888;
  const char *sysdir = NULL;
  char prefix[4096];
  char *argv[16];
  int argc = 0;
  const char *machine = get_option("oricutron_machine", "auto");
  const char *drive = get_option("oricutron_drive", "auto");

  if(!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
  {
    log_cb(RETRO_LOG_ERROR, "[Oricutron] The frontend does not take XRGB8888.\n");
    return false;
  }

  /* The ROMs, and the patch and symbol files beside them, are in
     <system>/oricutron/roms/, as in the standalone's own directory */
  if(!environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &sysdir) || !sysdir)
    sysdir = ".";
  snprintf(prefix, sizeof(prefix), "%s%coricutron%c", sysdir, PATHSEP, PATHSEP);
  set_fileprefix(prefix);

  argv[argc++] = "oricutron";
  if(strcmp(machine, "auto") != 0)
  {
    argv[argc++] = "-m";
    argv[argc++] = (char *)machine;
  }
  if(strcmp(drive, "auto") != 0)
  {
    argv[argc++] = "-k";
    argv[argc++] = (char *)drive;
  }
  argv[argc++] = "--turbotape";
  argv[argc++] = strcmp(get_option("oricutron_turbotape", "enabled"), "enabled") == 0 ? "on" : "off";
  if(game && game->path)
    argv[argc++] = (char *)game->path;
  argv[argc] = NULL;

  memset(&oric, 0, sizeof(oric));
  memset(keys_down, 0, sizeof(keys_down));
  ticks_ms = 0;
  if(!init(&oric, argc, argv))
  {
    log_cb(RETRO_LOG_ERROR, "[Oricutron] The machine did not start. Are its ROMs in %sroms%c?\n", prefix, PATHSEP);
    shut(&oric);
    return false;
  }
  oric_running = SDL_TRUE;
  frame_hz = oric.vid_freq ? 50 : 60;
  return true;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num)
{
  (void)type; (void)info; (void)num;
  return false;
}

void retro_unload_game(void)
{
  if(!oric_running)
    return;
  release_keys();
  shut(&oric);
  oric_running = SDL_FALSE;
}

void retro_reset(void)
{
  if(oric_running)
    resetoric(&oric, NULL, 0);
}

/* RetroPad buttons, as the Oric keys most games are played with */
static const struct { unsigned id; int key; } pad_keys[] = {
  { RETRO_DEVICE_ID_JOYPAD_UP,     RETROK_UP },
  { RETRO_DEVICE_ID_JOYPAD_DOWN,   RETROK_DOWN },
  { RETRO_DEVICE_ID_JOYPAD_LEFT,   RETROK_LEFT },
  { RETRO_DEVICE_ID_JOYPAD_RIGHT,  RETROK_RIGHT },
  { RETRO_DEVICE_ID_JOYPAD_B,      RETROK_SPACE },
  { RETRO_DEVICE_ID_JOYPAD_A,      RETROK_SPACE },
  { RETRO_DEVICE_ID_JOYPAD_Y,      RETROK_LCTRL },
  { RETRO_DEVICE_ID_JOYPAD_X,      RETROK_ESCAPE },
  { RETRO_DEVICE_ID_JOYPAD_START,  RETROK_RETURN },
};

static void update_input(void)
{
  unsigned char now[RETROK_LAST];
  size_t i;
  int k;

  input_poll_cb();
  memset(now, 0, sizeof(now));
  for(k = RETROK_BACKSPACE; k < RETROK_LAST; k++)
    if(input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, k))
      now[k] = 1;
  for(i = 0; i < sizeof(pad_keys) / sizeof(pad_keys[0]); i++)
    if(input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, pad_keys[i].id))
      now[pad_keys[i].key] = 1;

  for(k = 0; k < RETROK_LAST; k++)
  {
    if(now[k] != keys_down[k])
    {
      ay_keypress(&oric.ay, mapkey(&oric, k), now[k] ? SDL_TRUE : SDL_FALSE);
      keys_down[k] = now[k];
    }
  }
}

static void present_frame(void)
{
  Uint32 pal[16];
  int i;

  for(i = 0; i < 8; i++)
  {
    pal[i] = ((Uint32)oricpalette[i*3] << 16) | ((Uint32)oricpalette[i*3+1] << 8) | oricpalette[i*3+2];
    pal[i+8] = ((Uint32)(oricpalette[i*3]/2) << 16) | ((Uint32)(oricpalette[i*3+1]/2) << 8) | (oricpalette[i*3+2]/2);
  }
  if(oric.scr)
  {
    for(i = 0; i < SCREEN_W * SCREEN_H; i++)
      framebuf[i] = pal[oric.scr[i] & 15];
  }
  video_cb(framebuf, SCREEN_W, SCREEN_H, SCREEN_W * sizeof(Uint32));
}

void retro_run(void)
{
  SDL_bool framedone = SDL_FALSE, needrender = SDL_FALSE;
  int samples, hz;

  if(!oric_running)
    return;

  update_input();

  /* One emulated frame: frameloop_normal returns when the ULA has drawn its
     last line, or early when the monitor would take over (a breakpoint or a
     JAM opcode). A core cannot show the monitor, so the frame ends there and
     the machine is left running; a CPU stuck on a JAM stays stuck rather
     than hanging the frontend. */
  frameloop_normal(&oric, &framedone, &needrender);
  if(oric.emu_mode != EM_RUNNING)
    oric.emu_mode = EM_RUNNING;
  once_per_frame(&oric);

  hz = oric.vid_freq ? 50 : 60;
  if(hz != frame_hz)
  {
    struct retro_system_av_info av;
    frame_hz = hz;
    retro_get_system_av_info(&av);
    environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &av);
  }
  ticks_ms += 1000 / frame_hz;

  present_frame();

  /* The AY's output for this frame: the cycles the CPU ran since the last
     call are turned into samples by the emulator's own audio callback */
  samples = AUDIO_FREQ / frame_hz;
  ay_callback(&oric.ay, (Sint8 *)soundbuf, samples * 2 * sizeof(Sint16));
  audio_batch_cb(soundbuf, samples);
}

size_t retro_serialize_size(void) { return 0; }
bool retro_serialize(void *data, size_t size) { (void)data; (void)size; return false; }
bool retro_unserialize(const void *data, size_t size) { (void)data; (void)size; return false; }
void retro_cheat_reset(void) {}
void retro_cheat_set(unsigned index, bool enabled, const char *code) { (void)index; (void)enabled; (void)code; }
unsigned retro_get_region(void) { return RETRO_REGION_PAL; }
void *retro_get_memory_data(unsigned id) { (void)id; return NULL; }
size_t retro_get_memory_size(unsigned id) { (void)id; return 0; }
