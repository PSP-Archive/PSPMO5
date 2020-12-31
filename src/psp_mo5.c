/*
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
#include <zlib.h>
#include "SDL.h"

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspsdk.h>
#include <pspctrl.h>
#include <pspthreadman.h>
#include <stdlib.h>
#include <stdio.h>

#include <zlib.h>
#include <psppower.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "global.h"
#include "psp_mo5.h"


void
psp_mo5_hard_reset()
{
  Initprog();
  Ejectmemo();
  Ejectk7();
  Ejectfd();

  if (MO5.mo5_disk_mode) {
    mo5_load_dos();
  }
}

void
psp_mo5_soft_reset()
{
  Initprog();
}

void
psp_mo5_eject()
{
  Ejectmemo();

  if (MO5.mo5_disk_mode) {
    mo5_load_dos();
  }
}

int
psp_mo5_rewind_k7()
{
  Rewindk7();
  return 0;
}

int
psp_mo5_get_k7_index()
{
  return GetK7Index();
}

int 
psp_mo5_load_rom(char *filename) 
{
  return Loadmemo(filename);
}

int 
psp_mo5_load_rom_buffer(char *rom_buffer, int rom_size) 
{
  return Loadmemo_buffer(rom_buffer, rom_size);
}

/* LUDO: */
void
mo5_synchronize(void)
{
  static u32 nextclock = 1;

  if (nextclock) {
    u32 curclock;
    do {
     curclock = SDL_GetTicks();
    } while (curclock < nextclock);
    nextclock = curclock + (u32)( 1000 / MO5.mo5_speed_limiter);
  }
}

void
mo5_update_fps()
{
  static u32 next_sec_clock = 0;
  static u32 cur_num_frame = 0;
  cur_num_frame++;
  u32 curclock = SDL_GetTicks();
  if (curclock > next_sec_clock) {
    next_sec_clock = curclock + 1000;
    MO5.mo5_current_fps = cur_num_frame;
    cur_num_frame = 0;
  }
}

void
psp_mo5_wait_vsync()
{
#ifndef LINUX_MODE
  static int loc_pv = 0;
  int cv = sceDisplayGetVcount();
  if (loc_pv == cv) {
    sceDisplayWaitVblankCB();
  }
  loc_pv = sceDisplayGetVcount();
#endif
}

# define MO5_VWIDTH  320
# define MO5_VHEIGHT 200

static inline void
loc_split_src_rect( SDL_Rect* r )
{
  if (r->x < 0) r->x = 0;
  if (r->y < 0) r->y = 0;
  if ((r->w + r->x) > MO5_WIDTH)  r->x = MO5_WIDTH  - r->w;
  if ((r->h + r->y) > MO5_HEIGHT) r->y = MO5_HEIGHT - r->h;
}

static void
Display_gu_normal(void)
{
  SDL_Rect srcRect;
  SDL_Rect dstRect;

  srcRect.w = MO5_VWIDTH;
  srcRect.h = MO5_VHEIGHT;
  srcRect.x = 8;
  srcRect.y = 8 + MO5.mo5_delta_y;
  loc_split_src_rect( &srcRect );
  
  dstRect.w = MO5_VWIDTH;
  dstRect.h = MO5_VHEIGHT;
  dstRect.x = (480 - dstRect.w) / 2;
  dstRect.y = (272 - dstRect.h) / 2;

  psp_sdl_gu_stretch(&srcRect, &dstRect);
}

static void
Display_gu_fit_width(void)
{
  SDL_Rect srcRect;
  SDL_Rect dstRect;

  srcRect.w = MO5_VWIDTH;
  srcRect.h = MO5_VHEIGHT;
  srcRect.x = 8;
  srcRect.y = 8 + MO5.mo5_delta_y;
  loc_split_src_rect( &srcRect );

  dstRect.x = 0;
  dstRect.y = 16;
  dstRect.w = 480;
  dstRect.h = 240;

  psp_sdl_gu_stretch(&srcRect, &dstRect);
}

static void
Display_gu_fit_height(void)
{
  SDL_Rect srcRect;
  SDL_Rect dstRect;

  srcRect.w = MO5_VWIDTH;
  srcRect.h = MO5_VHEIGHT;
  srcRect.x = 8;
  srcRect.y = 8 + MO5.mo5_delta_y;
  loc_split_src_rect( &srcRect );

  dstRect.x = 22;
  dstRect.y = 0;
  dstRect.w = 436;
  dstRect.h = 272;

  psp_sdl_gu_stretch(&srcRect, &dstRect);
}

static void
Display_gu_fit(void)
{
  SDL_Rect srcRect;
  SDL_Rect dstRect;

  srcRect.w = MO5_VWIDTH + 16;
  srcRect.h = MO5_VHEIGHT + 16;
  srcRect.x = 0;
  srcRect.y = 0;

  dstRect.x = 0;
  dstRect.y = 0;
  dstRect.w = 480;
  dstRect.h = 272;

  psp_sdl_gu_stretch(&srcRect, &dstRect);
}

static void
Display_gu_max(void)
{
  SDL_Rect srcRect;
  SDL_Rect dstRect;

  srcRect.x = 8;
  srcRect.y = 8;
  srcRect.w = MO5_VWIDTH;
  srcRect.h = MO5_VHEIGHT;
  dstRect.x = 0;
  dstRect.y = 0;
  dstRect.w = 480;
  dstRect.h = 272;

  psp_sdl_gu_stretch(&srcRect, &dstRect);
}

void
psp_mo5_display()
{
  if (MO5.psp_skip_cur_frame <= 0) {

    MO5.psp_skip_cur_frame = MO5.psp_skip_max_frame;

    if (MO5.mo5_render_mode == MO5_RENDER_NORMAL) Display_gu_normal();
    else
    if (MO5.mo5_render_mode == MO5_RENDER_FIT_H ) Display_gu_fit_height();
    else
    if (MO5.mo5_render_mode == MO5_RENDER_X125  ) Display_gu_fit_width();
    else
    if (MO5.mo5_render_mode == MO5_RENDER_FIT   ) Display_gu_fit();
    else
    if (MO5.mo5_render_mode == MO5_RENDER_MAX   ) Display_gu_max();

    if (psp_kbd_is_danzeff_mode()) {
      sceDisplayWaitVblankStart();
      danzeff_moveTo(-165, -50);
      danzeff_render();
    }

    if (MO5.mo5_view_fps) {
      char buffer[32];
      sprintf(buffer, "%3d", (int)MO5.mo5_current_fps);
      psp_sdl_fill_print(0, 0, buffer, 0xffffff, 0 );
    }

    if (MO5.psp_display_lr) {
      psp_kbd_display_active_mapping();
    }
    if (MO5.mo5_vsync) {
      psp_mo5_wait_vsync();
    }
    psp_sdl_flip();
  
    if (psp_screenshot_mode) {
      psp_screenshot_mode--;
      if (psp_screenshot_mode <= 0) {
        psp_sdl_save_screenshot();
        psp_screenshot_mode = 0;
      }
    }

  } else if (MO5.psp_skip_max_frame) {
    MO5.psp_skip_cur_frame--;
  }

  if (MO5.mo5_speed_limiter) {
	  mo5_synchronize();
  }

  if (MO5.mo5_view_fps) {
    mo5_update_fps();
  }
  
}

void
psp_mo5_initialize()
{
  InitScreen();
  Init6809registerpointers();
  Hardreset();
  psp_mo5_hard_reset();
}

# define FREQ_6809 1000000  // 1 MHz 

void
psp_mo5_main_loop()
{
  Run();
}
