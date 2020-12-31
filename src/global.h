#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#ifdef __cplusplus
extern "C" {
#endif

# define MO5_RENDER_NORMAL     0
# define MO5_RENDER_FIT_H      1
# define MO5_RENDER_X125       2
# define MO5_RENDER_FIT        3
# define MO5_RENDER_MAX        4
# define MO5_LAST_RENDER       4

# define MO5_WIDTH   336
# define MO5_HEIGHT  216

# define SNAP_WIDTH   (MO5_WIDTH/2)
# define SNAP_HEIGHT  (MO5_HEIGHT/2)

# define MAX_PATH   256
# define MO5_MAX_SAVE_STATE 5
# define MO5_MAX_CHEAT        10

#include <psptypes.h>
#include <pspctrl.h>

#define TRUE 1
#define FALSE 0

#include <SDL.h>

typedef unsigned char BYTE;
typedef unsigned int DWORD;
typedef unsigned short WORD;
typedef unsigned int BOOL;

#define MO5_CHEAT_NONE    0
#define MO5_CHEAT_ENABLE  1
#define MO5_CHEAT_DISABLE 2

#define MO5_RAM_SIZE 0xa000

#define MO5_CHEAT_COMMENT_SIZE 25
  
  typedef struct MO5_cheat_t {
    unsigned char  type;
    unsigned short addr;
    unsigned char  value;
    char           comment[MO5_CHEAT_COMMENT_SIZE];
  } MO5_cheat_t;


  typedef struct MO5_save_t {

    SDL_Surface    *surface;
    char            used;
    char            thumb;
    ScePspDateTime  date;

  } MO5_save_t;

  typedef struct MO5_t {

    MO5_save_t mo5_save_state[MO5_MAX_SAVE_STATE];
    MO5_cheat_t mo5_cheat[MO5_MAX_CHEAT];

    int  comment_present;
    char mo5_save_name[MAX_PATH];
    char mo5_home_dir[MAX_PATH];
    int  psp_screenshot_id;
    int  psp_cpu_clock;
    int  psp_reverse_analog;
    int  psp_display_lr;
    int  mo5_view_fps;
    int  mo5_current_fps;
    int  mo5_snd_enable;
    int  mo5_render_mode;
    int  mo5_vsync;
    int  mo5_delta_y;
    int  psp_active_joystick;
    int  psp_skip_max_frame;
    int  psp_skip_cur_frame;
    int  mo5_speed_limiter;
    int  mo5_auto_fire;
    int  mo5_auto_fire_pressed;
    int  mo5_auto_fire_period;
    int  mo5_disk_mode;

  } MO5_t;

  extern MO5_t MO5;


//END_LUDO:
  extern void mo5_default_settings(void);
  extern int mo5_save_settings(void);
  extern int mo5_load_settings();
  extern int mo5_load_file_settings(char *FileName);

  extern void mo5_update_save_name(char *Name);
  extern void reset_save_name();
  extern void mo5_kbd_load(void);
  extern int mo5_kbd_save(void);
  extern void emulator_reset(void);
  extern int mo5_load_rom(char *FileName, int zip_format);

  extern int mo5_load_k7(char *FileName);
  extern int mo5_load_fd(char *FileName);
  extern int mo5_load_state(char *FileName);
  extern int mo5_snapshot_save_slot(int save_id);
  extern int mo5_snapshot_load_slot(int load_id);
  extern int mo5_snapshot_del_slot(int save_id);

  extern int psp_screenshot_mode;

  extern void myCtrlPeekBufferPositive( SceCtrlData* pc, int count );
  extern int psp_exit_now;

#ifdef __cplusplus
}
#endif

#endif
