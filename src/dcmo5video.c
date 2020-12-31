///////////////////////////////////////////////////////////////////////////////
// DCMO5VIDEO.C - Fonctions d'affichage pour dcmo5
// Author   : Daniel Coulom - danielcoulom@gmail.com
// Web site : http://dcmo5.free.fr
// Created  : July 2006
//
// This file is part of DCMO5 v11.
// 
// DCMO5 v11 is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// DCMO5 v11 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with DCMO5 v11.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <SDL/SDL.h>
#include <string.h>
#include "global.h"
#include "psp_sdl.h"

// global variables //////////////////////////////////////////////////////////
int xmouse;                    //abscisse souris dans fenetre utilisateur
int ymouse;                    //ordonnée souris dans fenetre utilisateur
int vblcount, framecount;      //compteur d'affichage 
u16 pcolor[19][8];
int framedelay;                //nombre de VBL entre deux affichages de l'ecran

//Initialisation palette /////////////////////////////////////////////////////
void Initpalette()
{
 int i, j;
 //palette
 // 0 noir  1 rouge  2 vert   3 jaune   4 bleu   5 magenta   6 cyan   7 blanc
 // 8 gris  9 rose  10 vert  11 jaune  12 bleu  13 magenta  14 cyan  15 orange
 int r[19]={0,15,0,15,0,15,0,15,7,10,3,10,3,10,7,11,11,14,2};
 int v[19]={0,0,15,15,0,0,15,15,7,3,10,10,3,3,14,3,11,14,2};
 int b[19]={0,0,0,0,15,15,15,15,7,3,3,3,10,10,14,0,11,14,2};
 //definition des intensites pour correction gamma
 int g[16]={0,60,90,110,130,148,165,180,193,205,215,225,230,235,240,255};
 //calcul de la palette 
  for(i = 0; i < 19; i++) {
    for(j = 0; j < 8; j++) 
    {
      pcolor[i][j] = psp_sdl_rgb(g[r[i]], g[v[i]], g[b[i]]);
    }
  }
}

extern SDL_Surface* blit_surface;

void
InitScreen()
{
}

static inline void 
ComposeMO5line(int a, u16* glob_p)
{
 int i, j, forme;
 u16 *c[2];
 u16 col;
 extern char ram[];
 int x = 0;
 for(i = 0; i < 40; i++) {
   c[0] = (u16 *)(pcolor + (ram[a] & 0x0f));
   c[1] = (u16 *)(pcolor + ((ram[a] >> 4) & 0x0f));
   forme = ram[0x2000 | a++]; 
   int mask = 0x1 << 7;
   for(j = 7; j >=0; j--) {
     col = *c[(forme & mask) ? 1:0];
     mask = mask >> 1;
     *glob_p++ = col; x++;
   }
 }           
}

void 
Displayline(int n)
{
  if (n == 0) {
   if(vblcount++ < framedelay) return;
   vblcount = 0;    
   psp_mo5_display();
   return;
  } 

  if(vblcount != 0) return;
  if((n > 255) || (n < 56)) return;

  u16* pmin = blit_surface->pixels;
  pmin += 8 + (8 * MO5_WIDTH);
  u16* p0 = pmin + (n - 56) * MO5_WIDTH;
  ComposeMO5line((n - 56) * 40, p0);   //partie centrale
} 
