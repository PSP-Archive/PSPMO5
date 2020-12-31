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
#include <psptypes.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <SDL.h>

#include "global.h"
#include "psp_sound.h"

 static u16* loc_buffer_write = 0;
 static u16* loc_buffer_read  = 0;
 static int  loc_buffer_index = 0;
 static int  loc_buffer_length = 0;


void
psp_sound_add_sample()
{
 extern int sound;

  loc_buffer_write[loc_buffer_index++] = sound << 8;
  if (loc_buffer_index > loc_buffer_length) {
    char* swap_buffer = loc_buffer_write;
    loc_buffer_write = loc_buffer_read;
    loc_buffer_read  = swap_buffer;
    loc_buffer_index = 0;
  }
}

void 
psp_play_sound(void *udata, Uint8 *stream, int bufferlength)
{
  if (loc_buffer_read) {
    uint* src = (uint *)loc_buffer_read;
    uint* dst = (uint *)stream;
    int   len = bufferlength / 4;
    while (len--) {
      *dst++ = *src++;
    }
  }
# if 0 //LUDO:
 int i;
 int mcycles; //nombre de milliemes de cycles entre deux echantillons
 int icycles; //nombre entier de cycles entre deux echantillons
 extern int sound, frequency;
 extern int Run(int n);
 extern void Testshiftkey();
 //45 cycles 6809 a 992250 Hz = un echantillon a 22050 Hz
 for(i = 0; i < bufferlength; i++)
 {
  if(pause6809) {stream[i] = 128; continue;}
  //calcul et execution du nombre de cycles entre deux echantillons
  //nbre de cycles theoriques pour la periode en cours =
  //nbre theorique + relicat arrondi precedent - cycles en trop precedent
  mcycles = frequency * 100000 / 2205;   //milliemes de cycles theoriques
  mcycles += report;                     //milliemes de cycles corriges
  icycles = mcycles / 1000;              //nbre entier de cycles a effectuer 
  report = mcycles - 1000 * icycles;     //relicat de l'arrondi a reporter
  report -= 1000 * Run(icycles);         //retrancher les milliemes en trop  
  stream[i] = sound + 128;
 }
 Testshiftkey(); //contournement bug SDL sur l'appui simultane SHIFT, ALT, CTRL
# endif
}  

void
psp_sound_pause()
{
  if (MO5.mo5_snd_enable) {
    SDL_PauseAudio(1);
  }
}

void
psp_sound_resume()
{
  if (MO5.mo5_snd_enable) {
    SDL_PauseAudio(0);
  }
}

void
psp_sound_init()
{
#ifndef LINUX_MODE
  SDL_AudioSpec desired;
  SDL_AudioSpec obtained;

  memset(&obtained,0,sizeof(SDL_AudioSpec));
  memset(&desired,0,sizeof(SDL_AudioSpec));

  /* la SDL ne fait que du 44100 S16 */
  desired.freq     = 44100;
  desired.format   = AUDIO_S16LSB;
  desired.channels = 1;
  desired.samples  = 512; 
  desired.callback = psp_play_sound;
  desired.userdata = NULL;
# endif

  loc_buffer_read  = (u16 *)malloc( 1024 );
  loc_buffer_write = (u16 *)malloc( 1024 );

# ifndef LINUX_MODE
  if (SDL_OpenAudio(&desired, &obtained) < 0) {
    fprintf(stderr, "Could not open audio: %s\n", SDL_GetError());
    psp_sdl_exit(1);
  }
# endif
  loc_buffer_length = 512;
  loc_buffer_index  = 0;

  //fprintf(stdout, "obtained=%d %d %d\n", obtained.samples, obtained.format, obtained.freq);
}
