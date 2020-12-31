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

#ifndef _PSP_MO5_H_
#define _PSP_MO5_H_

#ifdef __cplusplus
extern "C" {
#endif

  extern void psp_mo5_reset();
  extern int psp_mo5_load_state(char *filename);
  extern int psp_mo5_save_state(char *filename);
  extern int psp_mo5_load_rom(char *filename);
  extern void mo5_synchronize(void);
  extern void mo5_update_fps();
  extern void psp_mo5_main_loop();

#ifdef __cplusplus
}
#endif

#endif
