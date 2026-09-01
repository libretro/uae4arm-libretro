#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "uae.h"
#include "options.h"
#include "gui.h"
#include "memory.h"
#include "newcpu.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include "inputdevice.h"
#include "savestate.h"
#include "picasso96.h"

//#include <png.h>

#ifndef VITA
#include <linux/fb.h>
#include <sys/ioctl.h>
#endif

extern unsigned int retrow,retroh;

#include "libretro-core.h"



static int fbdev = -1;
static unsigned int current_vsync_frame = 0;
uae_u32 time_per_frame = 20000; // Default for PAL (50 Hz): 20000 microsecs
static uae_u32 last_synctime;

/* Possible screen modes (x and y resolutions) */
#define MAX_SCREEN_MODES 7
static int x_size_table[MAX_SCREEN_MODES] = { 640, 640, 720,800, 1024, 1152, 1280 };
static int y_size_table[MAX_SCREEN_MODES] = { 400, 480, 568,480,  768,  864,  960 };

static int red_bits, green_bits, blue_bits;
static int red_shift, green_shift, blue_shift;

struct PicassoResolution *DisplayModes;
struct MultiDisplay Displays[MAX_DISPLAYS];

int screen_is_picasso = 0;

int delay_savestate_frame = 0;


static uae_u32 next_synctime = 0;

int graphics_setup (void)
{
#ifdef PICASSO96
  picasso_InitResolutions();
  InitPicasso96();
#endif
  return 1;
}


#ifdef WITH_LOGGING

//retro_surface_t *liveInfo = NULL;
//TTF_Font *liveFont = NULL;
int liveInfoCounter = 0;
void ShowLiveInfo(char *msg)
{
  printf("%s\n", msg);
}

void RefreshLiveInfo()
{
}

#endif


void InitAmigaVidMode(struct uae_prefs *p)
{
  LOGI("retro:(%d,%d) gfx(%d,%d) res %i,led:%d\n",retrow,retroh,p->gfx_size.width,p->gfx_size.height,p->gfx_resolution,currprefs.leds_on_screen);
  /* Initialize structure for Amiga video modes */
  gfxvidinfo.drawbuffer.pixbytes = 2;
  gfxvidinfo.drawbuffer.bufmem = (uae_u8 *)Retro_Screen;
  gfxvidinfo.drawbuffer.outwidth = p->gfx_size.width;
  gfxvidinfo.drawbuffer.outheight = p->gfx_size.height << p->gfx_vresolution;
  gfxvidinfo.drawbuffer.rowbytes =p->gfx_size.width * 2;// retro_screen_surface->pitch;
}


void graphics_subshutdown (void)
{
}


static int CalcPandoraWidth(struct uae_prefs *p)
{
  int amigaWidth = p->gfx_size.width;
  int amigaHeight = p->gfx_size.height;
  int pandHeight = 480;
  
  p->gfx_resolution = p->gfx_size.width > 600 ? 1 : 0;
  if(amigaWidth > 600)
    amigaWidth = amigaWidth / 2; // Hires selected, but we calc in lores
  int pandWidth = (amigaWidth * pandHeight) / amigaHeight;
  pandWidth = pandWidth & (~1);
  if((pandWidth * amigaHeight) / pandHeight < amigaWidth)
    pandWidth += 2;
  if(pandWidth > 800)
    pandWidth = 800;
  return pandWidth;
}


static void open_screen(struct uae_prefs *p)
{
  char layersize[20];

  graphics_subshutdown();
  
  if(!screen_is_picasso)
  {
    int layerwidth = CalcPandoraWidth(p);
	  snprintf(layersize, 20, "%dx480", layerwidth);
  }
  else
  {
#ifdef PICASSO96
    if(picasso_vidinfo.height < 480)
	    snprintf(layersize, 20, "%dx480", picasso_vidinfo.width);
	  else
	    snprintf(layersize, 20, "%dx%d", picasso_vidinfo.width, picasso_vidinfo.height);
#endif
  }
  

  if(!screen_is_picasso)
  {

  }
  else
  {
  }
  if(retro_screen_surface != NULL)
  {
    InitAmigaVidMode(p);
    init_row_map();
  }
  
  current_vsync_frame = 0;
}


void update_display(struct uae_prefs *p)
{
  open_screen(p);
    
  framecnt = 1; // Don't draw frame before reset done
}


int check_prefs_changed_gfx (void)
{
  int changed = 0;
  
  if(currprefs.gfx_size.height != changed_prefs.gfx_size.height ||
     currprefs.gfx_size.width != changed_prefs.gfx_size.width ||
     currprefs.gfx_resolution != changed_prefs.gfx_resolution ||
		 currprefs.gfx_vresolution != changed_prefs.gfx_vresolution)
  {
  	cfgfile_configuration_change(1);
    currprefs.gfx_size.height = changed_prefs.gfx_size.height;
    currprefs.gfx_size.width = changed_prefs.gfx_size.width;
    currprefs.gfx_resolution = changed_prefs.gfx_resolution;
		currprefs.gfx_vresolution = changed_prefs.gfx_vresolution;
    update_display(&currprefs);
    changed = 1;
  }
  if (currprefs.leds_on_screen != changed_prefs.leds_on_screen ||
      currprefs.pandora_hide_idle_led != changed_prefs.pandora_hide_idle_led ||
      currprefs.pandora_vertical_offset != changed_prefs.pandora_vertical_offset)	
  {
    currprefs.leds_on_screen = changed_prefs.leds_on_screen;
    currprefs.pandora_hide_idle_led = changed_prefs.pandora_hide_idle_led;
    currprefs.pandora_vertical_offset = changed_prefs.pandora_vertical_offset;
    changed = 1;
  }
  if (currprefs.chipset_refreshrate != changed_prefs.chipset_refreshrate) 
  {
  	currprefs.chipset_refreshrate = changed_prefs.chipset_refreshrate;
	  init_hz_normal ();
	  changed = 1;
  }

	currprefs.filesys_limit = changed_prefs.filesys_limit;
	currprefs.harddrive_read_only = changed_prefs.harddrive_read_only;
  
  if(changed)
		init_custom ();

  return changed;
}


int lockscr (void)
{
  init_row_map();
  return 1;
}


void unlockscr (void)
{
}


void wait_for_vsync(void)
{

}


bool render_screen (bool immediate)
{
	if (savestate_state == STATE_DOSAVE)
	{
    if(delay_savestate_frame > 0)
      --delay_savestate_frame;
    else
	    savestate_state = 0;
  }

#ifdef WITH_LOGGING
  RefreshLiveInfo();
#endif

	return true;
}


extern void DISK_GUI_change (void);

void show_screen (int mode)
{
  uae_u32 start;

  DISK_GUI_change();

  start = read_processor_time();

  co_switch(mainThread);

  idletime = start - last_synctime;

  last_synctime = read_processor_time();

  if(!screen_is_picasso)
  	gfxvidinfo.drawbuffer.bufmem = (uae_u8 *)retro_screen_surface->pixels;

#if 0
  if (last_synctime - next_synctime > time_per_frame - (uae_u32)5000)
    next_synctime = last_synctime + time_per_frame * (1 + currprefs.gfx_framerate);
  else
    next_synctime = next_synctime + time_per_frame * (1 + currprefs.gfx_framerate);
#endif
}


uae_u32 target_lastsynctime(void)
{
  return last_synctime;
}


bool show_screen_maybe (bool show)
{
	if (show)
		show_screen (0);
	return false;
}


void black_screen_now(void)
{
memset(Retro_Screen,0,retrow*retroh*PITCH);
}


static void graphics_subinit (void)
{
	if (0/*retro_screen_surface == NULL*/)
	{
		fprintf(stderr, "Unable to set video mode\n");
		return;
	}
	else
	{

    InitAmigaVidMode(&currprefs);
	}
}

STATIC_INLINE int bitsInMask (uae_u32 mask)
{
	/* count bits in mask */
	int n = 0;
	while (mask)
	{
		n += mask & 1;
		mask >>= 1;
	}
	return n;
}


STATIC_INLINE int maskShift (uae_u32 mask)
{
	/* determine how far mask is shifted */
	int n = 0;
	while (!(mask & 1))
	{
		n++;
		mask >>= 1;
	}
	return n;
}


static int init_colors (void)
{
  int red_bits, green_bits, blue_bits;
  int red_shift, green_shift, blue_shift;

	/* Truecolor: */
	red_bits = 5;//bitsInMask(retro_screen_surface->format->Rmask);
	green_bits = 6;//bitsInMask(retro_screen_surface->format->Gmask);
	blue_bits = 5;//bitsInMask(retro_screen_surface->format->Bmask);
	red_shift = 11;//maskShift(retro_screen_surface->format->Rmask);
	green_shift = 5;//maskShift(retro_screen_surface->format->Gmask);
	blue_shift = 0;//maskShift(retro_screen_surface->format->Bmask);
	alloc_colors64k (red_bits, green_bits, blue_bits, red_shift, green_shift, blue_shift, 0);
	notice_new_xcolors();

	return 1;
}


/*
 * Find the colour depth of the display
 */
static int get_display_depth (void)
{
  return 16;
}


int GetSurfacePixelFormat(void)
{
  int depth = get_display_depth();
  int unit = (depth + 1) & 0xF8;

  return (unit == 8 ? RGBFB_CHUNKY
		: depth == 15 && unit == 16 ? RGBFB_R5G5B5
		: depth == 16 && unit == 16 ? RGBFB_R5G6B5
		: unit == 24 ? RGBFB_B8G8R8
		: unit == 32 ? RGBFB_R8G8B8A8
		: RGBFB_NONE);
}


int graphics_init (bool mousecapture)
{
	graphics_subinit ();

  if (!init_colors ())
		return 0;
    
  return 1;
}

void graphics_leave (void)
{
  graphics_subshutdown ();
}

bool vsync_switchmode (int hz)
{
	int changed_height = changed_prefs.gfx_size.height;
	
	if (hz >= 55)
		hz = 60;
	else
		hz = 50;

  if(hz == 50 && currVSyncRate == 60)
  {
    // Switch from NTSC -> PAL
    switch(changed_height) {
      case 200: changed_height = 240; break;
      case 216: changed_height = 262; break;
      case 240: changed_height = 270; break;
      case 256: changed_height = 270; break;
      case 262: changed_height = 270; break;
      case 270: changed_height = 270; break;
    }
  }
  else if(hz == 60 && currVSyncRate == 50)
  {
    // Switch from PAL -> NTSC
    switch(changed_height) {
      case 200: changed_height = 200; break;
      case 216: changed_height = 200; break;
      case 240: changed_height = 200; break;
      case 256: changed_height = 216; break;
      case 262: changed_height = 216; break;
      case 270: changed_height = 240; break;
    }
  }
  
  if(hz != currVSyncRate) 
  {
    black_screen_now();
    fpscounter_reset();
    time_per_frame = 1000 * 1000 / (hz);
  }
  
  if(!picasso_on && !picasso_requested_on)
    changed_prefs.gfx_size.height = changed_height;
  
  return true;
}


bool target_graphics_buffer_update (void)
{
  bool rate_changed = false;
  
  if(currprefs.gfx_size.height != changed_prefs.gfx_size.height)
  {
    update_display(&changed_prefs);
    rate_changed = true;
  }

	if(rate_changed)
  {
  	black_screen_now();
    fpscounter_reset();
    time_per_frame = 1000 * 1000 / (currprefs.chipset_refreshrate);
  }

  return true;
}


#ifdef PICASSO96


int picasso_palette (struct MyCLUTEntry *CLUT)
{
  int i, changed;

  changed = 0;
  for (i = 0; i < 256; i++) {
    int r = CLUT[i].Red;
    int g = CLUT[i].Green;
    int b = CLUT[i].Blue;
    int value = (r << 16 | g << 8 | b);
  	uae_u32 v = CONVERT_RGB(value);
	  if (v !=  picasso_vidinfo.clut[i]) {
	     picasso_vidinfo.clut[i] = v;
	     changed = 1;
	  } 
  }
  return changed;
}

static int resolution_compare (const void *a, const void *b)
{
  struct PicassoResolution *ma = (struct PicassoResolution *)a;
  struct PicassoResolution *mb = (struct PicassoResolution *)b;
  if (ma->res.width < mb->res.width)
  	return -1;
  if (ma->res.width > mb->res.width)
  	return 1;
  if (ma->res.height < mb->res.height)
  	return -1;
  if (ma->res.height > mb->res.height)
  	return 1;
  return ma->depth - mb->depth;
}
static void sortmodes (void)
{
  int	i = 0, idx = -1;
  int pw = -1, ph = -1;
  while (DisplayModes[i].depth >= 0)
  	i++;
  qsort (DisplayModes, i, sizeof (struct PicassoResolution), resolution_compare);
  for (i = 0; DisplayModes[i].depth >= 0; i++) {
  	if (DisplayModes[i].res.height != ph || DisplayModes[i].res.width != pw) {
	    ph = DisplayModes[i].res.height;
	    pw = DisplayModes[i].res.width;
	    idx++;
	  }
	  DisplayModes[i].residx = idx;
  }
}

static void modesList (void)
{
  int i, j;

  i = 0;
  while (DisplayModes[i].depth >= 0) {
  	write_log ("%d: %s (", i, DisplayModes[i].name);
  	j = 0;
  	while (DisplayModes[i].refresh[j] > 0) {
	    if (j > 0)
	    	write_log (",");
	    write_log ("%d", DisplayModes[i].refresh[j]);
	    j++;
	  }
	  write_log (")\n");
	  i++;
  }
}

void picasso_InitResolutions (void)
{
  struct MultiDisplay *md1;
  int i, count = 0;
  char tmp[200];
  int bit_idx;
  int bits[] = { 8, 16, 32 };
  
  Displays[0].primary = 1;
  Displays[0].disabled = 0;
  Displays[0].rect.left = 0;
  Displays[0].rect.top = 0;
  Displays[0].rect.right = 800;
  Displays[0].rect.bottom = 480;
  sprintf (tmp, "%s (%d*%d)", "Display", Displays[0].rect.right, Displays[0].rect.bottom);
  Displays[0].name = my_strdup(tmp);
  Displays[0].name2 = my_strdup("Display");

  md1 = Displays;
  DisplayModes = md1->DisplayModes = xmalloc (struct PicassoResolution, MAX_PICASSO_MODES);
  for (i = 0; i < MAX_SCREEN_MODES && count < MAX_PICASSO_MODES; i++) {
    for(bit_idx = 0; bit_idx < 3; ++bit_idx) {
      int bitdepth = bits[bit_idx];
      int bit_unit = (bitdepth + 1) & 0xF8;
      int rgbFormat = (bitdepth == 8 ? RGBFB_CLUT : (bitdepth == 16 ? RGBFB_R5G6B5 : RGBFB_R8G8B8A8));
      int pixelFormat = 1 << rgbFormat;
  	  pixelFormat |= RGBFF_CHUNKY;
      
  	  if (bitdepth==16)
  	  {
  	    DisplayModes[count].res.width = x_size_table[i];
  	    DisplayModes[count].res.height = y_size_table[i];
  	    DisplayModes[count].depth = bit_unit >> 3;
        DisplayModes[count].refresh[0] = 50;
        DisplayModes[count].refresh[1] = 60;
        DisplayModes[count].refresh[2] = 0;
        DisplayModes[count].colormodes = pixelFormat;
        sprintf(DisplayModes[count].name, "%dx%d, %d-bit",
  	      DisplayModes[count].res.width, DisplayModes[count].res.height, DisplayModes[count].depth * 8);
  
  	    count++;
      }
    }
  }
  DisplayModes[count].depth = -1;
  sortmodes();
  modesList();
  DisplayModes = Displays[0].DisplayModes;
}
#endif


#ifdef PICASSO96
void gfx_set_picasso_state (int on)
{
	if (on == screen_is_picasso)
		return;

	screen_is_picasso = on;
  open_screen(&currprefs);
  if(retro_screen_surface != NULL)
    picasso_vidinfo.rowbytes	= retrow*PITCH;// retro_screen_surface->pitch;
}

void gfx_set_picasso_modeinfo (uae_u32 w, uae_u32 h, uae_u32 depth, RGBFTYPE rgbfmt)
{
  depth >>= 3;
  if( ((unsigned)picasso_vidinfo.width == w ) &&
    ( (unsigned)picasso_vidinfo.height == h ) &&
    ( (unsigned)picasso_vidinfo.depth == depth ) &&
    ( picasso_vidinfo.selected_rgbformat == rgbfmt) )
  	return;

  picasso_vidinfo.selected_rgbformat = rgbfmt;
  picasso_vidinfo.width = w;
  picasso_vidinfo.height = h;
  picasso_vidinfo.depth = 2; // Native depth
  picasso_vidinfo.extra_mem = 1;

  picasso_vidinfo.pixbytes = 2; // Native bytes
  if (screen_is_picasso)
  {
  	open_screen(&currprefs);
  	//if(retro_screen_surface != NULL)
      picasso_vidinfo.rowbytes	=  retrow*PITCH;//retro_screen_surface->pitch;
    picasso_vidinfo.rgbformat = RGBFB_R5G6B5;
  }
}

uae_u8 *gfx_lock_picasso (void)
{
  picasso_vidinfo.rowbytes =  retrow*PITCH;//retro_screen_surface->pitch;
  return (uae_u8 *)Retro_Screen;//retro_screen_surface->pixels;
}

void gfx_unlock_picasso (bool dorender)
{
  if(dorender)
  {
    render_screen(true);
    show_screen(0);
  }
}

#endif // PICASSO96
