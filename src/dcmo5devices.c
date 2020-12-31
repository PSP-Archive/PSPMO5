//////////////////////////////////////////////////////////////////////////////
// DCMO5DEVICES.C   Emulation des peripheriques MO5
// Author   : Daniel Coulom - danielcoulom@gmail.com
// Web site : http://dcmo5.free.fr
// Created  : January 2005
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
//////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>  

// Variable globales /////////////////////////////////////////////////////////
DIR *dmemo = NULL;  // pointeur directory pour recherche memo5 
FILE *ffd = NULL;   // pointeur fichier disquette
FILE *fk7 = NULL;   // pointeur fichier k7
FILE *fprn = NULL;  // pointeur fichier imprimante
int k7bit = 0;      // puissance de 2 designant le bit k7 en cours
int k7octet;        // octet de la cassette en cours de traitement
int k7index;        // compteur du lecteur de cassette
int k7indexmax;     // compteur du lecteur de cassette en fin de bande
int k7protection;   // indicateur lecture seule pour la cassette
int fdprotection;   // indicateur lecture seule pour la disquette
# if 0 //LUDO:
char k7name[100];   // nom du fichier cassette    
char fdname[100];   // nom du fichier disquette    
char memoname[100]; // nom du fichier cartouche
# endif

//pointeurs vers fonctions d'acces memoire
# if 0 //LUDO:
extern char (*Mgetc)(unsigned short a);
extern void (*Mputc)(unsigned short a, char c);
# else
# define Mgetc(A)   MgetMO5((A))
# define Mputc(A,V)   MputMO5((A), (V))
# endif
extern void Initprog();

 extern int carflags, cartype;
 extern char ram[], car[];

//6809 registers
extern int CC;
extern short D, S;
extern char *Ap, *Bp;

// Emulation imprimante //////////////////////////////////////////////////////
void Imprime()
{
 if(fprn == NULL) fprn = fopen("dcmo5-printer.txt", "ab");
 if(fprn != NULL) {fputc(*Bp, fprn); CC &= 0xfe;};
}

// Erreur lecture/ecriture fichier qd ou fd //////////////////////////////////
void Diskerror(int n)
{
 Mputc(0x204e, n - 1); //erreur 53 = erreur entree/sortie 
 CC |= 0x01; //indicateur d'erreur
 return;
}

// Lecture d'un secteur /////////////////////////////////////////////////////
void Readsector()
{
 char buffer[256];
 int i, j, u, p, s;
 //if(controller == 0) Warning(M_DSK_NOTSELECTED);
 //erreur 71=lecteur non prêt
 if(ffd == NULL) {Diskerror(71); return;}
 u = Mgetc(0x2049) & 0xff; if(u > 03) {Diskerror(53); return;}
 p = Mgetc(0x204a) & 0xff; if(p != 0) {Diskerror(53); return;}
 p = Mgetc(0x204b) & 0xff; if(p > 79) {Diskerror(53); return;}
 s = Mgetc(0x204c) & 0xff; if((s == 0) || (s > 16)) {Diskerror(53); return;}
 s += 16 * p + 1280 * u;
 fseek(ffd, 0, SEEK_END);
 if((s << 8) > ftell(ffd)) {Diskerror(53); return;}
 for(j = 0; j < 256; j++) buffer[j] = 0xe5;
 fseek(ffd, (s - 1) << 8, SEEK_SET);
 i = ((Mgetc(0x204f) & 0xff) << 8) + (Mgetc(0x2050) & 0xff);
 if(fread(buffer, 256, 1, ffd) == 0) {Diskerror(53); return;}  
 for(j = 0; j < 256; j++) Mputc(i++, buffer[j]);
}

// Ecriture d'un secteur /////////////////////////////////////////////////////
void Writesector()
{
 char buffer[256];
 int i, j, u, p, s;
 //if(controller == 0) Warning(M_DSK_NOTSELECTED);
 //erreur 71 = lecteur non prêt
 if(ffd == NULL) {Diskerror(71); return;}
 //erreur 72 = protection ecriture
 if(fdprotection == 1) {Diskerror(72); return;}
 u = Mgetc(0x2049) & 0xff; if(u > 03) {Diskerror(53); return;}
 p = Mgetc(0x204a) & 0xff; if(p != 0) {Diskerror(53); return;}
 p = Mgetc(0x204b) & 0xff; if(p > 79) {Diskerror(53); return;}
 s = Mgetc(0x204c) & 0xff; if((s == 0) || (s > 16)) {Diskerror(53); return;}
 s += 16 * p + 1280 * u;
 fseek(ffd, (s - 1) << 8, SEEK_SET);
 i = 256 * (Mgetc(0x204f) & 0xff) + (Mgetc(0x2050) & 0xff);
 for(j = 0; j < 256; j++) buffer[j] = Mgetc(i++);
 if(fwrite(buffer, 256, 1, ffd) == 0) Diskerror(53);
}

// Formatage d'un disque ////////////////////////////////////////////////////
void Formatdisk()
{
 char buffer[256];
 int i, u, fatlength;
 //if(controller == 0) Warning(M_DSK_NOTSELECTED);
 //erreur 71 = lecteur non prêt
 if(ffd == NULL) {Diskerror(71); return;}
 //erreur 72 = protection ecriture
 if(fdprotection == 1) {Diskerror(72); return;}
 u = Mgetc(0x2049) & 0xff; if(u > 03) return; //unite
 u = (1280 * u) << 8; //debut de l'unité dans le fichier .fd
 fatlength = 80;
 //rem: fatlength provisoire !!!!! (tester la variable adéquate)
 //initialisation de tout le disque avec E5
 for(i = 0; i < 256; i++) buffer[i] = 0xe5;
 fseek(ffd, u, SEEK_SET);
 for(i = 0; i < (fatlength * 8); i++)
  if(fwrite(buffer, 256, 1, ffd) == 0) {Diskerror(53); return;}
 //initialisation de la piste 20 a FF
 for(i = 0; i < 256; i++) buffer[i] = 0xff;
 fseek(ffd, u + 0x14000, SEEK_SET);
 for(i = 0; i < 16; i++)
  if(fwrite(buffer, 256, 1, ffd) == 0) {Diskerror(53); return;}
 //ecriture de la FAT
 buffer[0x00] = 0;
 buffer[0x29] = 0xfe; buffer[0x2a] = 0xfe;
 for(i = fatlength + 1; i < 256; i++) buffer[i] = 0xfe;
 fseek(ffd, u + 0x14100, SEEK_SET);
 if(fwrite(buffer, 256, 1, ffd) == 0) {Diskerror(53); return;}
}

int 
Loadfd(char *filename)
{
  if(ffd) {
    fclose(ffd); ffd = NULL;
  } //fermeture disquette éventuellement ouverte
  ffd = fopen(filename, "rb+");
  if(ffd == NULL) return 0;
  //fdprotection = 1;
  return 0;
}

// Emulation cassette ////////////////////////////////////////////////////////
int 
Readoctetk7()
{
 int byte = 0;
 extern void Initprog();
 if(fk7 == NULL) {Initprog(); return -1;}
 byte = fgetc(fk7);
 if(byte == EOF) {
  Initprog();
  fseek(fk7, 0, SEEK_SET); k7index = 0; 
  return -1;
 }
 *Ap = k7octet = byte; Mputc(0x2045, byte); k7bit = 0;
 if((ftell(fk7) & 511) == 0) { 
   k7index = ftell(fk7) >> 9; 
 } 
  return 0;
}

void Readbitk7()
{
 int octet = Mgetc(0x2045) << 1;
 if(k7bit == 0) {Readoctetk7(); k7bit = 0x80;}
 if((k7octet & k7bit)) {octet |= 0x01; *Ap = 0xff;} else *Ap = 0;
 Mputc(0x2045, octet); k7bit >>= 1;
}

int 
Writeoctetk7()
{
 extern void Initprog();
 if(fk7 == NULL) {Initprog(); return -1; }
 if(k7protection) {Initprog(); return -1; }
 if(fputc(*Ap, fk7) == EOF) {Initprog(); return -1; }
 Mputc(0x2045, 0); 
 if((ftell(fk7) & 511) == 0) {
   k7index = ftell(fk7) >> 9; 
 } 
 return 0;
}

int 
Loadk7(char *filename)
{
 if (fk7) {
   fclose(fk7); fk7 = NULL;
 } //fermeture cassette éventuellement ouverte
 fk7 = fopen(filename, "rb+");
 if(fk7 == NULL) return 0;
 k7index = 0;
 fseek(fk7, 0, SEEK_END);   
 k7indexmax = ftell(fk7) >> 9;
 fseek(fk7, 0, SEEK_SET);   
 //k7protection = 1;
 return 0;
}

void 
Rewindk7()
{
 k7index = 0;
 if(fk7 == NULL) return;
 fseek(fk7, 0, SEEK_SET);
}

void
Ejectk7()
{
 if (fk7) {
   fclose(fk7); fk7 = NULL;
 } //fermeture cassette éventuellement ouverte
}

void
Ejectfd()
{
  if(ffd) {
    fclose(ffd); ffd = NULL;
  } //fermeture disquette éventuellement ouverte
}

int
GetK7Index()
{
  return k7index;
}

int
Ejectmemo()
{
  carflags = 0; Initprog();
  return 0;
}

// Emulation cartouche memo5 /////////////////////////////////////////////////
int 
Loadmemo(char *filename)
{
 FILE *fp = NULL;    
 int i, c, carsize;
 //ouverture du fichier memo5
 fp = fopen(filename, "rb+");
# if 0 
 if(fp == NULL) {memoname[0] = 0; carflags = 0; Initprog(); return;}
# else
  if (fp == NULL) return -1;
# endif
 //chargement
 carsize = 0;
 while(((c = fgetc(fp)) != EOF) && (carsize < 0x10000)) car[carsize++] = c;
 fclose(fp);
 for(i = 0; i < 0xc000; i++) ram[i] = -((i & 0x80) >> 7);
 cartype = 0; //cartouche <= 16 Ko
 if(carsize > 0x4000) cartype = 1;   //bank switch system
 carflags = 4; //cartridge enabled, write disabled, bank 0; 
 Initprog();   //initialisation programme pour lancer la cassette
 return 0;
}

int 
Loadmemo_buffer(char *rom_buffer, int rom_size)
{
 int i, c, carsize;
 //chargement
 carsize = 0;
 i = 0;
 while((i < rom_size) && (carsize < 0x10000)) car[carsize++] = rom_buffer[i++];
 for(i = 0; i < 0xc000; i++) ram[i] = -((i & 0x80) >> 7);
 cartype = 0; //cartouche <= 16 Ko
 if(carsize > 0x4000) cartype = 1;   //bank switch system
 carflags = 4; //cartridge enabled, write disabled, bank 0; 
 Initprog();   //initialisation programme pour lancer la cassette
 return 0;
}

// Emulation crayon optique //////////////////////////////////////////////////
void Readpenxy()
{
 extern int CC, xpen, ypen;
 extern void Mputw(unsigned short a, short w);
 if((xpen < 0) || (xpen >= 320)) {CC |= 1; return;}
 if((ypen < 0) || (ypen >= 200)) {CC |= 1; return;}
 Mputw(S + 6, xpen); Mputw(S + 8, ypen); CC &= 0xfe;
 return;
}

// Initialisation noms de fichiers et pointeur de fonction de chargement //////
void Initfilenames(char c)
{
# if 0 //LUDO:
 extern void (*Load[]);
 k7name[0] = 0;    
 fdname[0] = 0;    
 memoname[0] = 0;    
# endif
# if 0 //LUDO:
 Load[0] = Loadk7; 
 Load[1] = Loadfd; 
 Load[2] = Loadmemo; 
# endif
}     
