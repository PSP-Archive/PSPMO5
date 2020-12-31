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

#include <pspctrl.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <SDL.h>

#include "global.h"
#include "psp_kbd.h"
#include "psp_menu.h"
#include "psp_sdl.h"
#include "psp_danzeff.h"
#include "psp_irkeyb.h"
#include "psp_battery.h"

# define KBD_MIN_ANALOG_TIME      150000
# define KBD_MIN_START_TIME       800000
# define KBD_MAX_EVENT_TIME       500000
# define KBD_MIN_PENDING_TIME     300000
# define KBD_MIN_HOTKEY_TIME     1000000
# define KBD_MIN_IR_PENDING_TIME  100000
//# define KBD_MIN_DANZEFF_TIME   150000
# define KBD_MIN_DANZEFF_TIME      10000
# define KBD_MIN_COMMAND_TIME     100000
# define KBD_MIN_BATTCHECK_TIME 90000000 
# define KBD_MIN_AUTOFIRE_TIME   1000000

 static SceCtrlData    loc_button_data;
 static unsigned int   loc_last_event_time = 0;
 static unsigned int   loc_last_hotkey_time = 0;
#ifdef USE_PSP_IRKEYB
 static unsigned int   loc_last_irkbd_event_time = 0;
#endif
 static unsigned int   loc_last_analog_time = 0;
 static long           first_time_stamp = -1;
 static long           first_time_batt_stamp = -1;
 static long           first_time_auto_stamp = -1;
 static char           loc_button_press[ KBD_MAX_BUTTONS ]; 
 static char           loc_button_release[ KBD_MAX_BUTTONS ]; 
 static unsigned int   loc_button_mask[ KBD_MAX_BUTTONS ] =
 {
   PSP_CTRL_UP         , /*  KBD_UP         */
   PSP_CTRL_RIGHT      , /*  KBD_RIGHT      */
   PSP_CTRL_DOWN       , /*  KBD_DOWN       */
   PSP_CTRL_LEFT       , /*  KBD_LEFT       */
   PSP_CTRL_TRIANGLE   , /*  KBD_TRIANGLE   */
   PSP_CTRL_CIRCLE     , /*  KBD_CIRCLE     */
   PSP_CTRL_CROSS      , /*  KBD_CROSS      */
   PSP_CTRL_SQUARE     , /*  KBD_SQUARE     */
   PSP_CTRL_SELECT     , /*  KBD_SELECT     */
   PSP_CTRL_START      , /*  KBD_START      */
   PSP_CTRL_HOME       , /*  KBD_HOME       */
   PSP_CTRL_HOLD       , /*  KBD_HOLD       */
   PSP_CTRL_LTRIGGER   , /*  KBD_LTRIGGER   */
   PSP_CTRL_RTRIGGER   , /*  KBD_RTRIGGER   */
 };

 static char loc_button_name[ KBD_ALL_BUTTONS ][20] =
 {
   "UP",
   "RIGHT",
   "DOWN",
   "LEFT",
   "TRIANGLE",
   "CIRCLE",
   "CROSS",
   "SQUARE",
   "SELECT",
   "START",
   "HOME",
   "HOLD",
   "LTRIGGER",
   "RTRIGGER",
   "JOY_UP",
   "JOY_RIGHT",
   "JOY_DOWN",
   "JOY_LEFT"
 };

 static char loc_button_name_L[ KBD_ALL_BUTTONS ][20] =
 {
   "L_UP",
   "L_RIGHT",
   "L_DOWN",
   "L_LEFT",
   "L_TRIANGLE",
   "L_CIRCLE",
   "L_CROSS",
   "L_SQUARE",
   "L_SELECT",
   "L_START",
   "L_HOME",
   "L_HOLD",
   "L_LTRIGGER",
   "L_RTRIGGER",
   "L_JOY_UP",
   "L_JOY_RIGHT",
   "L_JOY_DOWN",
   "L_JOY_LEFT"
 };
 
  static char loc_button_name_R[ KBD_ALL_BUTTONS ][20] =
 {
   "R_UP",
   "R_RIGHT",
   "R_DOWN",
   "R_LEFT",
   "R_TRIANGLE",
   "R_CIRCLE",
   "R_CROSS",
   "R_SQUARE",
   "R_SELECT",
   "R_START",
   "R_HOME",
   "R_HOLD",
   "R_LTRIGGER",
   "R_RTRIGGER",
   "R_JOY_UP",
   "R_JOY_RIGHT",
   "R_JOY_DOWN",
   "R_JOY_LEFT"
 };
 
  struct mo5_key_trans psp_mo5_key_info[MO5_MAX_KEY]=
  {
    { MO5K_0,             0x1e,   0,    "0" },
    { MO5K_1,             0x2f,   0,    "1" },
    { MO5K_2,             0x27,   0,    "2" },
    { MO5K_3,             0x1f,   0,    "3" },
    { MO5K_4,             0x17,   0,    "4" },
    { MO5K_5,             0x0f,   0,    "5" },
    { MO5K_6,             0x07,   0,    "6" },
    { MO5K_7,             0x06,   0,    "7" },
    { MO5K_8,             0x0e,   0,    "8" },
    { MO5K_9,             0x16,   0,    "9" },
    { MO5K_a,             0x2d,   1,    "a" },
    { MO5K_b,             0x22,   1,    "b" },
    { MO5K_c,             0x32,   1,    "c" },
    { MO5K_d,             0x1b,   1,    "d" },
    { MO5K_e,             0x1d,   1,    "e" },
    { MO5K_f,             0x13,   1,    "f" },
    { MO5K_g,             0x0b,   1,    "g" },
    { MO5K_h,             0x03,   1,    "h" },
    { MO5K_i,             0x0c,   1,    "i" },
    { MO5K_j,             0x02,   1,    "j" },
    { MO5K_k,             0x0a,   1,    "k" },
    { MO5K_l,             0x12,   1,    "l" },
    { MO5K_m,             0x1a,   1,    "m" },
    { MO5K_n,             0x00,   1,    "n" },
    { MO5K_o,             0x14,   1,    "o" },
    { MO5K_p,             0x1c,   1,    "p" },
    { MO5K_q,             0x2b,   1,    "q" },
    { MO5K_r,             0x15,   1,    "r" },
    { MO5K_s,             0x23,   1,    "s" },
    { MO5K_t,             0x0d,   1,    "t" },
    { MO5K_u,             0x04,   1,    "u" },
    { MO5K_v,             0x2a,   1,    "v" },
    { MO5K_w,             0x30,   1,    "w" },
    { MO5K_x,             0x28,   1,    "x" },
    { MO5K_y,             0x05,   1,    "y" },
    { MO5K_z,             0x25,   1,    "z" },
    { MO5K_A,             0x2d,   0,    "A" },
    { MO5K_B,             0x22,   0,    "B" },
    { MO5K_C,             0x32,   0,    "C" },
    { MO5K_D,             0x1b,   0,    "D" },
    { MO5K_E,             0x1d,   0,    "E" },
    { MO5K_F,             0x13,   0,    "F" },
    { MO5K_G,             0x0b,   0,    "G" },
    { MO5K_H,             0x03,   0,    "H" },
    { MO5K_I,             0x0c,   0,    "I" },
    { MO5K_J,             0x02,   0,    "J" },
    { MO5K_K,             0x0a,   0,    "K" },
    { MO5K_L,             0x12,   0,    "L" },
    { MO5K_M,             0x1a,   0,    "M" },
    { MO5K_N,             0x00,   0,    "N" },
    { MO5K_O,             0x14,   0,    "O" },
    { MO5K_P,             0x1c,   0,    "P" },
    { MO5K_Q,             0x2b,   0,    "Q" },
    { MO5K_R,             0x15,   0,    "R" },
    { MO5K_S,             0x23,   0,    "S" },
    { MO5K_T,             0x0d,   0,    "T" },
    { MO5K_U,             0x04,   0,    "U" },
    { MO5K_V,             0x2a,   0,    "V" },
    { MO5K_W,             0x30,   0,    "W" },
    { MO5K_X,             0x28,   0,    "X" },
    { MO5K_Y,             0x05,   0,    "Y" },
    { MO5K_Z,             0x25,   0,    "Z" },
    { MO5K_AMPERSAND,     0x07,   1,    "&" },
    { MO5K_SEMICOLON,     0x2e,   1,    ";" },
    { MO5K_MINUS,         0x26,   0,    "-" },
    { MO5K_EXCLAMATN,     0x2f,   1,    "!" },
    { MO5K_DBLQUOTE,      0x27,   1,    "\"" },
    { MO5K_HASH,          0x1f,   1,    "#" },
    { MO5K_DOLLAR,        0x17,   1,    "$" },
    { MO5K_PERCENT,       0x0f,   1,    "%" },
    { MO5K_QUOTE,         0x06,   1,    "'" },
    { MO5K_LEFTPAREN,     0x0e,   1,    "(" },
    { MO5K_RIGHTPAREN,    0x16,   1,    ")" },
    { MO5K_PLUS,          0x2e,   0,    "+" },
    { MO5K_EQUAL,         0x26,   1,    "=" },
    { MO5K_EFF,           0x01,   0,    "EFF" },
    { MO5K_INS,           0x09,   0,    "INS" },
    { MO5K_RAZ,           0x33,   0,    "RAZ" },
    { MO5K_ENT,           0x34,   0,    "ENTER" },
    { MO5K_CNT,           0x35,   0,    "CNT" },
    { MO5K_ACC,           0x36,   0,    "ACC" },
    { MO5K_STP,           0x37,   0,    "STOP" },
    { MO5K_SHIFT,         0x38,   0,    "SHIFT" },
    { MO5K_BASIC,         0x39,   0,    "BASIC" },
    { MO5K_RETURN,        0x34,   0,    "RETURN" },
    { MO5K_CAPSLOCK,      0x38,   0,    "CAPSLOCK" },
    { MO5K_ESC,           0x37,   0,    "ESC" },
    { MO5K_SPACE,         0x20,   0,    "SPACE" },
    { MO5K_LEFT,          0x29,   0,    "LEFT" },
    { MO5K_UP,            0x31,   0,    "UP" },
    { MO5K_RIGHT,         0x19,   0,    "RIGHT" },
    { MO5K_DOWN,          0x21,   0,    "DOWN" },
    { MO5K_AT,            0x18,   0,    "@" },
    { MO5K_COLON,         0x2c,   1,    ":" },
    { MO5K_COMMA,         0x08,   0,    "," },
    { MO5K_PERIOD,        0x10,   0,    "." },
    { MO5K_SLASH,         0x24,   0,    "/" },
    { MO5K_ASTERISK,      0x2c,   0,    "*" },
    { MO5K_LESS,          0x08,   1,    "<" },
    { MO5K_GREATER,       0x10,   1,    ">" },
    { MO5K_QUESTION,      0x24,   1,    "?" },
    { MO5K_POWER,         0x18,   1,    "^" },
    { MO5K_SUPPR,         0x01,   0,    "SUPPR" },

    { MO5_JOY1_UP,        0x00,   0,    "JOY1_UP" },
    { MO5_JOY1_DOWN,      0x00,   0,    "JOY1_DOWN" },
    { MO5_JOY1_LEFT,      0x00,   0,    "JOY1_LEFT" },
    { MO5_JOY1_RIGHT,     0x00,   0,    "JOY1_RIGHT" },
    { MO5_JOY1_FIRE,      0x00,   0,    "JOY1_FIRE" },
    { MO5_JOY2_UP,        0x00,   0,    "JOY2_UP" },
    { MO5_JOY2_DOWN,      0x00,   0,    "JOY2_DOWN" },
    { MO5_JOY2_LEFT,      0x00,   0,    "JOY2_LEFT" },
    { MO5_JOY2_RIGHT,     0x00,   0,    "JOY2_RIGHT" },
    { MO5_JOY2_FIRE,      0x00,   0,    "JOY2_FIRE" },

    { MO5_C_FPS,          0x00,   0,    "C_FPS" },
    { MO5_C_JOY,          0x00,   0,    "C_JOY" },
    { MO5_C_RENDER,       0x00,   0,    "C_RENDER" },
    { MO5_C_LOAD,         0x00,   0,    "C_LOAD" },
    { MO5_C_SAVE,         0x00,   0,    "C_SAVE" },
    { MO5_C_RESET,        0x00,   0,    "C_RESET" },
    { MO5_C_AUTOFIRE,     0x00,   0,    "C_AUTOFIRE" },
    { MO5_C_INCFIRE,      0x00,   0,    "C_INCFIRE" },
    { MO5_C_DECFIRE,      0x00,   0,    "C_DECFIRE" },
    { MO5_C_INCDELTA,     0x00,   0,    "C_INCDELTA" },
    { MO5_C_DECDELTA,     0x00,   0,    "C_DECDELTA" },
    { MO5_C_SCREEN,       0x00,   0,    "C_SCREEN" }
  };

 static int loc_default_mapping[ KBD_ALL_BUTTONS ] = {
    MO5K_UP              , /*  KBD_UP         */
    MO5K_RIGHT           , /*  KBD_RIGHT      */
    MO5K_DOWN            , /*  KBD_DOWN       */
    MO5K_LEFT            , /*  KBD_LEFT       */
    MO5K_RETURN          , /*  KBD_TRIANGLE   */
    MO5_JOY1_FIRE        , /*  KBD_CIRCLE     */
    MO5K_SPACE           , /*  KBD_CROSS      */
    MO5K_EFF             , /*  KBD_SQUARE     */
    -1                   , /*  KBD_SELECT     */
    -1                   , /*  KBD_START      */
    -1                   , /*  KBD_HOME       */
    -1                   , /*  KBD_HOLD       */
   KBD_LTRIGGER_MAPPING  , /*  KBD_LTRIGGER   */
   KBD_RTRIGGER_MAPPING  , /*  KBD_RTRIGGER   */
    MO5_JOY1_UP          , /*  KBD_JOY_UP     */
    MO5_JOY1_RIGHT       , /*  KBD_JOY_RIGHT  */
    MO5_JOY1_DOWN        , /*  KBD_JOY_DOWN   */
    MO5_JOY1_LEFT          /*  KBD_JOY_LEFT   */
  };

 static int loc_default_mapping_L[ KBD_ALL_BUTTONS ] = {
    MO5_C_INCDELTA       , /*  KBD_UP         */
    MO5_C_RENDER         , /*  KBD_RIGHT      */
    MO5_C_DECDELTA       , /*  KBD_DOWN       */
    MO5_C_RENDER         , /*  KBD_LEFT       */
    MO5_C_LOAD           , /*  KBD_TRIANGLE   */
    MO5_C_JOY            , /*  KBD_CIRCLE     */
    MO5_C_SAVE           , /*  KBD_CROSS      */
    MO5_C_FPS            , /*  KBD_SQUARE     */
    -1                   , /*  KBD_SELECT     */
    -1                   , /*  KBD_START      */
    -1                   , /*  KBD_HOME       */
    -1                   , /*  KBD_HOLD       */
   KBD_LTRIGGER_MAPPING  , /*  KBD_LTRIGGER   */
   KBD_RTRIGGER_MAPPING  , /*  KBD_RTRIGGER   */
    MO5_JOY1_UP          , /*  KBD_JOY_UP     */
    MO5_JOY1_RIGHT       , /*  KBD_JOY_RIGHT  */
    MO5_JOY1_DOWN        , /*  KBD_JOY_DOWN   */
    MO5_JOY1_LEFT          /*  KBD_JOY_LEFT   */
  };

 static int loc_default_mapping_R[ KBD_ALL_BUTTONS ] = {
    MO5K_UP              , /*  KBD_UP         */
    MO5_C_INCFIRE        , /*  KBD_RIGHT      */
    MO5K_DOWN            , /*  KBD_DOWN       */
    MO5_C_DECFIRE        , /*  KBD_LEFT       */
    MO5_C_RESET          , /*  KBD_TRIANGLE   */
    MO5K_EFF             , /*  KBD_CIRCLE     */
    MO5_C_AUTOFIRE       , /*  KBD_CROSS      */
    MO5_JOY1_FIRE        , /*  KBD_SQUARE     */
    -1                   , /*  KBD_SELECT     */
    -1                   , /*  KBD_START      */
    -1                   , /*  KBD_HOME       */
    -1                   , /*  KBD_HOLD       */
   KBD_LTRIGGER_MAPPING  , /*  KBD_LTRIGGER   */
   KBD_RTRIGGER_MAPPING  , /*  KBD_RTRIGGER   */
    MO5_JOY1_UP          , /*  KBD_JOY_UP     */
    MO5_JOY1_RIGHT       , /*  KBD_JOY_RIGHT  */
    MO5_JOY1_DOWN        , /*  KBD_JOY_DOWN   */
    MO5_JOY1_LEFT          /*  KBD_JOY_LEFT   */
  };

 int psp_kbd_mapping[ KBD_ALL_BUTTONS ];
 int psp_kbd_mapping_L[ KBD_ALL_BUTTONS ];
 int psp_kbd_mapping_R[ KBD_ALL_BUTTONS ];
 int psp_kbd_presses[ KBD_ALL_BUTTONS ];
 int kbd_ltrigger_mapping_active;
 int kbd_rtrigger_mapping_active;

# define KBD_MAX_ENTRIES   104


  int kbd_layout[KBD_MAX_ENTRIES][2] = {
    /* Key            Ascii */
    { MO5K_0,             '0' },
    { MO5K_1,             '1' },
    { MO5K_2,             '2' },
    { MO5K_3,             '3' },
    { MO5K_4,             '4' },
    { MO5K_5,             '5' },
    { MO5K_6,             '6' },
    { MO5K_7,             '7' },
    { MO5K_8,             '8' },
    { MO5K_9,             '9' },
    { MO5K_a,             'a' },
    { MO5K_b,             'b' },
    { MO5K_c,             'c' },
    { MO5K_d,             'd' },
    { MO5K_e,             'e' },
    { MO5K_f,             'f' },
    { MO5K_g,             'g' },
    { MO5K_h,             'h' },
    { MO5K_i,             'i' },
    { MO5K_j,             'j' },
    { MO5K_k,             'k' },
    { MO5K_l,             'l' },
    { MO5K_m,             'm' },
    { MO5K_n,             'n' },
    { MO5K_o,             'o' },
    { MO5K_p,             'p' },
    { MO5K_q,             'q' },
    { MO5K_r,             'r' },
    { MO5K_s,             's' },
    { MO5K_t,             't' },
    { MO5K_u,             'u' },
    { MO5K_v,             'v' },
    { MO5K_w,             'w' },
    { MO5K_x,             'x' },
    { MO5K_y,             'y' },
    { MO5K_z,             'z' },
    { MO5K_A,             'A' },
    { MO5K_B,             'B' },
    { MO5K_C,             'C' },
    { MO5K_D,             'D' },
    { MO5K_E,             'E' },
    { MO5K_F,             'F' },
    { MO5K_G,             'G' },
    { MO5K_H,             'H' },
    { MO5K_I,             'I' },
    { MO5K_J,             'J' },
    { MO5K_K,             'K' },
    { MO5K_L,             'L' },
    { MO5K_M,             'M' },
    { MO5K_N,             'N' },
    { MO5K_O,             'O' },
    { MO5K_P,             'P' },
    { MO5K_Q,             'Q' },
    { MO5K_R,             'R' },
    { MO5K_S,             'S' },
    { MO5K_T,             'T' },
    { MO5K_U,             'U' },
    { MO5K_V,             'V' },
    { MO5K_W,             'W' },
    { MO5K_X,             'X' },
    { MO5K_Y,             'Y' },
    { MO5K_Z,             'Z' },
    { MO5K_AMPERSAND,     '&' },
    { MO5K_SEMICOLON,     ';' },
    { MO5K_MINUS,         '-' },
    { MO5K_EXCLAMATN,     '!' },
    { MO5K_DBLQUOTE,      '"' },
    { MO5K_HASH,          '#' },
    { MO5K_DOLLAR,        '$' },
    { MO5K_PERCENT,       '%' },
    { MO5K_QUOTE,         '\'' },
    { MO5K_LEFTPAREN,     '(' },
    { MO5K_RIGHTPAREN,    ')' },
    { MO5K_PLUS,          '+' },
    { MO5K_EQUAL,         '=' },
    { MO5K_EFF,           DANZEFF_DEL },
    { MO5K_INS,           DANZEFF_INSERT },
    { MO5K_RAZ,           DANZEFF_RAZ },
    { MO5K_ENT,           DANZEFF_ENTER },
    { MO5K_CNT,           DANZEFF_CNT  },
    { MO5K_ACC,           DANZEFF_ACC  },
    { MO5K_STP,           DANZEFF_STOP },
    { MO5K_SHIFT,         DANZEFF_SHIFT  },
    { MO5K_BASIC,         DANZEFF_BASIC  },
    { MO5K_RETURN,        DANZEFF_RETURN  },
    { MO5K_CAPSLOCK,      DANZEFF_CAPSLOCK },
    { MO5K_ESC,           DANZEFF_ESC },
    { MO5K_SPACE,         ' ' },
    { MO5K_AT,            '@' },
    { MO5K_COLON,         ':' },
    { MO5K_COMMA,         ',' },
    { MO5K_PERIOD,        '.' },
    { MO5K_SLASH,         '/' },
    { MO5K_ASTERISK,      '*' },
    { MO5K_LESS,          '<' },
    { MO5K_GREATER,       '>' },
    { MO5K_QUESTION,      '?' },
    { MO5K_POWER,         '^' }
  };

        int psp_kbd_mapping[ KBD_ALL_BUTTONS ];

#ifdef USE_PSP_IRKEYB
# define IRKEYB_MO5_MAX_PENDING 20
 static int irkeyb_mo5_pending_max  = 0;
 static int irkeyb_mo5_pending_curr = 0;
 static int irkeyb_mo5_pending_keys[IRKEYB_MO5_MAX_PENDING];
# endif

 static int danzeff_mo5_key     = 0;
 static int danzeff_mo5_pending = 0;
 static int danzeff_mode        = 0;

       char command_keys[ 128 ];
 static int command_mode        = 0;
 static int command_index       = 0;
 static int command_size        = 0;
 static int command_mo5_pending = 0;
 static int command_mo5_key     = 0;

int
mo5_key_event(int mo5_idx, int button_press)
{
  int dcmo5_key = 0;
  int joy_id    = 0;
  int shift     = 0;

  if (MO5.psp_active_joystick) {
    if ((mo5_idx >= MO5_JOY1_UP   ) && 
        (mo5_idx <= MO5_JOY1_FIRE )) {
      /* Convert Joystick Player 1 -> Player 2 ? */
      mo5_idx = mo5_idx - MO5_JOY1_UP + MO5_JOY2_UP;
    }
  }

  if ((mo5_idx >=           0) && 
      (mo5_idx <= MO5K_SUPPR )) {

    shift     = psp_mo5_key_info[mo5_idx].shift;
    dcmo5_key = psp_mo5_key_info[mo5_idx].dcmo5_key;

    if (button_press) {
      if (shift) {
        dcmo5_key_press( psp_mo5_key_info[MO5K_SHIFT].dcmo5_key );
      }
      dcmo5_key_press( dcmo5_key );
    } else {
      dcmo5_key_release( dcmo5_key );
      if (shift) {
        dcmo5_key_release( psp_mo5_key_info[MO5K_SHIFT].dcmo5_key );
      }
    }

  } else
  if ((mo5_idx >= MO5_JOY1_UP) &&
      (mo5_idx <= MO5_JOY2_FIRE)) {

    if (mo5_idx == MO5_JOY1_FIRE) {
      joy_id = 8;
    } else 
    if (mo5_idx == MO5_JOY2_FIRE) {
      joy_id = 9;
    } else
    if (mo5_idx < MO5_JOY1_FIRE) {
      joy_id = mo5_idx - MO5_JOY1_UP;
    } else 
    if (mo5_idx < MO5_JOY2_FIRE) {
      joy_id = mo5_idx - MO5_JOY2_UP;
    }

    if (button_press) dcmo5_joy_press( joy_id );
    else              dcmo5_joy_release( joy_id );

  } else
  if ((mo5_idx >= MO5_C_FPS) &&
      (mo5_idx <= MO5_C_SCREEN)) {

    if (button_press) {
      SceCtrlData c;
      myCtrlPeekBufferPositive(&c, 1);
      if ((c.TimeStamp - loc_last_hotkey_time) > KBD_MIN_HOTKEY_TIME) {
        loc_last_hotkey_time = c.TimeStamp;
        mo5_treat_command_key(mo5_idx);
      }
    }
  }
  return 0;
}

int 
mo5_kbd_reset()
{
  dcmo5_reset_keyboard();
  return 0;
}

int
mo5_get_key_from_ascii(int key_ascii)
{
  int index;
  for (index = 0; index < KBD_MAX_ENTRIES; index++) {
   if (kbd_layout[index][1] == key_ascii) return kbd_layout[index][0];
  }
  return -1;
}

void
psp_kbd_run_command(char *Command)
{
  strncpy(command_keys, Command, 128);
  command_size  = strlen(Command);
  command_index = 0;

  command_mo5_key     = 0;
  command_mo5_pending = 0;
  command_mode        = 1;
}

int
psp_kbd_reset_mapping(void)
{
  memcpy(psp_kbd_mapping, loc_default_mapping, sizeof(loc_default_mapping));
  memcpy(psp_kbd_mapping_L, loc_default_mapping_L, sizeof(loc_default_mapping_L));
  memcpy(psp_kbd_mapping_R, loc_default_mapping_R, sizeof(loc_default_mapping_R));
  return 0;
}

int
psp_kbd_reset_hotkeys(void)
{
  int index;
  int key_id;
  for (index = 0; index < KBD_ALL_BUTTONS; index++) {
    key_id = loc_default_mapping[index];
    if ((key_id >= MO5_C_FPS) && (key_id <= MO5_C_SCREEN)) {
      psp_kbd_mapping[index] = key_id;
    }
    key_id = loc_default_mapping_L[index];
    if ((key_id >= MO5_C_FPS) && (key_id <= MO5_C_SCREEN)) {
      psp_kbd_mapping_L[index] = key_id;
    }
    key_id = loc_default_mapping_R[index];
    if ((key_id >= MO5_C_FPS) && (key_id <= MO5_C_SCREEN)) {
      psp_kbd_mapping_R[index] = key_id;
    }
  }
  return 0;
}

int
psp_kbd_load_mapping_file(FILE *KbdFile)
{
  char     Buffer[512];
  char    *Scan;
  int      tmp_mapping[KBD_ALL_BUTTONS];
  int      tmp_mapping_L[KBD_ALL_BUTTONS];
  int      tmp_mapping_R[KBD_ALL_BUTTONS];
  int      mo5_key_id = 0;
  int      kbd_id = 0;

  memcpy(tmp_mapping, loc_default_mapping, sizeof(loc_default_mapping));
  memcpy(tmp_mapping_L, loc_default_mapping_L, sizeof(loc_default_mapping_R));
  memcpy(tmp_mapping_R, loc_default_mapping_R, sizeof(loc_default_mapping_R));

  while (fgets(Buffer,512,KbdFile) != (char *)0) {
      
      Scan = strchr(Buffer,'\n');
      if (Scan) *Scan = '\0';
      /* For this #@$% of windows ! */
      Scan = strchr(Buffer,'\r');
      if (Scan) *Scan = '\0';
      if (Buffer[0] == '#') continue;

      Scan = strchr(Buffer,'=');
      if (! Scan) continue;
    
      *Scan = '\0';
      mo5_key_id = atoi(Scan + 1);

      for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++) {
        if (!strcasecmp(Buffer,loc_button_name[kbd_id])) {
          tmp_mapping[kbd_id] = mo5_key_id;
          //break;
        }
      }
      for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++) {
        if (!strcasecmp(Buffer,loc_button_name_L[kbd_id])) {
          tmp_mapping_L[kbd_id] = mo5_key_id;
          //break;
        }
      }
      for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++) {
        if (!strcasecmp(Buffer,loc_button_name_R[kbd_id])) {
          tmp_mapping_R[kbd_id] = mo5_key_id;
          //break;
        }
      }
  }

  memcpy(psp_kbd_mapping, tmp_mapping, sizeof(psp_kbd_mapping));
  memcpy(psp_kbd_mapping_L, tmp_mapping_L, sizeof(psp_kbd_mapping_L));
  memcpy(psp_kbd_mapping_R, tmp_mapping_R, sizeof(psp_kbd_mapping_R));
  
  return 0;
}

int
psp_kbd_load_mapping(char *kbd_filename)
{
  FILE    *KbdFile;
  int      error = 0;

  KbdFile = fopen(kbd_filename, "r");
  error   = 1;

  if (KbdFile != (FILE*)0) {
  psp_kbd_load_mapping_file(KbdFile);
  error = 0;
    fclose(KbdFile);
  }

  kbd_ltrigger_mapping_active = 0;
  kbd_rtrigger_mapping_active = 0;
    
  return error;
}

int
psp_kbd_save_mapping(char *kbd_filename)
{
  FILE    *KbdFile;
  int      kbd_id = 0;
  int      error = 0;

  KbdFile = fopen(kbd_filename, "w");
  error   = 1;

  if (KbdFile != (FILE*)0) {

    for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++)
    {
      fprintf(KbdFile, "%s=%d\n", loc_button_name[kbd_id], psp_kbd_mapping[kbd_id]);
    }
    for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++)
    {
      fprintf(KbdFile, "%s=%d\n", loc_button_name_L[kbd_id], psp_kbd_mapping_L[kbd_id]);
    }
    for (kbd_id = 0; kbd_id < KBD_ALL_BUTTONS; kbd_id++)
    {
      fprintf(KbdFile, "%s=%d\n", loc_button_name_R[kbd_id], psp_kbd_mapping_R[kbd_id]);
    }
    error = 0;
    fclose(KbdFile);
  }

  return error;
}

int
psp_kbd_enter_command()
{
  SceCtrlData  c;

  unsigned int command_key = 0;
  int          mo5_key     = 0;
  int          key_event   = 0;

  myCtrlPeekBufferPositive(&c, 1);

  if (command_mo5_pending) 
  {
    if ((c.TimeStamp - loc_last_event_time) > KBD_MIN_COMMAND_TIME) {
      loc_last_event_time = c.TimeStamp;
      command_mo5_pending = 0;
      mo5_key_event(command_mo5_key, 0);
    }

    return 0;
  }

  if ((c.TimeStamp - loc_last_event_time) > KBD_MIN_COMMAND_TIME) {
    loc_last_event_time = c.TimeStamp;

    if (command_index >= command_size) {

      command_mode  = 0;
      command_index = 0;
      command_size  = 0;

      command_mo5_pending = 0;
      command_mo5_key     = 0;

      return 0;
    }
  
    command_key = command_keys[command_index++];
    mo5_key = mo5_get_key_from_ascii(command_key);

    if (mo5_key != -1) {
      command_mo5_key     = mo5_key;
      command_mo5_pending = 1;
      mo5_key_event(command_mo5_key, 1);
    }

    return 1;
  }

  return 0;
}

int 
psp_kbd_is_danzeff_mode()
{
  return danzeff_mode;
}

int
psp_kbd_enter_danzeff()
{
  unsigned int danzeff_key = 0;
  int          mo5_key   = 0;
  SceCtrlData  c;

  if (! danzeff_mode) {
    psp_init_keyboard();
    danzeff_mode = 1;
  }

  myCtrlPeekBufferPositive(&c, 1);
  c.Buttons &= PSP_ALL_BUTTON_MASK;

  if (danzeff_mo5_pending) 
  {
    mo5_key_event(danzeff_mo5_key, 1);

    if ((c.TimeStamp - loc_last_event_time) > KBD_MIN_PENDING_TIME) {
      loc_last_event_time = c.TimeStamp;
      danzeff_mo5_pending = 0;
      mo5_key_event(danzeff_mo5_key, 0);
    }
    return 0;
  }

  if ((c.TimeStamp - loc_last_event_time) > KBD_MIN_DANZEFF_TIME) {
    loc_last_event_time = c.TimeStamp;
  
    myCtrlPeekBufferPositive(&c, 1);
    c.Buttons &= PSP_ALL_BUTTON_MASK;
# ifdef USE_PSP_IRKEYB
    psp_irkeyb_set_psp_key(&c);
# endif
    danzeff_key = danzeff_readInput(c);
  }

  if (danzeff_key == DANZEFF_LEFT) {
    danzeff_key = DANZEFF_DEL;
  } else if (danzeff_key == DANZEFF_DOWN) {
    danzeff_key = DANZEFF_ENTER;
  } else if (danzeff_key == DANZEFF_RIGHT) {
  } else if (danzeff_key == DANZEFF_UP) {
  }

  if (danzeff_key > DANZEFF_START) {
    mo5_key = mo5_get_key_from_ascii(danzeff_key);

    if (mo5_key != -1) {
      danzeff_mo5_key     = mo5_key;
      danzeff_mo5_pending = 1;
      mo5_key_event(danzeff_mo5_key, 1);
    }

    return 1;

  } else if (danzeff_key == DANZEFF_START) {
    danzeff_mode          = 0;
    danzeff_mo5_pending = 0;
    danzeff_mo5_key     = 0;

    psp_kbd_wait_no_button();

  } else if (danzeff_key == DANZEFF_SELECT) {
    danzeff_mode          = 0;
    danzeff_mo5_pending = 0;
    danzeff_mo5_key     = 0;
    psp_main_menu();
    psp_init_keyboard();

    psp_kbd_wait_no_button();
  }

  return 0;
}

#ifdef USE_PSP_IRKEYB
int
psp_kbd_enter_irkeyb()
{
  int mo5_key   = 0;
  int psp_irkeyb = PSP_IRKEYB_EMPTY;

  SceCtrlData  c;
  myCtrlPeekBufferPositive(&c, 1);

  if (irkeyb_mo5_pending_max) 
  {
    if ((c.TimeStamp - loc_last_irkbd_event_time) > KBD_MIN_IR_PENDING_TIME) {
      loc_last_irkbd_event_time = c.TimeStamp;
      mo5_key_event(irkeyb_mo5_pending_keys[irkeyb_mo5_pending_curr], 0);
      irkeyb_mo5_pending_curr++;
      if (irkeyb_mo5_pending_curr >= irkeyb_mo5_pending_max) {
        irkeyb_mo5_pending_curr = 0;
        irkeyb_mo5_pending_max  = 0;
      }
    }

    return 0;
  }

  loc_last_irkbd_event_time = c.TimeStamp;
  
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
      if ((irkeyb_mo5_pending_max+1) < IRKEYB_MO5_MAX_PENDING) {
        irkeyb_mo5_pending_keys[irkeyb_mo5_pending_max++] = mo5_key;
        mo5_key_event(mo5_key, 1);
      }
      return 1;
    }
  }
  return 0;
}
# endif

void
psp_kbd_display_active_mapping()
{
  if (kbd_ltrigger_mapping_active) {
    psp_sdl_fill_rectangle(0, 0, 10, 3, psp_sdl_rgb(0x0, 0x0, 0xff), 0);
  } else {
    psp_sdl_fill_rectangle(0, 0, 10, 3, 0x0, 0);
  }

  if (kbd_rtrigger_mapping_active) {
    psp_sdl_fill_rectangle(470, 0, 10, 3, psp_sdl_rgb(0x0, 0x0, 0xff), 0);
  } else {
    psp_sdl_fill_rectangle(470, 0, 10, 3, 0x0, 0);
  }
}

int
mo5_decode_key(int psp_b, int button_pressed)
{
  int wake = 0;
  int reverse_analog = MO5.psp_reverse_analog;

  if (reverse_analog) {
    if ((psp_b >= KBD_JOY_UP  ) &&
        (psp_b <= KBD_JOY_LEFT)) {
      psp_b = psp_b - KBD_JOY_UP + KBD_UP;
    } else
    if ((psp_b >= KBD_UP  ) &&
        (psp_b <= KBD_LEFT)) {
      psp_b = psp_b - KBD_UP + KBD_JOY_UP;
    }
  }

  if (psp_b == KBD_START) {
     if (button_pressed) psp_kbd_enter_danzeff();
  } else
  if (psp_b == KBD_SELECT) {
    if (button_pressed) {
      psp_main_menu();
      psp_init_keyboard();
    }
  } else {
 
    if (psp_kbd_mapping[psp_b] >= 0) {
      wake = 1;
      if (button_pressed) {
        // Determine which buton to press first (ie which mapping is currently active)
        if (kbd_ltrigger_mapping_active) {
          // Use ltrigger mapping
          psp_kbd_presses[psp_b] = psp_kbd_mapping_L[psp_b];
          mo5_key_event(psp_kbd_presses[psp_b], button_pressed);
        } else
        if (kbd_rtrigger_mapping_active) {
          // Use rtrigger mapping
          psp_kbd_presses[psp_b] = psp_kbd_mapping_R[psp_b];
          mo5_key_event(psp_kbd_presses[psp_b], button_pressed);
        } else {
          // Use standard mapping
          psp_kbd_presses[psp_b] = psp_kbd_mapping[psp_b];
          mo5_key_event(psp_kbd_presses[psp_b], button_pressed);
        }
      } else {
          // Determine which button to release (ie what was pressed before)
          mo5_key_event(psp_kbd_presses[psp_b], button_pressed);
      }

    } else {
      if (psp_kbd_mapping[psp_b] == KBD_LTRIGGER_MAPPING) {
        kbd_ltrigger_mapping_active = button_pressed;
        kbd_rtrigger_mapping_active = 0;
      } else
      if (psp_kbd_mapping[psp_b] == KBD_RTRIGGER_MAPPING) {
        kbd_rtrigger_mapping_active = button_pressed;
        kbd_ltrigger_mapping_active = 0;
      }
    }
  }
  return 0;
}

# define ANALOG_THRESHOLD 60

void 
kbd_get_analog_direction(int Analog_x, int Analog_y, int *x, int *y)
{
  int DeltaX = 255;
  int DeltaY = 255;
  int DirX   = 0;
  int DirY   = 0;

  *x = 0;
  *y = 0;

  if (Analog_x <=        ANALOG_THRESHOLD)  { DeltaX = Analog_x; DirX = -1; }
  else 
  if (Analog_x >= (255 - ANALOG_THRESHOLD)) { DeltaX = 255 - Analog_x; DirX = 1; }

  if (Analog_y <=        ANALOG_THRESHOLD)  { DeltaY = Analog_y; DirY = -1; }
  else 
  if (Analog_y >= (255 - ANALOG_THRESHOLD)) { DeltaY = 255 - Analog_y; DirY = 1; }

  *x = DirX;
  *y = DirY;
}

void
kbd_change_auto_fire(int auto_fire)
{
  MO5.mo5_auto_fire = auto_fire;
  if (MO5.mo5_auto_fire_pressed) {
    if (MO5.psp_active_joystick) {
      mo5_key_event(MO5_JOY2_FIRE, 0);
    } else {
      mo5_key_event(MO5_JOY1_FIRE, 0);
    }
    MO5.mo5_auto_fire_pressed = 0;
  }
}


static int 
kbd_reset_button_status(void)
{
  int b = 0;
  /* Reset Button status */
  for (b = 0; b < KBD_MAX_BUTTONS; b++) {
    loc_button_press[b]   = 0;
    loc_button_release[b] = 0;
  }
  psp_init_keyboard();
  return 0;
}

int
kbd_scan_keyboard(void)
{
  SceCtrlData c;
  long        delta_stamp;
  int         event;
  int         b;

  int new_Lx;
  int new_Ly;
  int old_Lx;
  int old_Ly;

  event = 0;
  myCtrlPeekBufferPositive( &c, 1 );
  c.Buttons &= PSP_ALL_BUTTON_MASK;

# ifdef USE_PSP_IRKEYB
  psp_irkeyb_set_psp_key(&c);
# endif

  if ((c.Buttons & (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) ==
      (PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER|PSP_CTRL_START)) {
    /* Exit ! */
    psp_sdl_exit(0);
  }

  delta_stamp = c.TimeStamp - first_time_stamp;
  if ((delta_stamp < 0) || (delta_stamp > KBD_MIN_BATTCHECK_TIME)) {
    first_time_stamp = c.TimeStamp;
    if (psp_is_low_battery()) {
      psp_main_menu();
      psp_init_keyboard();
      return 0;
    }
  }

  if (MO5.mo5_auto_fire) {
    delta_stamp = c.TimeStamp - first_time_auto_stamp;
    if ((delta_stamp < 0) || 
        (delta_stamp > (KBD_MIN_AUTOFIRE_TIME / (1 + MO5.mo5_auto_fire_period)))) {
      first_time_auto_stamp = c.TimeStamp;
      if (MO5.psp_active_joystick) {
        mo5_key_event(MO5_JOY2_FIRE, MO5.mo5_auto_fire_pressed);
      } else {
        mo5_key_event(MO5_JOY1_FIRE, MO5.mo5_auto_fire_pressed);
      }
      MO5.mo5_auto_fire_pressed = ! MO5.mo5_auto_fire_pressed;
    }
  }

  /* Check Analog Device */
  kbd_get_analog_direction(loc_button_data.Lx,loc_button_data.Ly,&old_Lx,&old_Ly);
  kbd_get_analog_direction( c.Lx, c.Ly, &new_Lx, &new_Ly);

  /* Analog device has moved */
  if (new_Lx > 0) {
    if (old_Lx <  0) mo5_decode_key(KBD_JOY_LEFT , 0);
    if (old_Lx <= 0) mo5_decode_key(KBD_JOY_RIGHT, 1);
  } else 
  if (new_Lx < 0) {
    if (old_Lx >  0) mo5_decode_key(KBD_JOY_RIGHT, 0);
    if (old_Lx >= 0) mo5_decode_key(KBD_JOY_LEFT , 1);
  } else {
    if (old_Lx >  0) mo5_decode_key(KBD_JOY_RIGHT , 0);
    else
    if (old_Lx <  0) mo5_decode_key(KBD_JOY_LEFT, 0);
  }

  if (new_Ly > 0) {
    if (old_Ly <  0) mo5_decode_key(KBD_JOY_UP , 0);
    if (old_Ly <= 0) mo5_decode_key(KBD_JOY_DOWN, 1);
  } else 
  if (new_Ly < 0) {
    if (old_Ly >  0) mo5_decode_key(KBD_JOY_DOWN, 0);
    if (old_Ly >= 0) mo5_decode_key(KBD_JOY_UP , 1);
  } else {
    if (old_Ly >  0) mo5_decode_key(KBD_JOY_DOWN , 0);
    else
    if (old_Ly <  0) mo5_decode_key(KBD_JOY_UP, 0);
  }

  for (b = 0; b < KBD_MAX_BUTTONS; b++) 
  {
    if (c.Buttons & loc_button_mask[b]) {
# if 0  //GAME MODE !
      if (!(loc_button_data.Buttons & loc_button_mask[b])) 
# endif
      {
        loc_button_press[b] = 1;
        event = 1;
      }
    } else {
      if (loc_button_data.Buttons & loc_button_mask[b]) {
        loc_button_release[b] = 1;
        loc_button_press[b] = 0;
        event = 1;
      }
    }
  }
  memcpy(&loc_button_data,&c,sizeof(SceCtrlData));

  return event;
}

void
kbd_wait_start(void)
{
  while (1)
  {
    SceCtrlData c;
    myCtrlPeekBufferPositive(&c, 1);
    c.Buttons &= PSP_ALL_BUTTON_MASK;
    if (c.Buttons & PSP_CTRL_START) break;
  }
  psp_kbd_wait_no_button();
}

void
psp_init_keyboard(void)
{
  mo5_kbd_reset();
  kbd_ltrigger_mapping_active = 0;
  kbd_rtrigger_mapping_active = 0;
}

void
psp_kbd_wait_no_button(void)
{
  SceCtrlData c;

  do {
   myCtrlPeekBufferPositive(&c, 1);
   c.Buttons &= PSP_ALL_BUTTON_MASK;

  } while (c.Buttons != 0);
} 

void
psp_kbd_wait_button(void)
{
  SceCtrlData c;

  do {
   myCtrlPeekBufferPositive(&c, 1);
  } while (c.Buttons == 0);
} 

int
psp_update_keys(void)
{
  int         b;

  static char first_time = 1;
  static int release_pending = 0;

  if (first_time) {

    memcpy(psp_kbd_mapping, loc_default_mapping, sizeof(loc_default_mapping));
    memcpy(psp_kbd_mapping_L, loc_default_mapping_L, sizeof(loc_default_mapping_L));
    memcpy(psp_kbd_mapping_R, loc_default_mapping_R, sizeof(loc_default_mapping_R));

    mo5_kbd_load();

    SceCtrlData c;
    myCtrlPeekBufferPositive(&c, 1);
    c.Buttons &= PSP_ALL_BUTTON_MASK;

    if (first_time_stamp == -1) first_time_stamp = c.TimeStamp;
    if ((! c.Buttons) && ((c.TimeStamp - first_time_stamp) < KBD_MIN_START_TIME)) return 0;

    first_time      = 0;
    release_pending = 0;

    for (b = 0; b < KBD_MAX_BUTTONS; b++) {
      loc_button_release[b] = 0;
      loc_button_press[b] = 0;
    }
    myCtrlPeekBufferPositive(&loc_button_data, 1);
    loc_button_data.Buttons &= PSP_ALL_BUTTON_MASK;

    psp_main_menu();
    psp_init_keyboard();

    return 0;
  }

  mo5_apply_cheats();

  if (command_mode) {
    return psp_kbd_enter_command();
  }

  if (danzeff_mode) {
    psp_kbd_enter_danzeff();
    return 0;
  }

# ifdef USE_PSP_IRKEYB
  if (psp_kbd_enter_irkeyb()) {
    return 0;
  }
# endif

  if (release_pending)
  {
    release_pending = 0;
    for (b = 0; b < KBD_MAX_BUTTONS; b++) {
      if (loc_button_release[b]) {
        loc_button_release[b] = 0;
        loc_button_press[b] = 0;
        mo5_decode_key(b, 0);
      }
    }
  }

  kbd_scan_keyboard();

  /* check press event */
  for (b = 0; b < KBD_MAX_BUTTONS; b++) {
    if (loc_button_press[b]) {
      loc_button_press[b] = 0;
      release_pending     = 0;
      mo5_decode_key(b, 1);
    }
  }
  /* check release event */
  for (b = 0; b < KBD_MAX_BUTTONS; b++) {
    if (loc_button_release[b]) {
      release_pending = 1;
      break;
    }
  }

  return 0;
}
