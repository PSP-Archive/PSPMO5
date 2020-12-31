///////////////////////////////////////////////////////////////////////////////
// DCMO5EMULATION.C - Emulateur Thomson MO5 portable
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

#include <string.h>
#include <stdio.h>
#include <zlib.h>
#include "dcmo5rom.h"
#include "dc6809emul.h"

// memory
char car[0x10000];   //espace cartouche 4x16K
char ram[0xc000];    //ram 48K 
char port[0x40];     //ports d'entree/sortie
// pointers
char *ramvideo;      //pointeur couleurs ou formes
char *ramuser;       //pointeur ram utilisateur fixe
char *romsys;        //pointeur rom systeme
char *rombank;       //pointeur banque rom ou cartouche
//flags cartouche
int cartype;         //type de cartouche (0=simple 1=switch bank, 2=os-9)
int carflags;        //bits0,1,4=bank, 2=cart-enabled, 3=write-enabled
//keyboard, joysticks and mouse
int touche[58];      //etat des touches MO5
int joysposition;    //position des manches
int joysaction;      //position des boutons d'action
int xpen, ypen;      //coordonnees crayon optique
int penbutton;       //mouse left button state
//affichage
int videolinecycle;  //compteur ligne (0-63)
int soundcycle;      //LUDO: pour le son
int videolinenumber; //numero de ligne video affich�e (0-311)
int bordercolor;     //couleur de la bordure de l'�cran
//divers
int opcycles;        //nombre de cycles de la derni�re op�ration
int sound;           //niveau du haut-parleur

//Acc�s m�moire
char MgetMO5(unsigned short a);
void MputMO5(unsigned short a, char c);
# if 0 //LUDO:
char (*Mgetc)(unsigned short);       //pointeur fonction courante
void (*Mputc)(unsigned short, char); //pointeur fonction courante
# else
# define Mgetc(A)   MgetMO5((A))
# define Mputc(A,V)   MputMO5((A), (V))
# endif
short Mgetw(unsigned short a) {return (Mgetc(a) << 8 | (Mgetc(++a) & 0xff));}
void Mputw(unsigned short a, short w) {Mputc(a, w >> 8); Mputc(++a, w);}

// Selection de banques memoire //////////////////////////////////////////////
void MO5videoram()
{
 ramvideo = ram + ((port[0] & 1) << 13);
 bordercolor = (port[0] >> 1) & 0x0f;
}   

void MO5rombank()
{
 if((carflags & 4) == 0) {rombank = mo5rom - 0xc000; return;}
 rombank = car - 0xb000 + ((carflags & 0x03) << 14);
 if(cartype == 2) if(carflags & 0x10) rombank += 0x10000;
}

void Switchmemo5bank(int a)
{
 if(cartype != 1) return;
 if((a & 0xfffc) != 0xbffc) return;
 carflags = (carflags & 0xfc) | (a & 3);
 MO5rombank(); 
}

// Signaux de synchronisation ligne et trame /////////////////////////////////
int Iniln()
{//11 microsecondes - 41 microsecondes - 12 microsecondes 
 if(videolinecycle < 23) return 0; else return 0x20;   
}    

int Initn()
{//debut � 12 microsecondes ligne 56, fin � 51 microsecondes ligne 255
 if(videolinenumber < 56) return 0;
 if(videolinenumber > 255) return 0;
 if(videolinenumber == 56) if (videolinecycle < 24) return 0;
 if(videolinenumber == 255) if (videolinecycle > 62) return 0;
 return 0x80;
} 

// Joystick emulation ////////////////////////////////////////////////////////
void Joysemul(int i, int state)
{
 //PA0=0 nord   PA1=0 sud   PA2=0 ouest   PA3=0 est   PB6=0 action
 //PA4=1 nord   PA5=1 sud   PA6=1 ouest   PA7=1 est   PB7=1 action
 int n;
 n = 0;
 switch(i)
 {
  case 0: if(joysposition & 0x02) n = 0x01; break;        
  case 1: if(joysposition & 0x01) n = 0x02; break;        
  case 2: if(joysposition & 0x08) n = 0x04; break;        
  case 3: if(joysposition & 0x04) n = 0x08; break;        
  case 4: if(joysposition & 0x20) n = 0x10; break;        
  case 5: if(joysposition & 0x10) n = 0x20; break;        
  case 6: if(joysposition & 0x80) n = 0x40; break;        
  case 7: if(joysposition & 0x40) n = 0x80; break;        
  case 8: if(state) joysaction |= 0x40; else joysaction &= 0xbf; return;        
  case 9: if(state) joysaction |= 0x80; else joysaction &= 0x7f; return; 
  default: return;       
 }
 if(state) joysposition |= n; else joysposition &= (~n); 
}

// Joystick move /////////////////////////////////////////////////////////////
void Joysmove(int n, int x, int y)
{
 n <<= 2;
 int i;
 i = (y < 27768) ? 0 : 0x80; Joysemul(n++, i);
 i = (y > 37767) ? 0 : 0x80; Joysemul(n++, i);
 i = (x < 27768) ? 0 : 0x80; Joysemul(n++, i);
 i = (x > 37767) ? 0 : 0x80; Joysemul(n++, i);
}

// Initialisation programme de l'ordinateur �mul� ////////////////////////////
void Initprog()
{
 int i;   
 short Mgetw(unsigned short a);
 for(i = 0; i < 58; i++) touche[i] = 0x80; //touches relach�es
 joysposition = 0xff;                      //manettes au centre
 joysaction = 0xc0;                        //boutons relach�s
 carflags &= 0xec;
 sound = 0; 
 MO5videoram();
 MO5rombank();
 CC = 0x10;
 PC = Mgetw(0xfffe);
}

// Hardreset de l'ordinateur �mul� ///////////////////////////////////////////
void Hardreset()
{
 int i;
 extern void Initpalette();
# if 0 //LUDO:
 int pause6809;    
 pause6809 = 1;  
 Mputc = MputMO5;
 Mgetc = MgetMO5;
# endif
 xpen = 0;
 ypen = 0;
 penbutton = 0;
 videolinecycle = 0;
 videolinenumber = 0;
 romsys = mo5rom - 0xc000;
 ramuser = ram + 0x2000;
 for(i = 0; i < sizeof(ram); i++) ram[i] = -((i & 0x80) >> 7);
 for(i = 0; i < sizeof(port); i++) port[i] = 0;
 for(i = 0; i < sizeof(car); i++) car[i] = 0;     
 Initpalette();
 Initprog();
# if 0 //LUDO:
 pause6809 = 0;
# endif
}

// Traitement des entrees-sorties ////////////////////////////////////////////
void Entreesortie(int io)
{
 extern void Readbitk7(), Readoctetk7(), Writeoctetk7(), Readpenxy();
 extern void Readsector(), Writesector(), Formatdisk(), Imprime();
 switch(io)
 {
  case 0x14: Readsector(); break;      //lit secteur qd-fd
  case 0x15: Writesector(); break;     //ecrit secteur qd-fd
  case 0x18: Formatdisk(); break;      //formatage qd-fd
  case 0x41: Readbitk7(); break;       //lit bit cassette
  case 0x42: Readoctetk7(); break;     //lit octet cassette
  case 0x45: Writeoctetk7(); break;    //ecrit octet cassette
  case 0x4b: Readpenxy(); break;       //lit position crayon
  case 0x51: Imprime(); break;         //imprime un caractere
  default: break;                      //code op. invalide
 }
}

// Execution n cycles processeur 6809 ////////////////////////////////////////
void Run()
{
 int i, opcycles;
 extern int Run6809();
 void Irq(); void Displayline(int);
 i = 0;
 while(1) {
  opcycles = Run6809(); //execution d'une instruction
  if(opcycles < 0) {Entreesortie(-opcycles); opcycles = 64;}
  i += opcycles;       //nombre total de cycles executes
  soundcycle += opcycles;
  if (soundcycle >= 45) {
    psp_sound_add_sample();
    soundcycle -= 45;
  }
  videolinecycle += opcycles;
  if(videolinecycle < 64) continue; //attente d'une fin de ligne
  videolinecycle -= 64;
  Displayline(videolinenumber++);
  if(videolinenumber < 312) continue; //attente d'une fin de trame    
  videolinenumber -= 312;
  psp_update_keys();
  Irq();     
 }
}  

// Ecriture memoire MO5 //////////////////////////////////////////////////////
void MputMO5(unsigned short a, char c)
{
 switch(a >> 12)
 {
  case 0x0: case 0x1: ramvideo[a] = c; break;
  case 0xa: switch(a)
   {
    case 0xa7c0: port[0] = c & 0x5f; MO5videoram(); break;
    case 0xa7c1: port[1] = c & 0x7f; sound = (c & 1) << 5; break;
    case 0xa7c2: port[2] = c & 0x3f; break;
    case 0xa7c3: port[3] = c & 0x3f; break;
    case 0xa7cb: carflags = c; MO5rombank(); break;
    case 0xa7cc: port[0x0c] = c; return;
    case 0xa7cd: port[0x0d] = c; sound = c & 0x3f; return;
    case 0xa7ce: port[0x0e] = c; return; //registre controle position joysticks
    case 0xa7cf: port[0x0f] = c; return; //registre controle action - musique
   } 
   break;
  case 0xb: case 0xc: case 0xd: case 0xe:
   if(carflags & 8) if(cartype == 0) rombank[a] = c; break;
  case 0xf: break;
  default: ramuser[a] = c;
 }
}  

// Lecture memoire MO5 ///////////////////////////////////////////////////////
char MgetMO5(unsigned short a)
{
 switch(a >> 12)
 {
  case 0x0: case 0x1: return ramvideo[a];
  case 0xa: switch(a)
   {
    case 0xa7c0: return port[0] | 0x80 | (penbutton << 5);
    case 0xa7c1: return port[1] | touche[(port[1] & 0xfe)>> 1];
    case 0xa7c2: return port[2];
    case 0xa7c3: return port[3] | ~Initn();
    case 0xa7cb: return (carflags&0x3f)|((carflags&0x80)>>1)|((carflags&0x40)<<1);
    case 0xa7cc: return((port[0x0e] & 4) ? joysposition : port[0x0c]);
    case 0xa7cd: return((port[0x0f] & 4) ? joysaction | sound : port[0x0d]);
    case 0xa7ce: return 4;
    case 0xa7d8: return ~Initn(); //octet etat disquette
    case 0xa7e1: return 0xff; //zero provoque erreur 53 sur imprimante
    case 0xa7e6: return Iniln() << 1;
    case 0xa7e7: return Initn();
    default: if(a < 0xa7c0) return(cd90640rom[a & 0x7ff]);
             if(a < 0xa800) return(port[a & 0x3f]); 
             return(0);
   }
  case 0xb: Switchmemo5bank(a); return rombank[a];
  case 0xc: case 0xd: case 0xe: return rombank[a];
  case 0xf: return romsys[a];
  default: return ramuser[a];
 }
}

//LUDO: keyboard and joystick 
void
dcmo5_key_press( int key_id )
{
  touche[key_id] = 0x00;
}

void
dcmo5_key_release( int key_id )
{
  touche[key_id] = 0x80;
}

void
dcmo5_joy_press( int joy_id )
{
  if (joy_id <= 7) {
    joysposition &= (1 << joy_id);
  } else {
    if (joy_id == 8) joysaction &= ~0x40;
    else             joysaction &= ~0x80;
  }
}

void
dcmo5_joy_release( int joy_id )
{
  if (joy_id <= 7) {
    joysposition |= (1 << joy_id);
  } else {
    if (joy_id == 8) joysaction |= 0x40;
    else             joysaction |= 0x80;
  }
}

void
dcmo5_reset_keyboard()
{
  int i;
  for(i = 0; i < 58; i++) touche[i] = 0x80; //touches relach�es
  joysposition = 0xff;                      //manettes au centre
  joysaction = 0xc0;                        //boutons relach�s
}

#define STZ_SAVE(A) gzwrite(a_file, &(A), sizeof((A)))
#define STZ_READ(A) gzread(a_file, &(A), sizeof((A)))

int
dcmo5_save_state(char *filename)
{
  gzFile* a_file = gzopen( filename, "w");
  if (a_file) {
    STZ_SAVE(CC);
    STZ_SAVE(PC);
    STZ_SAVE(D);
    STZ_SAVE(X);
    STZ_SAVE(Y);
    STZ_SAVE(U);
    STZ_SAVE(S);
    STZ_SAVE(DA);
    STZ_SAVE(W);
    STZ_SAVE(touche);
    STZ_SAVE(joysposition);
    STZ_SAVE(joysaction);
    STZ_SAVE(carflags);

    STZ_SAVE(car);
    STZ_SAVE(ram);
    STZ_SAVE(port);

    STZ_SAVE(carflags);
    STZ_SAVE(cartype);
    STZ_SAVE(xpen);
    STZ_SAVE(ypen);
    STZ_SAVE(videolinecycle);
    STZ_SAVE(soundcycle);
    STZ_SAVE(videolinenumber);
    STZ_SAVE(bordercolor);
    STZ_SAVE(opcycles);
    STZ_SAVE(sound);

    gzclose(a_file);
    return 1;
  }
  return 0;
  
}

int
dcmo5_load_state(char *filename)
{
  gzFile* a_file = gzopen( filename, "r");
  if (a_file) {
    STZ_READ(CC);
    STZ_READ(PC);
    STZ_READ(D);
    STZ_READ(X);
    STZ_READ(Y);
    STZ_READ(U);
    STZ_READ(S);
    STZ_READ(DA);
    STZ_READ(W);
    STZ_READ(touche);
    STZ_READ(joysposition);
    STZ_READ(joysaction);
    STZ_READ(carflags);

    STZ_READ(car);
    STZ_READ(ram);
    STZ_READ(port);

    STZ_READ(carflags);
    STZ_READ(cartype);
    STZ_READ(xpen);
    STZ_READ(ypen);
    STZ_READ(videolinecycle);
    STZ_READ(soundcycle);
    STZ_READ(videolinenumber);
    STZ_READ(bordercolor);
    STZ_READ(opcycles);
    STZ_READ(sound);

  //char *ramvideo;      //pointeur couleurs ou formes
    MO5videoram();
  //char *ramuser;       //pointeur ram utilisateur fixe
    ramuser = ram + 0x2000;
  //char *romsys;        //pointeur rom systeme
    romsys = mo5rom - 0xc000;
  //char *rombank;       //pointeur banque rom ou cartouche
    MO5rombank();

    gzclose(a_file);
    return 1;
  }
  return 0;
}
