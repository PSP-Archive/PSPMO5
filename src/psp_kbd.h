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

# ifndef _KBD_H_
# define _KBD_H_

#ifdef __cplusplus
extern "C" {
#endif

# define PSP_ALL_BUTTON_MASK 0xFFFF

 enum mo5_keys_emum {

   MO5K_0,          
   MO5K_1,          
   MO5K_2,          
   MO5K_3,          
   MO5K_4,          
   MO5K_5,          
   MO5K_6,          
   MO5K_7,          
   MO5K_8,          
   MO5K_9,          
   MO5K_a,          
   MO5K_b,          
   MO5K_c,          
   MO5K_d,          
   MO5K_e,          
   MO5K_f,          
   MO5K_g,          
   MO5K_h,          
   MO5K_i,          
   MO5K_j,          
   MO5K_k,          
   MO5K_l,          
   MO5K_m,          
   MO5K_n,          
   MO5K_o,          
   MO5K_p,          
   MO5K_q,          
   MO5K_r,          
   MO5K_s,          
   MO5K_t,          
   MO5K_u,          
   MO5K_v,          
   MO5K_w,          
   MO5K_x,          
   MO5K_y,          
   MO5K_z,          
   MO5K_A,          
   MO5K_B,          
   MO5K_C,          
   MO5K_D,          
   MO5K_E,          
   MO5K_F,          
   MO5K_G,          
   MO5K_H,          
   MO5K_I,          
   MO5K_J,          
   MO5K_K,          
   MO5K_L,          
   MO5K_M,          
   MO5K_N,          
   MO5K_O,          
   MO5K_P,          
   MO5K_Q,          
   MO5K_R,          
   MO5K_S,          
   MO5K_T,          
   MO5K_U,          
   MO5K_V,          
   MO5K_W,          
   MO5K_X,          
   MO5K_Y,          
   MO5K_Z,          

   MO5K_AMPERSAND,  
   MO5K_SEMICOLON,  
   MO5K_MINUS,  
   MO5K_EXCLAMATN,  
   MO5K_DBLQUOTE,   
   MO5K_HASH,       
   MO5K_DOLLAR,     
   MO5K_PERCENT,    
   MO5K_QUOTE,      
   MO5K_LEFTPAREN,  
   MO5K_RIGHTPAREN, 
   MO5K_PLUS,       
   MO5K_EQUAL,      

   MO5K_EFF,     
   MO5K_INS,     
   MO5K_RAZ,     
   MO5K_ENT,     
   MO5K_CNT,     
   MO5K_ACC,     
   MO5K_STP,     
   MO5K_SHIFT,      
   MO5K_BASIC,      

   MO5K_RETURN,     
   MO5K_CAPSLOCK,   
   MO5K_ESC,        
   MO5K_SPACE,      
   MO5K_LEFT,       
   MO5K_UP,         
   MO5K_RIGHT,      
   MO5K_DOWN,       
   MO5K_AT,         
   MO5K_COLON,      
   MO5K_COMMA,      
   MO5K_PERIOD,     
   MO5K_SLASH,      
   MO5K_ASTERISK,   
   MO5K_LESS,       
   MO5K_GREATER,    
   MO5K_QUESTION,   
   MO5K_POWER,      
   MO5K_SUPPR,      

   MO5_JOY1_UP,     
   MO5_JOY1_DOWN,   
   MO5_JOY1_LEFT,   
   MO5_JOY1_RIGHT,  
   MO5_JOY1_FIRE,  

   MO5_JOY2_UP,     
   MO5_JOY2_DOWN,   
   MO5_JOY2_LEFT,   
   MO5_JOY2_RIGHT,  
   MO5_JOY2_FIRE,  

   MO5_C_FPS,
   MO5_C_JOY,
   MO5_C_RENDER,
   MO5_C_LOAD,
   MO5_C_SAVE,
   MO5_C_RESET,
   MO5_C_AUTOFIRE,
   MO5_C_INCFIRE,
   MO5_C_DECFIRE,
   MO5_C_INCDELTA,
   MO5_C_DECDELTA,
   MO5_C_SCREEN,
    
   MO5_MAX_KEY      
 };

# define KBD_UP           0
# define KBD_RIGHT        1
# define KBD_DOWN         2
# define KBD_LEFT         3
# define KBD_TRIANGLE     4
# define KBD_CIRCLE       5
# define KBD_CROSS        6
# define KBD_SQUARE       7
# define KBD_SELECT       8
# define KBD_START        9
# define KBD_HOME        10
# define KBD_HOLD        11
# define KBD_LTRIGGER    12
# define KBD_RTRIGGER    13

# define KBD_MAX_BUTTONS 14

# define KBD_JOY_UP      14
# define KBD_JOY_RIGHT   15
# define KBD_JOY_DOWN    16
# define KBD_JOY_LEFT    17

# define KBD_ALL_BUTTONS 18

# define KBD_UNASSIGNED         -1

# define KBD_LTRIGGER_MAPPING   -2
# define KBD_RTRIGGER_MAPPING   -3
# define KBD_NORMAL_MAPPING     -1

 struct mo5_key_trans {
   int  key;
   int  dcmo5_key;
   int  shift;
   char name[10];
 };
  

  extern int psp_screenshot_mode;
  extern int psp_kbd_mapping[ KBD_ALL_BUTTONS ];
  extern int psp_kbd_mapping_L[ KBD_ALL_BUTTONS ];
  extern int psp_kbd_mapping_R[ KBD_ALL_BUTTONS ];
  extern int psp_kbd_presses[ KBD_ALL_BUTTONS ];
  extern int kbd_ltrigger_mapping_active;
  extern int kbd_rtrigger_mapping_active;

  extern struct mo5_key_trans psp_mo5_key_info[MO5_MAX_KEY];

  extern int  psp_update_keys(void);
  extern void kbd_wait_start(void);
  extern void psp_init_keyboard(void);
  extern void psp_kbd_wait_no_button(void);
  extern int  psp_kbd_is_danzeff_mode(void);
  extern void psp_kbd_display_active_mapping(void);
  extern int psp_kbd_load_mapping(char *kbd_filename);
  extern int psp_kbd_save_mapping(char *kbd_filename);
  extern void psp_kbd_display_active_mapping(void);
  extern void kbd_change_auto_fire(int auto_fire);

#ifdef __cplusplus
}
#endif
# endif
