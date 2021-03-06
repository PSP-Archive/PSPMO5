/*
 *  Copyright (C) 2006 Ludovic Jacomme (ludovic.jacomme@gmail.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>

#include <pspctrl.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspiofilemgr.h>

#include "psp_sdl.h"
#include "psp_kbd.h"
#include "psp_menu.h"
#include "psp_fmgr.h"
#include "psp_menu_kbd.h"
#include "psp_danzeff.h"
#include "psp_irkeyb.h"

# define MENU_KBD_SKIN       0
# define MENU_KBD_KBD_SELECT 1
# define MENU_KBD_UP         2
# define MENU_KBD_DOWN       3
# define MENU_KBD_LEFT       4
# define MENU_KBD_RIGHT      5
# define MENU_KBD_CROSS      6
# define MENU_KBD_SQUARE     7
# define MENU_KBD_TRIANGLE   8
# define MENU_KBD_CIRCLE     9
# define MENU_KBD_LTRIGGER  10
# define MENU_KBD_RTRIGGER  11
# define MENU_KBD_JOY_UP    12
# define MENU_KBD_JOY_DOWN  13
# define MENU_KBD_JOY_LEFT  14
# define MENU_KBD_JOY_RIGHT 15

# define MENU_KBD_LOAD      16
# define MENU_KBD_SAVE      17
# define MENU_KBD_HOTKEYS   18
# define MENU_KBD_RESET     19

# define MENU_KBD_BACK      20

# define MAX_MENU_KBD_ITEM (MENU_KBD_BACK + 1)

  static menu_item_t menu_list[] =
  { 
   { "Skin     :" },
   { "Mapping  :" },

   { "Up       :" },
   { "Down     :" },
   { "Left     :" },
   { "Right    :" },

   { "Cross    :" },
   { "Square   :" },
   { "Triangle :" },
   { "Circle   :" },

   { "LTrigger :" },
   { "RTrigger :" },

   { "JoyUp    :" },
   { "JoyDown  :" },
   { "JoyLeft  :" },
   { "JoyRight :" },

   { "Load Keyboard" },
   { "Save Keyboard" },
   { "Set Hotkeys" },
   { "Reset Keyboard" },

   { "Back to Menu" }
  };

  static int cur_menu_id = MENU_KBD_LOAD;

  static int loc_kbd_mapping[ KBD_ALL_BUTTONS ];
  static int loc_kbd_mapping_L[ KBD_ALL_BUTTONS ];
  static int loc_kbd_mapping_R[ KBD_ALL_BUTTONS ];

  static int psp_menu_dirty = 1;

  int menu_kbd_selected = -1;
  
static int
psp_kbd_menu_id_to_key_id(int menu_id)
{
  int kbd_id = 0;

  switch ( menu_id ) 
  {
    case MENU_KBD_UP        : kbd_id = KBD_UP;        break;
    case MENU_KBD_DOWN      : kbd_id = KBD_DOWN;      break;
    case MENU_KBD_LEFT      : kbd_id = KBD_LEFT;      break;
    case MENU_KBD_RIGHT     : kbd_id = KBD_RIGHT;     break;
    case MENU_KBD_TRIANGLE  : kbd_id = KBD_TRIANGLE;  break;
    case MENU_KBD_CROSS     : kbd_id = KBD_CROSS;     break;
    case MENU_KBD_SQUARE    : kbd_id = KBD_SQUARE;    break;
    case MENU_KBD_CIRCLE    : kbd_id = KBD_CIRCLE;    break;
    case MENU_KBD_LTRIGGER  : kbd_id = KBD_LTRIGGER;  break;
    case MENU_KBD_RTRIGGER  : kbd_id = KBD_RTRIGGER;  break;
    case MENU_KBD_JOY_UP    : kbd_id = KBD_JOY_UP;    break;
    case MENU_KBD_JOY_DOWN  : kbd_id = KBD_JOY_DOWN;  break;
    case MENU_KBD_JOY_LEFT  : kbd_id = KBD_JOY_LEFT;  break;
    case MENU_KBD_JOY_RIGHT : kbd_id = KBD_JOY_RIGHT; break;
  }
  return kbd_id;
}

static void 
psp_display_screen_kbd_menu(void)
{
  char buffer[32];
  char *scan;
  int menu_id = 0;
  int kbd_id  = 0;
  int mo5_key = 0;
  int color   = 0;
  int x       = 0;
  int y       = 0;
  int y_step  = 0;

  //if (psp_menu_dirty) 
  {
    psp_sdl_blit_help();
    psp_menu_dirty = 0;

    psp_sdl_draw_rectangle(10,10,459,249,PSP_MENU_BORDER_COLOR,0);
    psp_sdl_draw_rectangle(11,11,457,247,PSP_MENU_BORDER_COLOR,0);

    psp_sdl_back2_print( 30, 6, " Start+L+R: EXIT ", PSP_MENU_WARNING_COLOR);

    psp_display_screen_menu_battery();

    psp_sdl_back2_print( 310, 6, " Start: Keyboard ", PSP_MENU_NOTE_COLOR);

    psp_sdl_back2_print(30, 254, " []: Cancel  O/X: Valid  Select: Back ", 
                       PSP_MENU_BORDER_COLOR);
    psp_sdl_back2_print(370, 254, " By Zx-81 ",
                       PSP_MENU_AUTHOR_COLOR);
  }
  
  x      = 20;
  y      = 25;
  y_step = 10;
  
  for (menu_id = 0; menu_id < MAX_MENU_KBD_ITEM; menu_id++) 
  {
    if (cur_menu_id == menu_id) color = PSP_MENU_SEL_COLOR;
    else 
    if (menu_id == MENU_KBD_KBD_SELECT) color = PSP_MENU_NOTE_COLOR;
    else                                color = PSP_MENU_TEXT_COLOR;

    psp_sdl_back2_print(x, y, menu_list[menu_id].title, color);

    if (menu_id == MENU_KBD_SKIN) {
      snprintf(buffer, 30, psp_kbd_skin_dir[psp_kbd_skin]);
      scan = strchr(buffer, '/');
      if (scan) *scan = 0;
      psp_sdl_back2_print(110, y, buffer, color);
    } else
    if (menu_id == MENU_KBD_KBD_SELECT) {

      if (menu_kbd_selected == -1) sprintf(buffer, "standard");
      else
      if (menu_kbd_selected == KBD_LTRIGGER_MAPPING) sprintf(buffer, "left");
      else
      if (menu_kbd_selected == KBD_RTRIGGER_MAPPING) sprintf(buffer, "right");

      string_fill_with_space(buffer, 20);
      psp_sdl_back2_print(110, y, buffer, color);

    } else
    if ((menu_id >= MENU_KBD_UP       ) && 
        (menu_id <= MENU_KBD_JOY_RIGHT)) 
    {
      kbd_id  = psp_kbd_menu_id_to_key_id(menu_id);

      if (menu_kbd_selected == KBD_NORMAL_MAPPING  ) mo5_key = loc_kbd_mapping[kbd_id];
      else
      if (menu_kbd_selected == KBD_LTRIGGER_MAPPING) mo5_key = loc_kbd_mapping_L[kbd_id];
      else
      if (menu_kbd_selected == KBD_RTRIGGER_MAPPING) mo5_key = loc_kbd_mapping_R[kbd_id];

      if ((mo5_key >= 0) && (mo5_key < MO5_MAX_KEY)) {
        strcpy(buffer, psp_mo5_key_info[mo5_key].name);
      } else 
      if (mo5_key == KBD_UNASSIGNED) {
        sprintf(buffer, "UNASSIGNED");
      } else
      if (mo5_key == KBD_LTRIGGER_MAPPING) {
        sprintf(buffer, "L MAPPING");
      } else
      if (mo5_key == KBD_RTRIGGER_MAPPING) {
        sprintf(buffer, "R MAPPING");
      } else {
        sprintf(buffer, "KEY %d", mo5_key);
      }
      string_fill_with_space(buffer, 12);
      psp_sdl_back2_print(110, y, buffer, color);

      if (menu_id == MENU_KBD_JOY_RIGHT) {
        y += y_step;
      }
    }
    y += y_step;
  }

  psp_menu_display_save_name();
}

static void
psp_keyboard_menu_update_lr(void)
{
  int kbd_id;

  for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++) {
    if (loc_kbd_mapping[kbd_id] == KBD_LTRIGGER_MAPPING) {
      loc_kbd_mapping_L[kbd_id] = KBD_LTRIGGER_MAPPING;
      loc_kbd_mapping_R[kbd_id] = KBD_LTRIGGER_MAPPING;
    } else 
    if (loc_kbd_mapping[kbd_id] == KBD_RTRIGGER_MAPPING) {
      loc_kbd_mapping_L[kbd_id] = KBD_RTRIGGER_MAPPING;
      loc_kbd_mapping_R[kbd_id] = KBD_RTRIGGER_MAPPING;
    }
  }
}

static void
psp_keyboard_menu_reset_kbd(void)
{
  psp_display_screen_kbd_menu();
  psp_sdl_back2_print(270, 80, "Reset Keyboard !", 
                     PSP_MENU_WARNING_COLOR);
  psp_menu_dirty = 1;
  psp_sdl_flip();
  psp_kbd_reset_mapping();
  sleep(1);

  memcpy(loc_kbd_mapping, psp_kbd_mapping, sizeof(psp_kbd_mapping));
  memcpy(loc_kbd_mapping_L, psp_kbd_mapping_L, sizeof(psp_kbd_mapping_L));
  memcpy(loc_kbd_mapping_R, psp_kbd_mapping_R, sizeof(psp_kbd_mapping_R));
}

static void
psp_keyboard_menu_hotkeys(void)
{
  psp_display_screen_kbd_menu();
  psp_sdl_back2_print(270, 80, "Set Hotkeys !", 
                     PSP_MENU_WARNING_COLOR);
  psp_menu_dirty = 1;
  psp_sdl_flip();
  psp_kbd_reset_hotkeys();
  sleep(1);

  memcpy(loc_kbd_mapping, psp_kbd_mapping, sizeof(psp_kbd_mapping));
  memcpy(loc_kbd_mapping_L, psp_kbd_mapping_L, sizeof(psp_kbd_mapping_L));
  memcpy(loc_kbd_mapping_R, psp_kbd_mapping_R, sizeof(psp_kbd_mapping_R));
}

static void
psp_keyboard_menu_load()
{
  int ret;

  ret = psp_fmgr_menu(FMGR_FORMAT_KBD);
  if (ret ==  1) /* load OK */
  {
    psp_display_screen_kbd_menu();
    psp_sdl_back2_print(270, 80, "File loaded !", 
                       PSP_MENU_NOTE_COLOR);
    psp_menu_dirty = 1;
    psp_sdl_flip();
    sleep(1);
  }
  else 
  if (ret == -1) /* Load Error */
  {
    psp_display_screen_kbd_menu();
    psp_sdl_back2_print(270, 80, "Can't load file !", 
                       PSP_MENU_WARNING_COLOR);
    psp_menu_dirty = 1;
    psp_sdl_flip();
    sleep(1);
  }

  memcpy(loc_kbd_mapping  , psp_kbd_mapping, sizeof(psp_kbd_mapping));
  memcpy(loc_kbd_mapping_L, psp_kbd_mapping_L, sizeof(psp_kbd_mapping_L));
  memcpy(loc_kbd_mapping_R, psp_kbd_mapping_R, sizeof(psp_kbd_mapping_R));
}

static void
psp_keyboard_menu_mapping(int kbd_id, int step)
{
  int *ploc_kbd_mapping;

  ploc_kbd_mapping = loc_kbd_mapping;
  
  if (menu_kbd_selected == KBD_LTRIGGER_MAPPING) {
    ploc_kbd_mapping = loc_kbd_mapping_L;
  } else
  if (menu_kbd_selected == KBD_RTRIGGER_MAPPING) {
    ploc_kbd_mapping = loc_kbd_mapping_R;
  }

  if (step < 0) ploc_kbd_mapping[kbd_id]--;
  else 
  if (step > 0) ploc_kbd_mapping[kbd_id]++;

  if (ploc_kbd_mapping[kbd_id] <  -3) ploc_kbd_mapping[kbd_id] = MO5_MAX_KEY-1;
  else
  if (ploc_kbd_mapping[kbd_id] >= MO5_MAX_KEY) ploc_kbd_mapping[kbd_id] = -3;
}

static void
psp_keyboard_menu_save()
{
  int error;

  psp_keyboard_menu_update_lr();

  memcpy(psp_kbd_mapping  , loc_kbd_mapping  , sizeof(psp_kbd_mapping));
  memcpy(psp_kbd_mapping_L, loc_kbd_mapping_L, sizeof(psp_kbd_mapping_L));
  memcpy(psp_kbd_mapping_R, loc_kbd_mapping_R, sizeof(psp_kbd_mapping_R));

  error = mo5_kbd_save();

  if (! error) /* save OK */
  {
    psp_display_screen_kbd_menu();
    psp_sdl_back2_print(270, 80, "File saved !", 
                       PSP_MENU_NOTE_COLOR);
    psp_menu_dirty = 1;
    psp_sdl_flip();
    sleep(1);
  }
  else 
  {
    psp_display_screen_kbd_menu();
    psp_sdl_back2_print(270, 80, "Can't save file !", 
                       PSP_MENU_WARNING_COLOR);
    psp_menu_dirty = 1;
    psp_sdl_flip();
    sleep(1);
  }
}

static void
psp_keyboard_select_change(int step)
{
  if (step > 0) {
    if (menu_kbd_selected == KBD_RTRIGGER_MAPPING) menu_kbd_selected = -1;
    else                                           menu_kbd_selected--;
  } else {
    if (menu_kbd_selected == -1) menu_kbd_selected = KBD_RTRIGGER_MAPPING;
    else                         menu_kbd_selected++;
  }
  psp_keyboard_menu_update_lr();
}

static void
psp_keyboard_menu_skin(int step)
{
  if (step > 0) {
    if (psp_kbd_skin < psp_kbd_last_skin) psp_kbd_skin++;
    else                                  psp_kbd_skin = 0;
  } else {
    if (psp_kbd_skin > 0) psp_kbd_skin--;
    else                  psp_kbd_skin = psp_kbd_last_skin;
  }

  danzeff_change_skin();
}

void 
psp_keyboard_menu(void)
{
  SceCtrlData c;
  long new_pad;
  long old_pad;
  int  last_time;
  int  end_menu;
  int  kbd_id;
  int  mo5_key;
  int  danzeff_mode;
  int  danzeff_key;
  int  psp_irkeyb;

  psp_menu_dirty = 1;

  psp_kbd_wait_no_button();

  old_pad      = 0;
  last_time    = 0;
  end_menu     = 0;
  kbd_id       = 0;

  danzeff_key  = 0;
  danzeff_mode = 0;

  memcpy(loc_kbd_mapping  , psp_kbd_mapping  , sizeof(psp_kbd_mapping));
  memcpy(loc_kbd_mapping_L, psp_kbd_mapping_L, sizeof(psp_kbd_mapping_L));
  memcpy(loc_kbd_mapping_R, psp_kbd_mapping_R, sizeof(psp_kbd_mapping_R));

# ifdef USE_PSP_IRKEYB
  psp_irkeyb = PSP_IRKEYB_EMPTY;
# endif

  while (! end_menu)
  {
    psp_display_screen_kbd_menu();

    if (danzeff_mode) {
      danzeff_moveTo(-50, -50);
      danzeff_render();
    }
    psp_sdl_flip();

    while (1) {
      myCtrlPeekBufferPositive(&c, 1);
      c.Buttons &= PSP_ALL_BUTTON_MASK;

# ifdef USE_PSP_IRKEYB
      psp_irkeyb = psp_irkeyb_set_psp_key(&c);
# endif
      new_pad = c.Buttons;

      if ((old_pad != new_pad) || ((c.TimeStamp - last_time) > PSP_MENU_MIN_TIME)) {
        last_time = c.TimeStamp;
        old_pad = new_pad;
        break;
      }
# ifdef USE_PSP_IRKEYB
      if (psp_irkeyb != PSP_IRKEYB_EMPTY) break;
# endif
    }

# ifdef USE_PSP_IRKEYB
    psp_irkeyb = psp_irkeyb_read_key();

    if (psp_irkeyb != PSP_IRKEYB_EMPTY) {
 
      if (psp_irkeyb == 0x8) {
        mo5_key = MO5K_EFF;
      } else
      if (psp_irkeyb == 0x9) {
        mo5_key = MO5K_RAZ;
      } else
      if (psp_irkeyb == 0xd) {
        mo5_key = MO5K_ENT;
      } else
      if (psp_irkeyb == 0x1b) {
        mo5_key = MO5K_STP;
      } else
      if (psp_irkeyb == PSP_IRKEYB_UP) {
        mo5_key = MO5_JOY1_UP;
      } else
      if (psp_irkeyb == PSP_IRKEYB_DOWN) {
        mo5_key = MO5_JOY1_DOWN;
      } else
      if (psp_irkeyb == PSP_IRKEYB_LEFT) {
        mo5_key = MO5_JOY1_LEFT;
      } else
      if (psp_irkeyb == PSP_IRKEYB_RIGHT) {
        mo5_key = MO5_JOY1_RIGHT;
      } else {
        mo5_key = mo5_get_key_from_ascii(psp_irkeyb);
      }

      if (mo5_key != -1) {
        if ((cur_menu_id >= MENU_KBD_UP       ) && 
            (cur_menu_id <= MENU_KBD_JOY_RIGHT)) 
        {
          kbd_id = psp_kbd_menu_id_to_key_id(cur_menu_id);

          if (menu_kbd_selected == -1) loc_kbd_mapping[kbd_id] = mo5_key;
          else
          if (menu_kbd_selected == KBD_LTRIGGER_MAPPING) loc_kbd_mapping_L[kbd_id] = mo5_key;
          else
          if (menu_kbd_selected == KBD_RTRIGGER_MAPPING) loc_kbd_mapping_R[kbd_id] = mo5_key;
        }
      }
    }
# endif

    if (danzeff_mode) {

      danzeff_key = danzeff_readInput(c);

      if (danzeff_key > DANZEFF_START) {
        mo5_key = mo5_get_key_from_ascii(danzeff_key);

        if (mo5_key != -1) {
          if ((cur_menu_id >= MENU_KBD_UP       ) && 
              (cur_menu_id <= MENU_KBD_JOY_RIGHT)) 
          {
            kbd_id = psp_kbd_menu_id_to_key_id(cur_menu_id);

            if (menu_kbd_selected == -1) loc_kbd_mapping[kbd_id] = mo5_key;
            else
            if (menu_kbd_selected == KBD_LTRIGGER_MAPPING) loc_kbd_mapping_L[kbd_id] = mo5_key;
            else
            if (menu_kbd_selected == KBD_RTRIGGER_MAPPING) loc_kbd_mapping_R[kbd_id] = mo5_key;
          }
        }

      } else 
      if ((danzeff_key == DANZEFF_START ) || 
          (danzeff_key == DANZEFF_SELECT)) 
      {
        danzeff_mode = 0;
        old_pad = new_pad = 0;

        psp_kbd_wait_no_button();
      }

      if (danzeff_key >= -1) {
        continue;
      }
    }

    if ((c.Buttons & (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) ==
        (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) {
      /* Exit ! */
      psp_sdl_exit(0);
    } else
    if (new_pad & PSP_CTRL_LTRIGGER) {
      psp_keyboard_select_change(-1);
    } else
    if (new_pad & PSP_CTRL_RTRIGGER) {
      psp_keyboard_select_change(+1);
    } else
    if ((new_pad & PSP_CTRL_CROSS ) || 
        (new_pad & PSP_CTRL_CIRCLE) || 
        (new_pad & PSP_CTRL_RIGHT ) ||
        (new_pad & PSP_CTRL_LEFT  )) 
    {
      int step;

      if (new_pad & PSP_CTRL_LEFT)  step = -1;
      else 
      if (new_pad & PSP_CTRL_RIGHT) step =  1;
      else                          step =  0;

      if ((cur_menu_id >= MENU_KBD_UP       ) && 
          (cur_menu_id <= MENU_KBD_JOY_RIGHT)) 
      {
        kbd_id = psp_kbd_menu_id_to_key_id(cur_menu_id);
        psp_keyboard_menu_mapping(kbd_id, step); 
      }
      else
      {
        switch (cur_menu_id ) 
        {
          case MENU_KBD_SKIN  : psp_keyboard_menu_skin(step);
          break;
          case MENU_KBD_KBD_SELECT  : psp_keyboard_select_change(step);
          break;
          case MENU_KBD_LOAD  : psp_menu_dirty = 1;
                                psp_keyboard_menu_load();
                                psp_menu_dirty = 1;
                                old_pad = new_pad = 0;
                                menu_kbd_selected = -1;
          break;
          case MENU_KBD_SAVE  : psp_keyboard_menu_save();
          break;
          case MENU_KBD_HOTKEYS     : psp_keyboard_menu_hotkeys();
          break;
          case MENU_KBD_RESET : psp_keyboard_menu_reset_kbd();
          break;
  
          case MENU_KBD_BACK  : end_menu = 1;
          break;
        }
      }

    } else
    if(new_pad & PSP_CTRL_UP) {

      if (cur_menu_id > 0) cur_menu_id--;
      else                 cur_menu_id = MAX_MENU_KBD_ITEM-1;

    } else
    if(new_pad & PSP_CTRL_DOWN) {

      if (cur_menu_id < (MAX_MENU_KBD_ITEM-1)) cur_menu_id++;
      else                                     cur_menu_id = 0;

    } else  
    if(new_pad & PSP_CTRL_SQUARE) {
      /* Cancel */
      end_menu = -1;
    } else 
    if ((new_pad & PSP_CTRL_SELECT) == PSP_CTRL_SELECT) {
      /* Back to Main Menu */
      end_menu = 1;
    } else
    if ((new_pad & PSP_CTRL_START) == PSP_CTRL_START) {
      if ((cur_menu_id < MENU_KBD_UP       ) ||
          (cur_menu_id > MENU_KBD_JOY_RIGHT)) {
        cur_menu_id = MENU_KBD_UP;
      }
      danzeff_mode = 1;
    }
  }
 
  if (end_menu > 0) {
    /* Validate */
    psp_keyboard_menu_update_lr();

    memcpy(psp_kbd_mapping  , loc_kbd_mapping  , sizeof(psp_kbd_mapping));
    memcpy(psp_kbd_mapping_L, loc_kbd_mapping_L, sizeof(psp_kbd_mapping_L));
    memcpy(psp_kbd_mapping_R, loc_kbd_mapping_R, sizeof(psp_kbd_mapping_R));
  }

  psp_kbd_wait_no_button();
}

