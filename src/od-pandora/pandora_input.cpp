#include "sysconfig.h"
#include "sysdeps.h"
#include "config.h"
#include "options.h"
#include "keyboard.h"
#include "inputdevice.h"


static int joyXviaCustom = 0;
static int joyYviaCustom = 0;
static int joyButXviaCustom[7] = { 0, 0, 0, 0, 0, 0, 0};
static int mouseBut1viaCustom = 0;
static int mouseBut2viaCustom = 0;


#define MAX_MOUSE_BUTTONS	2
#define MAX_MOUSE_AXES	2
#define FIRST_MOUSE_AXIS	0
#define FIRST_MOUSE_BUTTON	MAX_MOUSE_AXES


static int init_mouse (void) 
{
  return 1;
}

static void close_mouse (void) 
{
}

static int acquire_mouse (int num, int flags) 
{
  return 1;
}

static void unacquire_mouse (int num) 
{
}

static int get_mouse_num (void)
{
  return 2;
}

static TCHAR *get_mouse_friendlyname (int mouse)
{
  if(mouse == 0)
    return "Libretro mouse 0";
  else
    return "Libretro mouse 1";
}

static TCHAR *get_mouse_uniquename (int mouse)
{
  if(mouse == 0)
    return "MOUSE0";
  else
    return "MOUSE1";
}

static int get_mouse_widget_num (int mouse)
{
  return MAX_MOUSE_AXES + MAX_MOUSE_BUTTONS;
}

static int get_mouse_widget_first (int mouse, int type)
{
  switch (type) {
  	case IDEV_WIDGET_BUTTON:
	    return FIRST_MOUSE_BUTTON;
	  case IDEV_WIDGET_AXIS:
	    return FIRST_MOUSE_AXIS;
  	case IDEV_WIDGET_BUTTONAXIS:
  	  return MAX_MOUSE_AXES + MAX_MOUSE_BUTTONS; 
  }
  return -1;
}

static int get_mouse_widget_type (int mouse, int num, TCHAR *name, uae_u32 *code)
{
  if (num >= MAX_MOUSE_AXES && num < MAX_MOUSE_AXES + MAX_MOUSE_BUTTONS) {
  	if (name)
	    sprintf (name, "Button %d", num + 1 - MAX_MOUSE_AXES);
	  return IDEV_WIDGET_BUTTON;
  } else if (num < MAX_MOUSE_AXES) {
	  if (name) {
	    if(num == 0)
	      sprintf (name, "X Axis");
	    else if (num == 1)
	      sprintf (name, "Y Axis");
	    else
	      sprintf (name, "Axis %d", num + 1);
	  }
	  return IDEV_WIDGET_AXIS;
  }
  return IDEV_WIDGET_NONE;
}

static void read_mouse (void) 
{
}


static int get_mouse_flags (int num) 
{
  return 0;
}

struct inputdevice_functions inputdevicefunc_mouse = {
  init_mouse, close_mouse, acquire_mouse, unacquire_mouse, read_mouse,
  get_mouse_num, get_mouse_friendlyname, get_mouse_uniquename,
  get_mouse_widget_num, get_mouse_widget_type,
  get_mouse_widget_first,
  get_mouse_flags
};

static void setid (struct uae_input_device *uid, int i, int slot, int sub, int port, int evt, bool gp)
{
	if (gp)
		inputdevice_sparecopy (&uid[i], slot, 0);
  uid[i].eventid[slot][sub] = evt;
  uid[i].port[slot][sub] = port + 1;
}

static void setid_af (struct uae_input_device *uid, int i, int slot, int sub, int port, int evt, int af, bool gp)
{
  setid (uid, i, slot, sub, port, evt, gp);
  uid[i].flags[slot][sub] &= ~ID_FLAG_AUTOFIRE_MASK;
  if (af >= JPORT_AF_NORMAL)
    uid[i].flags[slot][sub] |= ID_FLAG_AUTOFIRE;
}

int input_get_default_mouse (struct uae_input_device *uid, int i, int port, int af, bool gp, bool wheel, bool joymouseswap)
{
  setid (uid, i, ID_AXIS_OFFSET + 0, 0, port, port ? INPUTEVENT_MOUSE2_HORIZ : INPUTEVENT_MOUSE1_HORIZ, gp);
  setid (uid, i, ID_AXIS_OFFSET + 1, 0, port, port ? INPUTEVENT_MOUSE2_VERT : INPUTEVENT_MOUSE1_VERT, gp);
  setid_af (uid, i, ID_BUTTON_OFFSET + 0, 0, port, port ? INPUTEVENT_JOY2_FIRE_BUTTON : INPUTEVENT_JOY1_FIRE_BUTTON, af, gp);
  setid (uid, i, ID_BUTTON_OFFSET + 1, 0, port, port ? INPUTEVENT_JOY2_2ND_BUTTON : INPUTEVENT_JOY1_2ND_BUTTON, gp);

  if (i == 0)
    return 1;
  return 0;
}


static int init_kb (void)
{
  return 1;
}

static void close_kb (void)
{
}

static int acquire_kb (int num, int flags)
{
  return 1;
}

static void unacquire_kb (int num)
{
}

static void read_kb (void)
{
}

static int get_kb_num (void) 
{
  return 1;
}

static TCHAR *get_kb_friendlyname (int kb) 
{
  return strdup("Default Keyboard");
}

static TCHAR *get_kb_uniquename (int kb) 
{
  return strdup("KEYBOARD0");
}

static int get_kb_widget_num (int kb) 
{
  return 255;
}

static int get_kb_widget_first (int kb, int type) 
{
  return 0;
}

static int get_kb_widget_type (int kb, int num, TCHAR *name, uae_u32 *code) 
{
  if(code)
    *code = num;
  return IDEV_WIDGET_KEY;
}

static int get_kb_flags (int num) 
{
  return 0;
}

struct inputdevice_functions inputdevicefunc_keyboard = {
	init_kb, close_kb, acquire_kb, unacquire_kb, read_kb,
	get_kb_num, get_kb_friendlyname, get_kb_uniquename,
	get_kb_widget_num, get_kb_widget_type,
	get_kb_widget_first,
	get_kb_flags
};

int input_get_default_keyboard (int num) 
{
  if (num == 0) {
    return 1;
  }
  return 0;
}


#define MAX_JOY_BUTTONS	7
#define MAX_JOY_AXES	2
#define FIRST_JOY_AXIS	0
#define FIRST_JOY_BUTTON	MAX_JOY_AXES


static int nr_joysticks = 0;
static char JoystickName[MAX_INPUT_DEVICES][80];

static char IsPS3Controller[MAX_INPUT_DEVICES];



static int get_joystick_num (void)
{
  return 2;
}

static int init_joystick (void)
{
  return 1;
}

static void close_joystick (void)
{
}


static int acquire_joystick (int num, int flags)
{
  return 1;
}

static void unacquire_joystick (int num)
{
}

static TCHAR *get_joystick_friendlyname (int joy)
{
  if (joy == 0) 
    return "Libretro Joystick 0";
  else
    return "Libretro Joystick 1";
}

static TCHAR *get_joystick_uniquename (int joy)
{
  if (joy == 0)
    return "JOY0";
  if (joy == 1)
    return "JOY1";
  if (joy == 2)
    return "JOY2";
  if (joy == 3)
    return "JOY3";
  if (joy == 4)
    return "JOY4";
  if (joy == 5)
    return "JOY5";
  if (joy == 6)
    return "JOY6";

  return "JOY7";
}


static int get_joystick_widget_num (int joy)
{
  return MAX_JOY_AXES + MAX_JOY_BUTTONS;
}

static int get_joystick_widget_first (int joy, int type)
{
  switch (type) {
  	case IDEV_WIDGET_BUTTON:
	    return FIRST_JOY_BUTTON;
	  case IDEV_WIDGET_AXIS:
	    return FIRST_JOY_AXIS;
  	case IDEV_WIDGET_BUTTONAXIS:
  	  return MAX_JOY_AXES + MAX_JOY_BUTTONS; 
  }
  return -1;
}

static int get_joystick_widget_type (int joy, int num, TCHAR *name, uae_u32 *code)
{
  if (num >= MAX_JOY_AXES && num < MAX_JOY_AXES + MAX_JOY_BUTTONS) {
  	if (name) {
	    switch(num)
	    {
	      case FIRST_JOY_BUTTON:
	        sprintf (name, "Button X/CD32 red");
	        break;
	      case FIRST_JOY_BUTTON + 1:
	        sprintf (name, "Button B/CD32 blue");
	        break;
	      case FIRST_JOY_BUTTON + 2:
	        sprintf (name, "Button A/CD32 green");
	        break;
	      case FIRST_JOY_BUTTON + 3:
	        sprintf (name, "Button Y/CD32 yellow");
	        break;
	      case FIRST_JOY_BUTTON + 4:
	        sprintf (name, "CD32 start");
	        break;
	      case FIRST_JOY_BUTTON + 5:
	        sprintf (name, "CD32 ffw");
	        break;
	      case FIRST_JOY_BUTTON + 6:
	        sprintf (name, "CD32 rwd");
	        break;
	    }
	  }
	  return IDEV_WIDGET_BUTTON;
  } else if (num < MAX_JOY_AXES) {
	  if (name) {
	    if(num == 0)
	      sprintf (name, "X Axis");
	    else if (num == 1)
	      sprintf (name, "Y Axis");
	    else
	      sprintf (name, "Axis %d", num + 1);
	  }
	  return IDEV_WIDGET_AXIS;
  }
  return IDEV_WIDGET_NONE;
}

static int get_joystick_flags (int num)
{
  return 0;
}


static void read_joystick (void)
{
}

struct inputdevice_functions inputdevicefunc_joystick = {
	init_joystick, close_joystick, acquire_joystick, unacquire_joystick,
	read_joystick, get_joystick_num, get_joystick_friendlyname, get_joystick_uniquename,
	get_joystick_widget_num, get_joystick_widget_type,
	get_joystick_widget_first,
	get_joystick_flags
};

int input_get_default_joystick (struct uae_input_device *uid, int num, int port, int af, int mode, bool gp, bool joymouseswap)
{
  int h, v;

  h = port ? INPUTEVENT_JOY2_HORIZ : INPUTEVENT_JOY1_HORIZ;
  v = port ? INPUTEVENT_JOY2_VERT : INPUTEVENT_JOY1_VERT;

  setid (uid, num, ID_AXIS_OFFSET + 0, 0, port, h, gp);
  setid (uid, num, ID_AXIS_OFFSET + 1, 0, port, v, gp);

  setid_af (uid, num, ID_BUTTON_OFFSET + 0, 0, port, port ? INPUTEVENT_JOY2_FIRE_BUTTON : INPUTEVENT_JOY1_FIRE_BUTTON, af, gp);
  setid (uid, num, ID_BUTTON_OFFSET + 1, 0, port, port ? INPUTEVENT_JOY2_2ND_BUTTON : INPUTEVENT_JOY1_2ND_BUTTON, gp);
  setid (uid, num, ID_BUTTON_OFFSET + 2, 0, port, port ? INPUTEVENT_JOY2_3RD_BUTTON : INPUTEVENT_JOY1_3RD_BUTTON, gp);

  if (mode == JSEM_MODE_JOYSTICK_CD32) {
    setid_af (uid, num, ID_BUTTON_OFFSET + 0, 0, port, port ? INPUTEVENT_JOY2_CD32_RED : INPUTEVENT_JOY1_CD32_RED, af, gp);
    setid (uid, num, ID_BUTTON_OFFSET + 1, 0, port, port ? INPUTEVENT_JOY2_CD32_BLUE : INPUTEVENT_JOY1_CD32_BLUE, gp);
    setid (uid, num, ID_BUTTON_OFFSET + 2, 0, port, port ? INPUTEVENT_JOY2_CD32_GREEN : INPUTEVENT_JOY1_CD32_GREEN, gp);
    setid (uid, num, ID_BUTTON_OFFSET + 3, 0, port, port ? INPUTEVENT_JOY2_CD32_YELLOW : INPUTEVENT_JOY1_CD32_YELLOW, gp);
    setid (uid, num, ID_BUTTON_OFFSET + 4, 0, port, port ? INPUTEVENT_JOY2_CD32_RWD : INPUTEVENT_JOY1_CD32_RWD, gp);
    setid (uid, num, ID_BUTTON_OFFSET + 5, 0, port, port ? INPUTEVENT_JOY2_CD32_FFW : INPUTEVENT_JOY1_CD32_FFW, gp);
    setid (uid, num, ID_BUTTON_OFFSET + 6, 0, port, port ? INPUTEVENT_JOY2_CD32_PLAY : INPUTEVENT_JOY1_CD32_PLAY, gp);

    // mouse left and 'space' events (Not real but very useful?)
    setid(uid, num, ID_BUTTON_OFFSET + 7, 0, port, port ? INPUTEVENT_JOY1_FIRE_BUTTON : INPUTEVENT_JOY2_FIRE_BUTTON, gp);
    setid(uid, num, ID_BUTTON_OFFSET + 8, 0, port, port ? INPUTEVENT_KEY_SPACE : INPUTEVENT_KEY_SPACE, gp);
  }
  if (num == 0) {
    return 1;
  }
  return 0;
}

int input_get_default_joystick_analog (struct uae_input_device *uid, int num, int port, int af, bool joymouseswap) 
{
  return 0;
}


void SimulateMouseOrJoy(int code, int keypressed)
{
  switch(code) {
    case REMAP_MOUSEBUTTON_LEFT:
      mouseBut1viaCustom = keypressed;
      setmousebuttonstate (0, 0, keypressed);
      setmousebuttonstate (1, 0, keypressed);
      break;
      
    case REMAP_MOUSEBUTTON_RIGHT:
      mouseBut2viaCustom = keypressed;
      setmousebuttonstate (0, 1, keypressed);
      setmousebuttonstate (1, 1, keypressed);
      break;
      
    case REMAP_JOYBUTTON_ONE:
      joyButXviaCustom[0] = keypressed;
      setjoybuttonstate (0, 0, keypressed);
      break;
      
    case REMAP_JOYBUTTON_TWO:
      joyButXviaCustom[1] = keypressed;
      setjoybuttonstate (0, 1, keypressed);
      break;
      
    case REMAP_JOY_UP:
      joyYviaCustom = keypressed;
      setjoystickstate (0, 1, keypressed ? -32767 : 0, 32767);
      break;
      
    case REMAP_JOY_DOWN:
      joyYviaCustom = keypressed;
      setjoystickstate (0, 1, keypressed ? 32767 : 0, 32767);
      break;
      
    case REMAP_JOY_LEFT:
      joyXviaCustom = keypressed;
      setjoystickstate (0, 0, keypressed ? -32767 : 0, 32767);
      break;
      
    case REMAP_JOY_RIGHT:
      joyXviaCustom = keypressed;
      setjoystickstate (0, 0, keypressed ? 32767 : 0, 32767);
      break;

    case REMAP_CD32_GREEN:
      joyButXviaCustom[2] = keypressed;
      setjoybuttonstate (0, 2, keypressed);
      break;

    case REMAP_CD32_YELLOW:
      joyButXviaCustom[3] = keypressed;
      setjoybuttonstate (0, 3, keypressed);
      break;

    case REMAP_CD32_PLAY:
      joyButXviaCustom[6] = keypressed;
      setjoybuttonstate (0, 6, keypressed);
      break;

    case REMAP_CD32_FFW:
      joyButXviaCustom[5] = keypressed;
      setjoybuttonstate (0, 5, keypressed);
      break;

    case REMAP_CD32_RWD:
      joyButXviaCustom[4] = keypressed;
      setjoybuttonstate (0, 4, keypressed);
      break;
  }  
}
