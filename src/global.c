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

   Ludovic Jacomme <Ludovic.Jacomme@gmail.com>
*/

#include <stdio.h>
#include <zlib.h>
#include "SDL.h"

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspsdk.h>
#include <pspctrl.h>
#include <pspthreadman.h>
#include <pspiofilemgr.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <zlib.h>
#include <psppower.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "global.h"
#include "psp_mo5.h"
#include "psp_fmgr.h"
#include "psp_kbd.h"
#include "psp_sdl.h"

  MO5_t MO5;

  int psp_screenshot_mode = 0;

void
psp_global_initialize()
{
  memset(&MO5, 0, sizeof(MO5_t));
  getcwd(MO5.mo5_home_dir,MAX_PATH);
  mo5_default_settings();
}

int 
SDL_main(int argc, char **argv)
{
  psp_global_initialize();
  psp_sdl_init();

  mo5_update_save_name("");
  mo5_load_settings();
  scePowerSetClockFrequency(MO5.psp_cpu_clock, MO5.psp_cpu_clock, MO5.psp_cpu_clock/2);

  psp_sdl_black_screen();
  psp_mo5_initialize();
  psp_run_load_file();
  psp_mo5_main_loop();

  psp_sdl_exit(0);

  return 0;
}

//LUDO:
void
mo5_default_settings()
{
  //LUDO:
  MO5.mo5_snd_enable    = 1;
  MO5.mo5_render_mode   = MO5_RENDER_FIT_H;
  MO5.mo5_delta_y       = 0;
  MO5.mo5_vsync         = 0;
  MO5.mo5_speed_limiter = 50;
  MO5.mo5_auto_fire = 0;
  MO5.mo5_auto_fire_period = 6;
  MO5.mo5_auto_fire_pressed = 0;
  MO5.psp_reverse_analog  = 0;
  MO5.psp_cpu_clock       = 222;
  MO5.psp_screenshot_id   = 0;
  MO5.psp_display_lr      = 0;
  MO5.mo5_disk_mode       = 1;
  MO5.mo5_view_fps        = 0;

  scePowerSetClockFrequency(MO5.psp_cpu_clock, MO5.psp_cpu_clock, MO5.psp_cpu_clock/2);
}

static int
loc_mo5_save_settings(char *chFileName)
{
  FILE* FileDesc;
  int   error = 0;

  FileDesc = fopen(chFileName, "w");
  if (FileDesc != (FILE *)0 ) {

    fprintf(FileDesc, "psp_cpu_clock=%d\n"      , MO5.psp_cpu_clock);
    fprintf(FileDesc, "psp_reverse_analog=%d\n" , MO5.psp_reverse_analog);
    fprintf(FileDesc, "psp_display_lr=%d\n"     , MO5.psp_display_lr);
    fprintf(FileDesc, "psp_skip_max_frame=%d\n" , MO5.psp_skip_max_frame);
    fprintf(FileDesc, "mo5_snd_enable=%d\n"   , MO5.mo5_snd_enable);
    fprintf(FileDesc, "mo5_render_mode=%d\n"  , MO5.mo5_render_mode);
    fprintf(FileDesc, "mo5_delta_y=%d\n"        , MO5.mo5_delta_y);
    fprintf(FileDesc, "mo5_vsync=%d\n"        , MO5.mo5_vsync);
    fprintf(FileDesc, "mo5_speed_limiter=%d\n", MO5.mo5_speed_limiter);
    fprintf(FileDesc, "mo5_auto_fire_period=%d\n", MO5.mo5_auto_fire_period);
    fprintf(FileDesc, "mo5_view_fps=%d\n"     , MO5.mo5_view_fps);
    fprintf(FileDesc, "mo5_disk_mode=%d\n"     , MO5.mo5_disk_mode);

    fclose(FileDesc);

  } else {
    error = 1;
  }

  return error;
}

int
mo5_save_settings(void)
{
  char  FileName[MAX_PATH+1];
  int   error;

  error = 1;

  snprintf(FileName, MAX_PATH, "%s/set/%s.set", MO5.mo5_home_dir, MO5.mo5_save_name);
  error = loc_mo5_save_settings(FileName);

  return error;
}

static int
loc_mo5_load_settings(char *chFileName)
{
  char  Buffer[512];
  char *Scan;
  unsigned int Value;
  FILE* FileDesc;

  FileDesc = fopen(chFileName, "r");
  if (FileDesc == (FILE *)0 ) return 0;

  while (fgets(Buffer,512, FileDesc) != (char *)0) {

    Scan = strchr(Buffer,'\n');
    if (Scan) *Scan = '\0';
    /* For this #@$% of windows ! */
    Scan = strchr(Buffer,'\r');
    if (Scan) *Scan = '\0';
    if (Buffer[0] == '#') continue;

    Scan = strchr(Buffer,'=');
    if (! Scan) continue;

    *Scan = '\0';
    Value = atoi(Scan+1);

    if (!strcasecmp(Buffer,"psp_cpu_clock"))      MO5.psp_cpu_clock = Value;
    else
    if (!strcasecmp(Buffer,"psp_reverse_analog")) MO5.psp_reverse_analog = Value;
    else
    if (!strcasecmp(Buffer,"psp_display_lr"))     MO5.psp_display_lr = Value;
    else
    if (!strcasecmp(Buffer,"psp_skip_max_frame")) MO5.psp_skip_max_frame = Value;
    else
    if (!strcasecmp(Buffer,"mo5_snd_enable"))   MO5.mo5_snd_enable = Value;
    else
    if (!strcasecmp(Buffer,"mo5_render_mode"))  MO5.mo5_render_mode = Value;
    else
    if (!strcasecmp(Buffer,"mo5_vsync"))  MO5.mo5_vsync = Value;
    else
    if (!strcasecmp(Buffer,"mo5_speed_limiter"))  MO5.mo5_speed_limiter = Value;
    else
    if (!strcasecmp(Buffer,"mo5_view_fps"))  MO5.mo5_view_fps = Value;
    else
    if (!strcasecmp(Buffer,"mo5_auto_fire_period")) MO5.mo5_auto_fire_period = Value;
    else
    if (!strcasecmp(Buffer,"mo5_disk_mode"))     MO5.mo5_disk_mode = Value;
    else
    if (!strcasecmp(Buffer,"mo5_delta_y"))  MO5.mo5_delta_y = Value;
  }

  fclose(FileDesc);

  scePowerSetClockFrequency(MO5.psp_cpu_clock, MO5.psp_cpu_clock, MO5.psp_cpu_clock/2);

  return 0;
}

int
mo5_load_settings()
{
  char  FileName[MAX_PATH+1];
  int   error;

  error = 1;

  snprintf(FileName, MAX_PATH, "%s/set/%s.set", MO5.mo5_home_dir, MO5.mo5_save_name);
  error = loc_mo5_load_settings(FileName);

  return error;
}

int
mo5_load_file_settings(char *FileName)
{
  return loc_mo5_load_settings(FileName);
}

static int 
loc_load_rom(char *TmpName)
{
  int error = psp_mo5_load_rom(TmpName);
  return error;
}

static int 
loc_load_rom_buffer(char *rom_buffer, int rom_size)
{
  int error = psp_mo5_load_rom_buffer(rom_buffer, rom_size);
  if (rom_buffer) free(rom_buffer);
  return error;
}


void
mo5_update_save_name(char *Name)
{
  char        TmpFileName[MAX_PATH];
# ifdef LINUX_MODE
  struct stat aStat;
# else
  SceIoStat   aStat;
# endif
  int         index;
  char       *SaveName;
  char       *Scan1;
  char       *Scan2;

  SaveName = strrchr(Name,'/');
  if (SaveName != (char *)0) SaveName++;
  else                       SaveName = Name;

  if (!strncasecmp(SaveName, "sav_", 4)) {
    Scan1 = SaveName + 4;
    Scan2 = strrchr(Scan1, '_');
    if (Scan2 && (Scan2[1] >= '0') && (Scan2[1] <= '5')) {
      strncpy(MO5.mo5_save_name, Scan1, MAX_PATH);
      MO5.mo5_save_name[Scan2 - Scan1] = '\0';
    } else {
      strncpy(MO5.mo5_save_name, SaveName, MAX_PATH);
    }
  } else {
    strncpy(MO5.mo5_save_name, SaveName, MAX_PATH);
  }

  if (MO5.mo5_save_name[0] == '\0') {
    strcpy(MO5.mo5_save_name,"default");
  }

  for (index = 0; index < MO5_MAX_SAVE_STATE; index++) {
    MO5.mo5_save_state[index].used  = 0;
    memset(&MO5.mo5_save_state[index].date, 0, sizeof(ScePspDateTime));
    MO5.mo5_save_state[index].thumb = 0;

    snprintf(TmpFileName, MAX_PATH, "%s/save/sav_%s_%d.sta", MO5.mo5_home_dir, MO5.mo5_save_name, index);
# ifdef LINUX_MODE
    if (! stat(TmpFileName, &aStat)) 
# else
    if (! sceIoGetstat(TmpFileName, &aStat))
# endif
    {
      MO5.mo5_save_state[index].used = 1;
      MO5.mo5_save_state[index].date = aStat.st_mtime;
      snprintf(TmpFileName, MAX_PATH, "%s/save/sav_%s_%d.png", MO5.mo5_home_dir, MO5.mo5_save_name, index);
# ifdef LINUX_MODE
      if (! stat(TmpFileName, &aStat)) 
# else
      if (! sceIoGetstat(TmpFileName, &aStat))
# endif
      {
        if (psp_sdl_load_thumb_png(MO5.mo5_save_state[index].surface, TmpFileName)) {
          MO5.mo5_save_state[index].thumb = 1;
        }
      }
    }
  }

  MO5.comment_present = 0;
  snprintf(TmpFileName, MAX_PATH, "%s/txt/%s.txt", MO5.mo5_home_dir, MO5.mo5_save_name);
# ifdef LINUX_MODE
  if (! stat(TmpFileName, &aStat)) 
# else
  if (! sceIoGetstat(TmpFileName, &aStat))
# endif
  {
    MO5.comment_present = 1;
  }
}

void
reset_save_name()
{
  mo5_update_save_name("");
}

typedef struct thumb_list {
  struct thumb_list *next;
  char              *name;
  char              *thumb;
} thumb_list;

static thumb_list* loc_head_thumb = 0;

static void
loc_del_thumb_list()
{
  while (loc_head_thumb != 0) {
    thumb_list *del_elem = loc_head_thumb;
    loc_head_thumb = loc_head_thumb->next;
    if (del_elem->name) free( del_elem->name );
    if (del_elem->thumb) free( del_elem->thumb );
    free(del_elem);
  }
}

static void
loc_add_thumb_list(char* filename)
{
  thumb_list *new_elem;
  char tmp_filename[MAX_PATH];

  strcpy(tmp_filename, filename);
  char* save_name = tmp_filename;

  /* .png extention */
  char* Scan = strrchr(save_name, '.');
  if ((! Scan) || (strcasecmp(Scan, ".png"))) return;
  *Scan = 0;

  if (strncasecmp(save_name, "sav_", 4)) return;
  save_name += 4;

  Scan = strrchr(save_name, '_');
  if (! Scan) return;
  *Scan = 0;

  /* only one png for a given save name */
  new_elem = loc_head_thumb;
  while (new_elem != 0) {
    if (! strcasecmp(new_elem->name, save_name)) return;
    new_elem = new_elem->next;
  }

  new_elem = (thumb_list *)malloc( sizeof( thumb_list ) );
  new_elem->next = loc_head_thumb;
  loc_head_thumb = new_elem;
  new_elem->name  = strdup( save_name );
  new_elem->thumb = strdup( filename );
}

void
load_thumb_list()
{
# ifndef LINUX_MODE
  char SaveDirName[MAX_PATH];
  struct SceIoDirent a_dirent;
  int fd = 0;

  loc_del_thumb_list();

  snprintf(SaveDirName, MAX_PATH, "%s/save", MO5.mo5_home_dir);
  memset(&a_dirent, 0, sizeof(a_dirent));

  fd = sceIoDopen(SaveDirName);
  if (fd < 0) return;

  while (sceIoDread(fd, &a_dirent) > 0) {
    if(a_dirent.d_name[0] == '.') continue;
    if(! FIO_S_ISDIR(a_dirent.d_stat.st_mode)) 
    {
      loc_add_thumb_list( a_dirent.d_name );
    }
  }
  sceIoDclose(fd);
# else
  char SaveDirName[MAX_PATH];
  DIR* fd = 0;

  loc_del_thumb_list();

  snprintf(SaveDirName, MAX_PATH, "%s/save", MO5.mo5_home_dir);

  fd = opendir(SaveDirName);
  if (!fd) return;

  struct dirent *a_dirent;
  while ((a_dirent = readdir(fd)) != 0) {
    if(a_dirent->d_name[0] == '.') continue;
    if (a_dirent->d_type != DT_DIR) 
    {
      loc_add_thumb_list( a_dirent->d_name );
    }
  }
  closedir(fd);
# endif
}

int
load_thumb_if_exists(char *Name)
{
  char        FileName[MAX_PATH];
  char        ThumbFileName[MAX_PATH];
# ifdef LINUX_MODE
  struct stat aStat;
# else
  SceIoStat   aStat;
# endif
  char       *SaveName;
  char       *Scan;

  strcpy(FileName, Name);
  SaveName = strrchr(FileName,'/');
  if (SaveName != (char *)0) SaveName++;
  else                       SaveName = FileName;

  Scan = strrchr(SaveName,'.');
  if (Scan) *Scan = '\0';

  if (!SaveName[0]) return 0;

  thumb_list *scan_list = loc_head_thumb;
  while (scan_list != 0) {
    if (! strcasecmp( SaveName, scan_list->name)) {
      snprintf(ThumbFileName, MAX_PATH, "%s/save/%s", MO5.mo5_home_dir, scan_list->thumb);
# ifdef LINUX_MODE
      if (! stat(ThumbFileName, &aStat)) 
# else
      if (! sceIoGetstat(ThumbFileName, &aStat))
# endif
      {
        if (psp_sdl_load_thumb_png(save_surface, ThumbFileName)) {
          return 1;
        }
      }
    }
    scan_list = scan_list->next;
  }
  return 0;
}

typedef struct comment_list {
  struct comment_list *next;
  char              *name;
  char              *filename;
} comment_list;

static comment_list* loc_head_comment = 0;

static void
loc_del_comment_list()
{
  while (loc_head_comment != 0) {
    comment_list *del_elem = loc_head_comment;
    loc_head_comment = loc_head_comment->next;
    if (del_elem->name) free( del_elem->name );
    if (del_elem->filename) free( del_elem->filename );
    free(del_elem);
  }
}

static void
loc_add_comment_list(char* filename)
{
  comment_list *new_elem;
  char  tmp_filename[MAX_PATH];

  strcpy(tmp_filename, filename);
  char* save_name = tmp_filename;

  /* .png extention */
  char* Scan = strrchr(save_name, '.');
  if ((! Scan) || (strcasecmp(Scan, ".txt"))) return;
  *Scan = 0;

  /* only one txt for a given save name */
  new_elem = loc_head_comment;
  while (new_elem != 0) {
    if (! strcasecmp(new_elem->name, save_name)) return;
    new_elem = new_elem->next;
  }

  new_elem = (comment_list *)malloc( sizeof( comment_list ) );
  new_elem->next = loc_head_comment;
  loc_head_comment = new_elem;
  new_elem->name  = strdup( save_name );
  new_elem->filename = strdup( filename );
}

void
load_comment_list()
{
# ifndef LINUX_MODE
  char SaveDirName[MAX_PATH];
  struct SceIoDirent a_dirent;
  int fd = 0;

  loc_del_comment_list();

  snprintf(SaveDirName, MAX_PATH, "%s/txt", MO5.mo5_home_dir);
  memset(&a_dirent, 0, sizeof(a_dirent));

  fd = sceIoDopen(SaveDirName);
  if (fd < 0) return;

  while (sceIoDread(fd, &a_dirent) > 0) {
    if(a_dirent.d_name[0] == '.') continue;
    if(! FIO_S_ISDIR(a_dirent.d_stat.st_mode)) 
    {
      loc_add_comment_list( a_dirent.d_name );
    }
  }
  sceIoDclose(fd);
# else
  char SaveDirName[MAX_PATH];
  DIR* fd = 0;

  loc_del_comment_list();

  snprintf(SaveDirName, MAX_PATH, "%s/txt", MO5.mo5_home_dir);

  fd = opendir(SaveDirName);
  if (!fd) return;

  struct dirent *a_dirent;
  while ((a_dirent = readdir(fd)) != 0) {
    if(a_dirent->d_name[0] == '.') continue;
    if (a_dirent->d_type != DT_DIR) 
    {
      loc_add_comment_list( a_dirent->d_name );
    }
  }
  closedir(fd);
# endif
}

char*
load_comment_if_exists(char *Name)
{
static char loc_comment_buffer[128];

  char        FileName[MAX_PATH];
  char        TmpFileName[MAX_PATH];
  FILE       *a_file;
  char       *SaveName;
  char       *Scan;

  loc_comment_buffer[0] = 0;

  strcpy(FileName, Name);
  SaveName = strrchr(FileName,'/');
  if (SaveName != (char *)0) SaveName++;
  else                       SaveName = FileName;

  Scan = strrchr(SaveName,'.');
  if (Scan) *Scan = '\0';

  if (!SaveName[0]) return 0;

  comment_list *scan_list = loc_head_comment;
  while (scan_list != 0) {
    if (! strcasecmp( SaveName, scan_list->name)) {
      snprintf(TmpFileName, MAX_PATH, "%s/txt/%s", MO5.mo5_home_dir, scan_list->filename);
      a_file = fopen(TmpFileName, "r");
      if (a_file) {
        char* a_scan = 0;
        loc_comment_buffer[0] = 0;
        if (fgets(loc_comment_buffer, 60, a_file) != 0) {
          a_scan = strchr(loc_comment_buffer, '\n');
          if (a_scan) *a_scan = '\0';
          /* For this #@$% of windows ! */
          a_scan = strchr(loc_comment_buffer,'\r');
          if (a_scan) *a_scan = '\0';
          fclose(a_file);
          return loc_comment_buffer;
        }
        fclose(a_file);
        return 0;
      }
    }
    scan_list = scan_list->next;
  }
  return 0;
}

void
mo5_kbd_load(void)
{
  char        TmpFileName[MAX_PATH + 1];
  struct stat aStat;

  snprintf(TmpFileName, MAX_PATH, "%s/kbd/%s.kbd", MO5.mo5_home_dir, MO5.mo5_save_name );
  if (! stat(TmpFileName, &aStat)) {
    psp_kbd_load_mapping(TmpFileName);
  }
}

int
mo5_kbd_save(void)
{
  char TmpFileName[MAX_PATH + 1];
  snprintf(TmpFileName, MAX_PATH, "%s/kbd/%s.kbd", MO5.mo5_home_dir, MO5.mo5_save_name );
  return( psp_kbd_save_mapping(TmpFileName) );
}


int
mo5_load_rom(char *FileName, int zip_format)
{
  char  SaveName[MAX_PATH+1];
  char*  ExtractName;
  char*  scan;
  int    format;
  int    error;
  size_t unzipped_size;

  error = 1;

  if (zip_format) {

    ExtractName = find_possible_filename_in_zip( FileName, "rom");
    if (ExtractName) {
      strncpy(SaveName, FileName, MAX_PATH);
      scan = strrchr(SaveName,'.');
      if (scan) *scan = '\0';
      mo5_update_save_name(SaveName);
      char* rom_buffer = extract_file_in_memory ( FileName, ExtractName, &unzipped_size);
      if (rom_buffer) {
        error = loc_load_rom_buffer( rom_buffer, unzipped_size );
      }
    }

  } else {
    strncpy(SaveName,FileName,MAX_PATH);
    scan = strrchr(SaveName,'.');
    if (scan) *scan = '\0';
    mo5_update_save_name(SaveName);
    error = loc_load_rom(FileName);
  }

  if (! error ) {
    mo5_kbd_load();
    mo5_load_cheat();
    mo5_load_settings();
  }

  return error;
}

int
mo5_load_k7(char *FileName)
{
  char  SaveName[MAX_PATH+1];
  char *scan;

  int error = Loadk7( FileName );
  if (! error) {
    strncpy(SaveName,FileName,MAX_PATH);
    scan = strrchr(SaveName,'.');
    if (scan) *scan = '\0';
    mo5_update_save_name( SaveName );
    mo5_kbd_load();
    mo5_load_cheat();
    mo5_load_settings();
  }
}

int
mo5_load_fd(char *FileName)
{
  char  SaveName[MAX_PATH+1];
  char *scan;

  int error = Loadfd( FileName );
  if (! error) {
    strncpy(SaveName,FileName,MAX_PATH);
    scan = strrchr(SaveName,'.');
    if (scan) *scan = '\0';
    mo5_update_save_name( SaveName );
    mo5_kbd_load();
    mo5_load_cheat();
    mo5_load_settings();
  }
  return error;
}

static int
loc_load_state(char *filename)
{
  int error;
  error = ! dcmo5_load_state(filename);
  return error;
}

int
mo5_load_dos()
{
  return loc_load_state("./dos.stz");
}

int
mo5_load_state(char *FileName)
{
  char *pchPtr;
  char *scan;
  char  SaveName[MAX_PATH+1];
  char  TmpFileName[MAX_PATH + 1];
  int   format;
  int   error;

  error = 1;

  strncpy(SaveName,FileName,MAX_PATH);
  scan = strrchr(SaveName,'.');
  if (scan) *scan = '\0';
  mo5_update_save_name(SaveName);
  error = loc_load_state(FileName);

  if (! error ) {
    mo5_kbd_load();
    mo5_load_cheat();
    mo5_load_settings();
  }

  return error;
}

static int
loc_mo5_save_state(char *filename)
{
  int error;
  error = ! dcmo5_save_state(filename);
  return error;
}

int
mo5_snapshot_save_slot(int save_id)
{
  char      FileName[MAX_PATH+1];
# ifdef LINUX_MODE
  struct stat aStat;
# else
  SceIoStat   aStat;
# endif
  int       error;

  error = 1;

  if (save_id < MO5_MAX_SAVE_STATE) {
    snprintf(FileName, MAX_PATH, "%s/save/sav_%s_%d.stz", MO5.mo5_home_dir, MO5.mo5_save_name, save_id);
    error = loc_mo5_save_state(FileName);
    if (! error) {
# ifdef LINUX_MODE
      if (! stat(FileName, &aStat)) 
# else
      if (! sceIoGetstat(FileName, &aStat))
# endif
      {
        MO5.mo5_save_state[save_id].used  = 1;
        MO5.mo5_save_state[save_id].thumb = 0;
        MO5.mo5_save_state[save_id].date  = aStat.st_mtime;
        snprintf(FileName, MAX_PATH, "%s/save/sav_%s_%d.png", MO5.mo5_home_dir, MO5.mo5_save_name, save_id);
        if (psp_sdl_save_thumb_png(MO5.mo5_save_state[save_id].surface, FileName)) {
          MO5.mo5_save_state[save_id].thumb = 1;
        }
      }
    }
  }

  return error;
}

int
mo5_snapshot_load_slot(int load_id)
{
  char  FileName[MAX_PATH+1];
  int   error;

  error = 1;

  if (load_id < MO5_MAX_SAVE_STATE) {
    snprintf(FileName, MAX_PATH, "%s/save/sav_%s_%d.stz", MO5.mo5_home_dir, MO5.mo5_save_name, load_id);
    error = loc_load_state(FileName);
  }
  return error;
}

int
mo5_snapshot_del_slot(int save_id)
{
  char  FileName[MAX_PATH+1];
# ifdef LINUX_MODE
  struct stat aStat;
# else
  SceIoStat   aStat;
# endif
  int   error;

  error = 1;

  if (save_id < MO5_MAX_SAVE_STATE) {
    snprintf(FileName, MAX_PATH, "%s/save/sav_%s_%d.stz", MO5.mo5_home_dir, MO5.mo5_save_name, save_id);
    error = remove(FileName);
    if (! error) {
      MO5.mo5_save_state[save_id].used  = 0;
      MO5.mo5_save_state[save_id].thumb = 0;
      memset(&MO5.mo5_save_state[save_id].date, 0, sizeof(ScePspDateTime));

      /* We keep always thumbnail with id 0, to have something to display in the file requester */ 
      if (save_id != 0) {
        snprintf(FileName, MAX_PATH, "%s/save/sav_%s_%d.png", MO5.mo5_home_dir, MO5.mo5_save_name, save_id);
# ifdef LINUX_MODE
        if (! stat(FileName, &aStat)) 
# else
        if (! sceIoGetstat(FileName, &aStat))
# endif
        {
          remove(FileName);
        }
      }
    }
  }

  return error;
}

static int
loc_mo5_save_cheat(char *chFileName)
{
  FILE* FileDesc;
  int   cheat_num;
  int   error = 0;

  FileDesc = fopen(chFileName, "w");
  if (FileDesc != (FILE *)0 ) {

    for (cheat_num = 0; cheat_num < MO5_MAX_CHEAT; cheat_num++) {
      MO5_cheat_t* a_cheat = &MO5.mo5_cheat[cheat_num];
      if (a_cheat->type != MO5_CHEAT_NONE) {
        fprintf(FileDesc, "%d,%x,%x,%s\n", 
                a_cheat->type, a_cheat->addr, a_cheat->value, a_cheat->comment);
      }
    }
    fclose(FileDesc);

  } else {
    error = 1;
  }

  return error;
}

int
mo5_save_cheat(void)
{
  char  FileName[MAX_PATH+1];
  int   error;

  error = 1;

  snprintf(FileName, MAX_PATH, "%s/cht/%s.cht", MO5.mo5_home_dir, MO5.mo5_save_name);
  error = loc_mo5_save_cheat(FileName);

  return error;
}

static int
loc_mo5_load_cheat(char *chFileName)
{
  char  Buffer[512];
  char *Scan;
  char *Field;
  unsigned int  cheat_addr;
  unsigned int  cheat_value;
  unsigned int  cheat_type;
  char         *cheat_comment;
  int           cheat_num;
  FILE* FileDesc;

  memset(MO5.mo5_cheat, 0, sizeof(MO5.mo5_cheat));
  cheat_num = 0;

  FileDesc = fopen(chFileName, "r");
  if (FileDesc == (FILE *)0 ) return 0;

  while (fgets(Buffer,512, FileDesc) != (char *)0) {

    Scan = strchr(Buffer,'\n');
    if (Scan) *Scan = '\0';
    /* For this #@$% of windows ! */
    Scan = strchr(Buffer,'\r');
    if (Scan) *Scan = '\0';
    if (Buffer[0] == '#') continue;

    /* %d, %x, %x, %s */
    Field = Buffer;
    Scan = strchr(Field, ',');
    if (! Scan) continue;
    *Scan = 0;
    if (sscanf(Field, "%d", &cheat_type) != 1) continue;
    Field = Scan + 1;
    Scan = strchr(Field, ',');
    if (! Scan) continue;
    *Scan = 0;
    if (sscanf(Field, "%x", &cheat_addr) != 1) continue;
    Field = Scan + 1;
    Scan = strchr(Field, ',');
    if (! Scan) continue;
    *Scan = 0;
    if (sscanf(Field, "%x", &cheat_value) != 1) continue;
    Field = Scan + 1;
    cheat_comment = Field;

    if (cheat_type <= MO5_CHEAT_NONE) continue;

    MO5_cheat_t* a_cheat = &MO5.mo5_cheat[cheat_num];

    a_cheat->type  = cheat_type;
    a_cheat->addr  = cheat_addr;
    a_cheat->value = cheat_value;
    strncpy(a_cheat->comment, cheat_comment, sizeof(a_cheat->comment));
    a_cheat->comment[sizeof(a_cheat->comment)-1] = 0;

    if (++cheat_num >= MO5_MAX_CHEAT) break;
  }
  fclose(FileDesc);

  return 0;
}

int
mo5_load_cheat()
{
  char  FileName[MAX_PATH+1];
  int   error;

  error = 1;

  snprintf(FileName, MAX_PATH, "%s/cht/%s.cht", MO5.mo5_home_dir, MO5.mo5_save_name);
  error = loc_mo5_load_cheat(FileName);

  return error;
}

int
mo5_load_file_cheat(char *FileName)
{
  return loc_mo5_load_cheat(FileName);
}

void
mo5_apply_cheats()
{
  extern char* ramuser;
  int cheat_num;
  for (cheat_num = 0; cheat_num < MO5_MAX_CHEAT; cheat_num++) {
    MO5_cheat_t* a_cheat = &MO5.mo5_cheat[cheat_num];
    if (a_cheat->type == MO5_CHEAT_ENABLE) {
      ramuser[a_cheat->addr & (MO5_RAM_SIZE-1)] = a_cheat->value;
    }
  }
}




void
mo5_treat_command_key(int mo5_idx)
{
  int new_render;

  switch (mo5_idx) 
  {
    case MO5_C_FPS: MO5.mo5_view_fps = ! MO5.mo5_view_fps;
    break;
    case MO5_C_JOY: MO5.psp_reverse_analog = ! MO5.psp_reverse_analog;
    break;
    case MO5_C_RENDER: 
      psp_sdl_black_screen();
      new_render = MO5.mo5_render_mode + 1;
      if (new_render > MO5_LAST_RENDER) new_render = 0;
      MO5.mo5_render_mode = new_render;
    break;
    case MO5_C_LOAD: psp_main_menu_load_current();
    break;
    case MO5_C_SAVE: psp_main_menu_save_current(); 
    break;
    case MO5_C_RESET: 
       psp_sdl_black_screen();
       psp_mo5_soft_reset(); 
       reset_save_name();
    break;
    case MO5_C_AUTOFIRE: 
       kbd_change_auto_fire(! MO5.mo5_auto_fire);
    break;
    case MO5_C_DECFIRE: 
      if (MO5.mo5_auto_fire_period > 0) MO5.mo5_auto_fire_period--;
    break;
    case MO5_C_INCFIRE: 
      if (MO5.mo5_auto_fire_period < 19) MO5.mo5_auto_fire_period++;
    break;
    case MO5_C_DECDELTA:
      if (MO5.mo5_delta_y > -20) MO5.mo5_delta_y--;
    break;
    case MO5_C_INCDELTA: 
      if (MO5.mo5_delta_y <  20) MO5.mo5_delta_y++;
    break;
    case MO5_C_SCREEN: psp_screenshot_mode = 10;
    break;
  }
}

  int psp_exit_now = 0;

void
myCtrlPeekBufferPositive( SceCtrlData* pc, int count )
{
  if (psp_exit_now) psp_sdl_exit(0);
  sceCtrlPeekBufferPositive( pc, count );
}

