//////////////////////////////////////////////////////////////////////////
// DC6809EMUL.C - Motorola 6809 micropocessor emulation
// Author   : Daniel Coulom - danielcoulom@gmail.com
// Web site : http://dcmo5.free.fr
// Created  : 1997-08
// Version  : 2006-10-25
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
//////////////////////////////////////////////////////////////////////////

//global variables
int n;                           //cycle count
int CC = 0x10;                   //condition code
short PC, D, X, Y, U, S, DA, W;  //6809 two byte registers + W work register
char *PCHp, *PCLp, *Ap, *Bp;     //pointers to register bytes
char *XHp, *XLp, *YHp, *YLp;     //pointers to register bytes
char *UHp, *ULp, *SHp, *SLp;     //pointers to register bytes
char *DPp, *DDp, *WHp, *WLp;     //pointers to register bytes
#define PCH *PCHp
#define PCL *PCLp
#define A   *Ap
#define B   *Bp
#define XH  *XHp
#define XL  *XLp
#define YH  *YHp
#define YL  *YLp
#define UH  *UHp
#define UL  *ULp
#define SH  *SHp
#define SL  *SLp
#define DP  *DPp
#define DD  *DDp
#define WH  *WHp
#define WL  *WLp

/*condition code masks (CC=EFHINZVC)*/
#define  C1 0x01  /* carry */
#define  V1 0x02  /* overflow */
#define  Z1 0x04  /* zero */
#define  Z0 0xfb  /* ~Z1 */
#define  N1 0x08  /* negative */
#define  I1 0x10  /* irq mask */
#define  H1 0x20  /* half-carry */
#define  F1 0x40  /* firq mask */
#define  E1 0x80  /* extended registers save */
#define  NV 0x0a  /* negative | overflow */
#define  ZC 0x05  /* zero | carry */
#define NZC 0x0e  /* negative | zero | carry */

/*
conditional jump summary

C BCC BCS      Z BNE BEQ      V BVC BVS     NZV BGT BLE
0 yes no       0 yes no       0 yes no      000 yes no
1 no  yes      1 no  yes      1 no  yes     100 no  yes
                                            010 no  yes
N BPL BMI     ZC BHI BLS     NV BGE BLT     001 no  yes
0 yes no      00 yes no      00 yes no      110 no  no
1 no  yes     10 no  yes     10 no  yes     101 yes no
              01 no  yes     01 no  yes     011 no  no
              11 no  no      11 yes no      111 no  yes
*/
#define BHI (CC&ZC)==0
#define BLS (CC&ZC)==Z1||(CC&ZC)==C1
#define BCC (CC&C1)==0  // BCC = BHS
#define BCS (CC&C1)==C1 // BCS = BLO
#define BNE (CC&Z1)==0
#define BEQ (CC&Z1)==Z1
#define BVC (CC&V1)==0
#define BVS (CC&V1)==V1
#define BPL (CC&N1)==0
#define BMI (CC&N1)==N1
#define BGE (CC&NV)==0||(CC&NV)==NV
#define BLT (CC&NV)==N1||(CC&NV)==V1
#define BGT (CC&NZC)==0||(CC&0x0e)==NV
#define BLE (CC&NZC)==N1||(CC&NZC)==Z1||(CC&NZC)==V1||(CC&NZC)==NZC
#define BRANCH {PC+=Mgetc(PC);}
#define LBRANCH {PC+=Mgetw(PC);n++;}

//repetitive code
#define IND Mgeti()
#define DIR DD=Mgetc(PC++)
#define EXT W=Mgetw(PC);PC+=2
#define SETZERO {if(W) CC &= Z0; else CC |= Z1;}

//external functions (or pointers to functions)
//memory is accessed through :
//Mgetc : reads one byte from address a
//Mgetw : reads two bytes from address a
//Mputc : writes one byte to address a
//Mputw : writes two bytes to address a
extern short Mgetw(unsigned short);         //function
extern void  Mputw(unsigned short, short);  //function
# if 0 //LUDO:
extern char (*Mgetc)(unsigned short a);
extern void (*Mputc)(unsigned short a, char c);
# else
# define Mgetc(A)   MgetMO5((A))
# define Mputc(A,V)   MputMO5((A), (V))
# endif

// Init pointers to register bytes ///////////////////////////////////////////
void Init6809registerpointers()
{
 int i = 1;                //integer used to test endianness
 char *c = (char*)&i;      //left byte of integer i  

 PCHp = PCLp = (char*)&PC; //PC msb & lsb
 Ap   = Bp   = (char*)&D;  // D msb & lsb
 XHp  = XLp  = (char*)&X;  // X msb & lsb
 YHp  = YLp  = (char*)&Y;  // Y msb & lsb
 UHp  = ULp  = (char*)&U;  // U msb & lsb
 SHp  = SLp  = (char*)&S;  // S msb & lsb
 DPp  = DDp  = (char*)&DA; //DP msb & lsb
 WHp  = WLp  = (char*)&W;  // W msb & lsb

 switch(c[0]) //test endianness: 1=little-endian, 0=big-endian
 {
  case 1:  PCHp++; Ap++; XHp++; YHp++; UHp++; SHp++; DPp++; WHp++; break;
  default: PCLp++; Bp++; XLp++; YLp++; ULp++; SLp++; DDp++; WLp++; break;
 }                         
}
  
// Get memory (indexed) //////////////////////////////////////////////////////
void Mgeti()
{
 int i;
 short *r;
 i = Mgetc(PC++);
 switch (i & 0x60)
 {
  case 0x00: r = &X; break;
  case 0x20: r = &Y; break;
  case 0x40: r = &U; break;
  case 0x60: r = &S; break;
  default: r = &X;
 }
 switch(i &= 0x9f)
 {
  case 0x80: n = 2; W = *r; *r += 1; return;                    // ,R+       
  case 0x81: n = 3; W = *r; *r += 2; return;                    // ,R++      
  case 0x82: n = 2; *r -= 1; W = *r; return;                    // ,-R       
  case 0x83: n = 3; *r -= 2; W = *r; return;                    // ,--R      
  case 0x84: n = 0; W = *r; return;                             // ,R        
  case 0x85: n = 1; W = *r + B; return;                         // B,R       
  case 0x86: n = 1; W = *r + A; return;                         // A,R       
  case 0x87: n = 0; W = *r; return;                             // invalid       
  case 0x88: n = 1; W = *r + Mgetc(PC++); return;               // char,R    
  case 0x89: n = 4; EXT; W += *r; return;                       // word,R    
  case 0x8a: n = 0; W = *r; return;                             // invalid       
  case 0x8b: n = 4; W = *r + D; return;                         // D,R       
  case 0x8c: n = 1; W = Mgetc(PC++); W += PC; return;           // char,PCR  
  case 0x8d: n = 5; EXT; W += PC; return;                       // word,PCR  
  case 0x8e: n = 0; W = *r; return;                             // invalid       
  case 0x8f: n = 0; W = *r; return;                             // invalid       
  case 0x90: n = 3; W = Mgetw(*r); return;                      // invalid       
  case 0x91: n = 6; *r += 2; W = Mgetw(*r - 2); return;         // [,R++]    
  case 0x92: n = 3; W = Mgetw(*r); return;                      // invalid       
  case 0x93: n = 6; *r -= 2; W = Mgetw(*r); return;             // [,--R]    
  case 0x94: n = 3; W = Mgetw(*r); return;                      // [,R]      
  case 0x95: n = 4; W = Mgetw(*r + B); return;                  // [B,R]     
  case 0x96: n = 4; W = Mgetw(*r + A); return;                  // [A,R]     
  case 0x97: n = 3; W = Mgetw(*r); return;                      // invalid       
  case 0x98: n = 4; W = Mgetw(*r + Mgetc(PC++)); return;        // [char,R]  
  case 0x99: n = 7; EXT; W = Mgetw(*r + W); return;             // [word,R]  
  case 0x9a: n = 3; W = Mgetw(*r); return;                      // invalid       
  case 0x9b: n = 7; W = Mgetw(*r + D); return;                  // [D,R]     
  case 0x9c: n = 4; W = Mgetw(PC+1+Mgetc(PC)); PC++; return;    // [char,PCR]
  case 0x9d: n = 8; EXT; W = Mgetw(PC + W); return;             // [word,PCR]
  case 0x9e: n = 3; W = Mgetw(*r); return;                      // invalid       
  case 0x9f: n = 5; EXT; W = Mgetw(W); return;                  // [word]    
  default  : n = 1; if(i & 0x10) i -= 0x20; W = *r + i; return; // 5 bits,R  
  //Assumes 0x84 for invalid bytes 0x87 0x8a 0x8e 0x8f
  //Assumes 0x94 for invalid bytes 0x90 0x92 0x97 0x9a 0x9e
 }
}

// PSH, PUL, EXG, TFR /////////////////////////////////////////////////////////
void Pshs(char c)
{
 //Pshs(0xff) = 12 cycles   
 //Pshs(0xfe) = 11 cycles   
 //Pshs(0x80) = 2 cycles   
 if(c & 0x80) {Mputc(--S,PCL); Mputc(--S,PCH); n += 2;}
 if(c & 0x40) {Mputc(--S, UL); Mputc(--S, UH); n += 2;}
 if(c & 0x20) {Mputc(--S, YL); Mputc(--S, YH); n += 2;}
 if(c & 0x10) {Mputc(--S, XL); Mputc(--S, XH); n += 2;}
 if(c & 0x08) {Mputc(--S, DP); n += 1;}
 if(c & 0x04) {Mputc(--S,  B); n += 1;}
 if(c & 0x02) {Mputc(--S,  A); n += 1;}
 if(c & 0x01) {Mputc(--S, CC); n += 1;}
}

void Pshu(char c)
{
 if(c & 0x80) {Mputc(--U,PCL); Mputc(--U,PCH); n += 2;}
 if(c & 0x40) {Mputc(--U, SL); Mputc(--U, SH); n += 2;}
 if(c & 0x20) {Mputc(--U, YL); Mputc(--U, YH); n += 2;}
 if(c & 0x10) {Mputc(--U, XL); Mputc(--U, XH); n += 2;}
 if(c & 0x08) {Mputc(--U, DP); n += 1;}
 if(c & 0x04) {Mputc(--U,  B); n += 1;}
 if(c & 0x02) {Mputc(--U,  A); n += 1;}
 if(c & 0x01) {Mputc(--U, CC); n += 1;}
}

void Puls(char c)
{
 if(c & 0x01) {CC = Mgetc(S); S++; n += 1;}
 if(c & 0x02) { A = Mgetc(S); S++; n += 1;}
 if(c & 0x04) { B = Mgetc(S); S++; n += 1;}
 if(c & 0x08) {DP = Mgetc(S); S++; n += 1;}
 if(c & 0x10) {XH = Mgetc(S); S++; XL = Mgetc(S); S++; n += 2;}
 if(c & 0x20) {YH = Mgetc(S); S++; YL = Mgetc(S); S++; n += 2;}
 if(c & 0x40) {UH = Mgetc(S); S++; UL = Mgetc(S); S++; n += 2;}
 if(c & 0x80) {PCH= Mgetc(S); S++; PCL= Mgetc(S); S++; n += 2;}
}

void Pulu(char c)
{
 if(c & 0x01) {CC = Mgetc(U); U++; n += 1;}
 if(c & 0x02) { A = Mgetc(U); U++; n += 1;}
 if(c & 0x04) { B = Mgetc(U); U++; n += 1;}
 if(c & 0x08) {DP = Mgetc(U); U++; n += 1;}
 if(c & 0x10) {XH = Mgetc(U); U++; XL = Mgetc(U); U++; n += 2;}
 if(c & 0x20) {YH = Mgetc(U); U++; YL = Mgetc(U); U++; n += 2;}
 if(c & 0x40) {SH = Mgetc(U); U++; SL = Mgetc(U); U++; n += 2;}
 if(c & 0x80) {PCH= Mgetc(U); U++; PCL= Mgetc(U); U++; n += 2;}
}

void Exg(char c)
{
 switch(c & 0xff)
 {
  case 0x01: W = D; D = X; X = W; return;    //D-X
  case 0x02: W = D; D = Y; Y = W; return;    //D-Y
  case 0x03: W = D; D = U; U = W; return;    //D-U
  case 0x04: W = D; D = S; S = W; return;    //D-S
  case 0x05: W = D; D = PC; PC = W; return;  //D-PC
  case 0x10: W = X; X = D; D = W; return;    //X-D
  case 0x12: W = X; X = Y; Y = W; return;    //X-Y
  case 0x13: W = X; X = U; U = W; return;    //X-U
  case 0x14: W = X; X = S; S = W; return;    //X-S
  case 0x15: W = X; X = PC; PC = W; return;  //X-PC
  case 0x20: W = Y; Y = D; D = W; return;    //Y-D
  case 0x21: W = Y; Y = X; X = W; return;    //Y-X
  case 0x23: W = Y; Y = U; U = W; return;    //Y-U
  case 0x24: W = Y; Y = S; S = W; return;    //Y-S
  case 0x25: W = Y; Y = PC; PC = W; return;  //Y-PC
  case 0x30: W = U; U = D; D = W; return;    //U-D
  case 0x31: W = U; U = X; X = W; return;    //U-X
  case 0x32: W = U; U = Y; Y = W; return;    //U-Y
  case 0x34: W = U; U = S; S = W; return;    //U-S
  case 0x35: W = U; U = PC; PC = W; return;  //U-PC
  case 0x40: W = S; S = D; D = W; return; 
  case 0x41: W = S; S = X; X = W; return; 
  case 0x42: W = S; S = Y; Y = W; return; 
  case 0x43: W = S; S = U; U = W; return; 
  case 0x45: W = S; S = PC; PC = W; return; 
  case 0x50: W = PC; PC = D; D = W; return; 
  case 0x51: W = PC; PC = X; X = W; return; 
  case 0x52: W = PC; PC = Y; Y = W; return; 
  case 0x53: W = PC; PC = U; U = W; return; 
  case 0x54: W = PC; PC = S; S = W; return; 
  case 0x89: W = A; A = B; B = W; return; 
  case 0x8a: W = A; A = CC; CC = W; return; 
  case 0x8b: W = A; A = DP; DP = W; return; 
  case 0x98: W = B; B = A; A = W; return; 
  case 0x9a: W = B; B = CC; CC = W; return; 
  case 0x9b: W = B; B = DP; DP = W; return; 
  case 0xa8: W = CC; CC = A; A = W; return; 
  case 0xa9: W = CC; CC = B; B = W; return; 
  case 0xab: W = CC; CC = DP; DP = W; return; 
  case 0xb8: W = DP; DP = A; A = W; return; 
  case 0xb9: W = DP; DP = B; B = W; return; 
  case 0xba: W = DP; DP = CC; CC = W; return; 
 }
}

void Tfr(char c)
{
 switch(c & 0xff)
 {
  case 0x01: X = D; return; 
  case 0x02: Y = D; return; 
  case 0x03: U = D; return; 
  case 0x04: S = D; return; 
  case 0x05: PC = D; return; 
  case 0x10: D = X; return; 
  case 0x12: Y = X; return; 
  case 0x13: U = X; return; 
  case 0x14: S = X; return; 
  case 0x15: PC = X; return; 
  case 0x20: D = Y; return; 
  case 0x21: X = Y; return; 
  case 0x23: U = Y; return; 
  case 0x24: S = Y; return; 
  case 0x25: PC = Y; return; 
  case 0x30: D = U; return; 
  case 0x31: X = U; return; 
  case 0x32: Y = U; return; 
  case 0x34: S = U; return; 
  case 0x35: PC = U; return; 
  case 0x40: D = S; return; 
  case 0x41: X = S; return; 
  case 0x42: Y = S; return; 
  case 0x43: U = S; return; 
  case 0x45: PC = S; return; 
  case 0x50: D = PC; return; 
  case 0x51: X = PC; return; 
  case 0x52: Y = PC; return; 
  case 0x53: U = PC; return; 
  case 0x54: S = PC; return; 
  case 0x89: B = A; return; 
  case 0x8a: CC = A; return; 
  case 0x8b: DP = A; return; 
  case 0x98: A = B; return; 
  case 0x9a: CC = B; return; 
  case 0x9b: DP = B; return; 
  case 0xa8: A = CC; return; 
  case 0xa9: B = CC; return; 
  case 0xab: DP = CC; return; 
  case 0xb8: A = DP; return; 
  case 0xb9: B = DP; return; 
  case 0xba: CC = DP; return; 
 }
}

// CLR, NEG, COM, INC, DEC  (CC=EFHINZVC) /////////////////////////////////////
char Clr()
{
 CC &= 0xf0;
 CC |= Z1;
 return 0;
}

char Neg(char c)
{
 CC &= 0xf0;
 if(c == -128) CC |= V1;
 c = - c;
 if(c != 0) CC |= C1;
 if(c < 0) CC |= N1;
 if(c == 0) CC |= Z1; 
 return c;
}

char Com(char c)
{
 CC &= 0xf0;
 c = ~c;
 CC |= C1;
 if(c < 0) CC |= N1;
 if(c == 0) CC |= Z1;
 return c;
}

char Inc(char c)
{
 CC &= 0xf1;
 if(c == 127) CC |= V1;
 c++;
 if(c < 0) CC |= N1;
 if(c == 0) CC |= Z1;
 return c;
}

char Dec(char c)
{
 CC &= 0xf1;
 if(c == -128) CC |= V1;
 c--;
 if(c < 0) CC |= N1;
 if(c == 0) CC |= Z1;
 return c;
}

// Registers operations  (CC=EFHINZVC) ////////////////////////////////////////
void Mul()
{
 D = (A & 0xff) * (B & 0xff);
 CC &= 0xf2;
 if(D < 0) CC |= C1;
 if(D == 0) CC |= Z1;
}

void Addc(char *r, char c)
{
 int i = *r + c;
 CC &= 0xd0;
 if(((*r & 0x0f) + (c & 0x0f)) & 0x10) CC |= H1;
 if(((*r & 0xff) + (c & 0xff)) & 0x100) CC |= C1;
 *r = i & 0xff;
 if(*r != i) CC |= V1;
 if(*r < 0) CC |= N1;
 if(*r == 0) CC |= Z1;
}

void Adc(char *r, char c)
{
 int carry = (CC & C1);
 int i = *r + c + carry;
 CC &= 0xd0;
 if(((*r & 0x0f) + (c & 0x0f) + carry) & 0x10) CC |= H1;
 if(((*r & 0xff) + (c & 0xff) + carry) & 0x100) CC |= C1;
 *r = i & 0xff;
 if(*r != i) CC |= V1;
 if(*r < 0) CC |= N1;
 if(*r == 0) CC |= Z1;
}

void Addw(short *r, short w)
{
 int i = *r + w;
 CC &= 0xf0;
 if(((*r & 0xffff) + (w & 0xffff)) & 0xf0000) CC |= C1;
 *r = i & 0xffff;
 if(*r != i) CC |= V1;
 if(*r < 0) CC |= N1;
 if(*r == 0) CC |= Z1;
}

void Subc(char *r, char c)
{
 int i = *r - c;
 CC &= 0xf0;
 if(((*r & 0xff) - (c & 0xff)) & 0x100) CC |= C1;
 *r = i & 0xff;
 if(*r != i) CC |= V1;
 if(*r < 0) CC |= N1;
 if(*r == 0) CC |= Z1;
}

void Sbc(char *r, char c)
{
 int carry = (CC & C1);
 int i = *r - c - carry;
 CC &= 0xf0;
 if(((*r & 0xff) - (c & 0xff) - carry) & 0x100) CC |= C1;
 *r = i & 0xff;
 if(*r != i) CC |= V1;
 if(*r < 0) CC |= N1;
 if(*r == 0) CC |= Z1;
}

void Subw(short *r, short w)
{
 int i = *r - w;
 CC &= 0xf0;
 if(((*r & 0xffff) - (w & 0xffff)) & 0x10000) CC |= C1;
 *r = i & 0xffff;
 if(*r != i) CC |= V1;
 if(*r < 0) CC |= N1;
 if(*r == 0) CC |= Z1;
}

void Daa()
{
 int i = A & 0xff;
 if((CC & H1) || ((i & 0x00f) > 0x09)) i += 0x06;
 if((CC & C1) || ((i & 0x1f0) > 0x90)) i += 0x60;
 A = i & 0xff;
 i = (i >> 1 & 0xff) | (CC << 7);
 CC &= 0xf0;
 if(i & 0x80) CC |= C1;
 if((A ^ i) & 0x80) CC |= V1;
 if(A < 0) CC |= N1;
 if(A == 0) CC |= Z1;
}

// Shift and rotate  (CC=EFHINZVC) ////////////////////////////////////////////
char Lsr(char c)
{
 CC &= 0xf2;
 if(c & 1) CC |= C1;
 c = (c & 0xff) >> 1; 
 if(c == 0) CC |= Z1;
 return c;
}

char Ror(char c)
{
 int carry = CC & C1; 
 CC &= 0xf2;
 if(c & 1) CC |= C1;
 c = ((c & 0xff) >> 1) | (carry << 7);
 if(c < 0) CC |= N1;
 if(c == 0) CC |= Z1;
 return c;
}

char Rol(char c)
{
 int carry = CC & C1;
 CC &= 0xf0;
 if(c < 0) CC |= C1;
 c = ((c & 0x7f) << 1) | carry;
 if((c >> 7 & 1) ^ (CC & C1)) CC |= V1;
 if(c < 0) CC |= N1;
 if(c == 0) CC |= Z1;
 return c;
}

char Asr(char c)
{
 CC &= 0xf2;
 if(c & 1) CC |= C1;
 c = ((c & 0xff) >> 1) | (c & 0x80);
 if(c < 0) CC |= N1;
 if(c == 0) CC |= Z1;
 return c;
}

char Asl(char c)
{
 CC &= 0xf0;
 if(c < 0) CC |= C1;
 c = (c & 0xff) << 1;
 if((c >> 7 & 1) ^ (CC & C1)) CC |= V1;
 if(c < 0) CC |= N1;
 if(c == 0) CC |= Z1;
 return c;
}

// Test and compare  (CC=EFHINZVC) ////////////////////////////////////////////
void Tstc(char c)
{
 CC &= 0xf1;
 if(c < 0) CC |= N1;
 if(c == 0) CC |= Z1;
}

void Tstw(short w)
{
 CC &= 0xf1;
 if(w < 0) CC |= N1;
 if(w == 0) CC |= Z1;
}

void Cmpc(char *reg, char c)
{
 char r = *reg;
 int i = *reg - c;
 CC &= 0xf0;
 if(((r & 0xff) - (c & 0xff)) & 0x100) CC |= C1;
 r = i & 0xff;
 if(r != i) CC |= V1;
 if(r < 0) CC |= N1;
 if(r == 0) CC |= Z1;
}

void Cmpw(short *reg, short w)
{
 short r = *reg;
 int i = *reg - w;
 CC &= 0xf0;
 if(((r & 0xffff) - (w & 0xffff)) & 0x10000) CC |= C1;
 r = i & 0xffff;
 if(r != i) CC |= V1;
 if(r < 0) CC |= N1;
 if(r == 0) CC |= Z1;
}

// Interrupt requests  (CC=EFHINZVC) //////////////////////////////////////////
void Swi(int n)
{
 CC |= E1;
 Pshs(0xff);
 CC |= I1;
 if(n == 1) {PC = Mgetw(0xfffa); return;}
 if(n == 2) {PC = Mgetw(0xfff4); return;}
 if(n == 3) {PC = Mgetw(0xfff2); return;}
} 

void Irq()
{
 if((CC & I1) == 0)
 {CC |= E1; Pshs(0xff); CC |= I1; PC = Mgetw(0xfff8);}
}

void Firq()
{
 if((CC & F1) == 0)
 {CC &= ~E1; Pshs(0x81); CC |= F1; CC |= I1; PC = Mgetw(0xfff6);}
}

// RTI ////////////////////////////////////////////////////////////////////////
void Rti() {Puls(0x01); if(CC & E1) Puls(0xfe); else Puls(0x80);}

int Run6809_0x10()
{
 __label__ 
l_0x00, l_0x01, l_0x02, l_0x03, l_0x04, l_0x05, l_0x06, l_0x07, l_0x08, l_0x09, l_0x0a, l_0x0b, l_0x0c,
l_0x0d, l_0x0e, l_0x0f, l_0x10, l_0x11, l_0x12, l_0x13, l_0x14, l_0x15, l_0x16, l_0x17, l_0x18, l_0x19,
l_0x1a, l_0x1b, l_0x1c, l_0x1d, l_0x1e, l_0x1f, l_0x20, l_0x21, l_0x22, l_0x23, l_0x24, l_0x25, l_0x26,
l_0x27, l_0x28, l_0x29, l_0x2a, l_0x2b, l_0x2c, l_0x2d, l_0x2e, l_0x2f, l_0x30, l_0x31, l_0x32, l_0x33,
l_0x34, l_0x35, l_0x36, l_0x37, l_0x38, l_0x39, l_0x3a, l_0x3b, l_0x3c, l_0x3d, l_0x3e, l_0x3f, l_0x40,
l_0x41, l_0x42, l_0x43, l_0x44, l_0x45, l_0x46, l_0x47, l_0x48, l_0x49, l_0x4a, l_0x4b, l_0x4c, l_0x4d,
l_0x4e, l_0x4f, l_0x50, l_0x51, l_0x52, l_0x53, l_0x54, l_0x55, l_0x56, l_0x57, l_0x58, l_0x59, l_0x5a,
l_0x5b, l_0x5c, l_0x5d, l_0x5e, l_0x5f, l_0x60, l_0x61, l_0x62, l_0x63, l_0x64, l_0x65, l_0x66, l_0x67,
l_0x68, l_0x69, l_0x6a, l_0x6b, l_0x6c, l_0x6d, l_0x6e, l_0x6f, l_0x70, l_0x71, l_0x72, l_0x73, l_0x74,
l_0x75, l_0x76, l_0x77, l_0x78, l_0x79, l_0x7a, l_0x7b, l_0x7c, l_0x7d, l_0x7e, l_0x7f, l_0x80, l_0x81,
l_0x82, l_0x83, l_0x84, l_0x85, l_0x86, l_0x87, l_0x88, l_0x89, l_0x8a, l_0x8b, l_0x8c, l_0x8d, l_0x8e,
l_0x8f, l_0x90, l_0x91, l_0x92, l_0x93, l_0x94, l_0x95, l_0x96, l_0x97, l_0x98, l_0x99, l_0x9a, l_0x9b,
l_0x9c, l_0x9d, l_0x9e, l_0x9f, l_0xa0, l_0xa1, l_0xa2, l_0xa3, l_0xa4, l_0xa5, l_0xa6, l_0xa7, l_0xa8,
l_0xa9, l_0xaa, l_0xab, l_0xac, l_0xad, l_0xae, l_0xaf, l_0xb0, l_0xb1, l_0xb2, l_0xb3, l_0xb4, l_0xb5,
l_0xb6, l_0xb7, l_0xb8, l_0xb9, l_0xba, l_0xbb, l_0xbc, l_0xbd, l_0xbe, l_0xbf, l_0xc0, l_0xc1, l_0xc2,
l_0xc3, l_0xc4, l_0xc5, l_0xc6, l_0xc7, l_0xc8, l_0xc9, l_0xca, l_0xcb, l_0xcc, l_0xcd, l_0xce, l_0xcf,
l_0xd0, l_0xd1, l_0xd2, l_0xd3, l_0xd4, l_0xd5, l_0xd6, l_0xd7, l_0xd8, l_0xd9, l_0xda, l_0xdb, l_0xdc,
l_0xdd, l_0xde, l_0xdf, l_0xe0, l_0xe1, l_0xe2, l_0xe3, l_0xe4, l_0xe5, l_0xe6, l_0xe7, l_0xe8, l_0xe9,
l_0xea, l_0xeb, l_0xec, l_0xed, l_0xee, l_0xef, l_0xf0, l_0xf1, l_0xf2, l_0xf3, l_0xf4, l_0xf5, l_0xf6,
l_0xf7, l_0xf8, l_0xf9, l_0xfa, l_0xfb, l_0xfc, l_0xfd, l_0xfe, l_0xff; 
    static const void* const a_jump_table[256] = 
	  { &&
l_0x00, && l_0x01, && l_0x02, && l_0x03, && l_0x04, && l_0x05, && l_0x06, && l_0x07, && l_0x08, && l_0x09, && l_0x0a, && l_0x0b, && l_0x0c, &&
l_0x0d, && l_0x0e, && l_0x0f, && l_0x10, && l_0x11, && l_0x12, && l_0x13, && l_0x14, && l_0x15, && l_0x16, && l_0x17, && l_0x18, && l_0x19, &&
l_0x1a, && l_0x1b, && l_0x1c, && l_0x1d, && l_0x1e, && l_0x1f, && l_0x20, && l_0x21, && l_0x22, && l_0x23, && l_0x24, && l_0x25, && l_0x26, &&
l_0x27, && l_0x28, && l_0x29, && l_0x2a, && l_0x2b, && l_0x2c, && l_0x2d, && l_0x2e, && l_0x2f, && l_0x30, && l_0x31, && l_0x32, && l_0x33, &&
l_0x34, && l_0x35, && l_0x36, && l_0x37, && l_0x38, && l_0x39, && l_0x3a, && l_0x3b, && l_0x3c, && l_0x3d, && l_0x3e, && l_0x3f, && l_0x40, &&
l_0x41, && l_0x42, && l_0x43, && l_0x44, && l_0x45, && l_0x46, && l_0x47, && l_0x48, && l_0x49, && l_0x4a, && l_0x4b, && l_0x4c, && l_0x4d, &&
l_0x4e, && l_0x4f, && l_0x50, && l_0x51, && l_0x52, && l_0x53, && l_0x54, && l_0x55, && l_0x56, && l_0x57, && l_0x58, && l_0x59, && l_0x5a, &&
l_0x5b, && l_0x5c, && l_0x5d, && l_0x5e, && l_0x5f, && l_0x60, && l_0x61, && l_0x62, && l_0x63, && l_0x64, && l_0x65, && l_0x66, && l_0x67, &&
l_0x68, && l_0x69, && l_0x6a, && l_0x6b, && l_0x6c, && l_0x6d, && l_0x6e, && l_0x6f, && l_0x70, && l_0x71, && l_0x72, && l_0x73, && l_0x74, &&
l_0x75, && l_0x76, && l_0x77, && l_0x78, && l_0x79, && l_0x7a, && l_0x7b, && l_0x7c, && l_0x7d, && l_0x7e, && l_0x7f, && l_0x80, && l_0x81, &&
l_0x82, && l_0x83, && l_0x84, && l_0x85, && l_0x86, && l_0x87, && l_0x88, && l_0x89, && l_0x8a, && l_0x8b, && l_0x8c, && l_0x8d, && l_0x8e, &&
l_0x8f, && l_0x90, && l_0x91, && l_0x92, && l_0x93, && l_0x94, && l_0x95, && l_0x96, && l_0x97, && l_0x98, && l_0x99, && l_0x9a, && l_0x9b, &&
l_0x9c, && l_0x9d, && l_0x9e, && l_0x9f, && l_0xa0, && l_0xa1, && l_0xa2, && l_0xa3, && l_0xa4, && l_0xa5, && l_0xa6, && l_0xa7, && l_0xa8, &&
l_0xa9, && l_0xaa, && l_0xab, && l_0xac, && l_0xad, && l_0xae, && l_0xaf, && l_0xb0, && l_0xb1, && l_0xb2, && l_0xb3, && l_0xb4, && l_0xb5, &&
l_0xb6, && l_0xb7, && l_0xb8, && l_0xb9, && l_0xba, && l_0xbb, && l_0xbc, && l_0xbd, && l_0xbe, && l_0xbf, && l_0xc0, && l_0xc1, && l_0xc2, &&
l_0xc3, && l_0xc4, && l_0xc5, && l_0xc6, && l_0xc7, && l_0xc8, && l_0xc9, && l_0xca, && l_0xcb, && l_0xcc, && l_0xcd, && l_0xce, && l_0xcf, &&
l_0xd0, && l_0xd1, && l_0xd2, && l_0xd3, && l_0xd4, && l_0xd5, && l_0xd6, && l_0xd7, && l_0xd8, && l_0xd9, && l_0xda, && l_0xdb, && l_0xdc, &&
l_0xdd, && l_0xde, && l_0xdf, && l_0xe0, && l_0xe1, && l_0xe2, && l_0xe3, && l_0xe4, && l_0xe5, && l_0xe6, && l_0xe7, && l_0xe8, && l_0xe9, &&
l_0xea, && l_0xeb, && l_0xec, && l_0xed, && l_0xee, && l_0xef, && l_0xf0, && l_0xf1, && l_0xf2, && l_0xf3, && l_0xf4, && l_0xf5, && l_0xf6, &&
l_0xf7, && l_0xf8, && l_0xf9, && l_0xfa, && l_0xfb, && l_0xfc, && l_0xfd, && l_0xfe, && l_0xff };
  unsigned char code;
  code = (unsigned char)Mgetc(PC++);
	goto *a_jump_table[code];
  l_0x21: PC += 2; return 5;                           /* LBRN    */
  l_0x22: if(BHI) LBRANCH; PC += 2; return 5 + n;      /* LBHI    */
  l_0x23: if(BLS) LBRANCH; PC += 2; return 5 + n;      /* LBLS    */
  l_0x24: if(BCC) LBRANCH; PC += 2; return 5 + n;      /* LBCC    */
  l_0x25: if(BCS) LBRANCH; PC += 2; return 5 + n;      /* LBCS    */
  l_0x26: if(BNE) LBRANCH; PC += 2; return 5 + n;      /* LBNE    */
  l_0x27: if(BEQ) LBRANCH; PC += 2; return 5 + n;      /* LBEQ    */
  l_0x28: if(BVC) LBRANCH; PC += 2; return 5 + n;      /* LBVC    */
  l_0x29: if(BVS) LBRANCH; PC += 2; return 5 + n;      /* LBVS    */
  l_0x2a: if(BPL) LBRANCH; PC += 2; return 5 + n;      /* LBPL    */
  l_0x2b: if(BMI) LBRANCH; PC += 2; return 5 + n;      /* LBMI    */
  l_0x2c: if(BGE) LBRANCH; PC += 2; return 5 + n;      /* LBGE    */
  l_0x2d: if(BLT) LBRANCH; PC += 2; return 5 + n;      /* LBLT    */
  l_0x2e: if(BGT) LBRANCH; PC += 2; return 5 + n;      /* LBGT    */
  l_0x2f: if(BLE) LBRANCH; PC += 2; return 5 + n;      /* LBLE    */
  l_0x3f: Swi(2); return 20;                           /* SWI2    */

  l_0x83: EXT; Cmpw(&D, W); return 5;                  /* CMPD #$ */
  l_0x8c: EXT; Cmpw(&Y, W); return 5;                  /* CMPY #$ */
  l_0x8e: EXT; Tstw(Y = W); return 4;                  /* LDY  #$ */
  l_0x93: DIR; Cmpw(&D, Mgetw(DA)); return 7;          /* CMPD /$ */
  l_0x9c: DIR; Cmpw(&Y, Mgetw(DA)); return 7;          /* CMPY /$ */
  l_0x9e: DIR; Tstw(Y = Mgetw(DA)); return 6;          /* LDY  /$ */
  l_0x9f: DIR; Mputw(DA, Y); Tstw(Y); return 6;        /* STY  /$ */
  l_0xa3: IND; Cmpw(&D, Mgetw(W)); return 7 + n;       /* CMPD IX */
  l_0xac: IND; Cmpw(&Y, Mgetw(W)); return 7 + n;       /* CMPY IX */
  l_0xae: IND; Tstw(Y = Mgetw(W)); return 6 + n;       /* LDY  IX */
  l_0xaf: IND; Mputw(W, Y); Tstw(Y); return 6 + n;     /* STY  IX */
  l_0xb3: EXT; Cmpw(&D, Mgetw(W)); return 8;           /* CMPD $  */
  l_0xbc: EXT; Cmpw(&Y, Mgetw(W)); return 8;           /* CMPY $  */
  l_0xbe: EXT; Tstw(Y = Mgetw(W)); return 7;           /* LDY  $  */
  l_0xbf: EXT; Mputw(W, Y); Tstw(Y); return 7;         /* STY  $  */
  l_0xce: EXT; Tstw(S = W); return 4;                  /* LDS  #$ */
  l_0xde: DIR; Tstw(S = Mgetw(DA)); return 6;          /* LDS  /$ */
  l_0xdf: DIR; Mputw(DA, S); Tstw(S); return 6;        /* STS  /$ */
  l_0xee: IND; Tstw(S = Mgetw(W)); return 6 + n;       /* LDS  IX */
  l_0xef: IND; Mputw(W, S); Tstw(S); return 6 + n;     /* STS  IX */
  l_0xfe: EXT; Tstw(S = Mgetw(W)); return 7;           /* LDS  $  */
  l_0xff: EXT; Mputw(W, S); Tstw(S); return 7;         /* STS  $  */
l_0xfd:
l_0xfc:
l_0xfb:
l_0xfa:
l_0xf9:
l_0xf8:
l_0xf7:
l_0xf6:
l_0xf5:
l_0xf4:
l_0xf3:
l_0xf2:
l_0xf1:
l_0xf0:
l_0xed:
l_0xec:
l_0xeb:
l_0xea:
l_0xe9:
l_0xe8:
l_0xe7:
l_0xe6:
l_0xe5:
l_0xe4:
l_0xe3:
l_0xe2:
l_0xe1:
l_0xe0:
l_0xdd:
l_0xdc:
l_0xdb:
l_0xda:
l_0xd9:
l_0xd8:
l_0xd7:
l_0xd6:
l_0xd5:
l_0xd4:
l_0xd3:
l_0xd2:
l_0xd1:
l_0xd0:
l_0xcf:
l_0xcd:
l_0xcc:
l_0xcb:
l_0xca:
l_0xc9:
l_0xc8:
l_0xc7:
l_0xc6:
l_0xc5:
l_0xc4:
l_0xc3:
l_0xc2:
l_0xc1:
l_0xc0:
l_0xbd:
l_0xbb:
l_0xba:
l_0xb9:
l_0xb8:
l_0xb7:
l_0xb6:
l_0xb5:
l_0xb4:
l_0xb2:
l_0xb1:
l_0xb0:
l_0xad:
l_0xab:
l_0xaa:
l_0xa9:
l_0xa8:
l_0xa7:
l_0xa6:
l_0xa5:
l_0xa4:
l_0xa2:
l_0xa1:
l_0xa0:
l_0x9d:
l_0x9b:
l_0x9a:
l_0x99:
l_0x98:
l_0x97:
l_0x96:
l_0x95:
l_0x94:
l_0x92:
l_0x91:
l_0x90:
l_0x8f:
l_0x8d:
l_0x8b:
l_0x8a:
l_0x89:
l_0x88:
l_0x87:
l_0x86:
l_0x85:
l_0x84:
l_0x82:
l_0x81:
l_0x80:
l_0x7f:
l_0x7e:
l_0x7d:
l_0x7c:
l_0x7b:
l_0x7a:
l_0x79:
l_0x78:
l_0x77:
l_0x76:
l_0x75:
l_0x74:
l_0x73:
l_0x72:
l_0x71:
l_0x70:
l_0x6f:
l_0x6e:
l_0x6d:
l_0x6c:
l_0x6b:
l_0x6a:
l_0x69:
l_0x68:
l_0x67:
l_0x66:
l_0x65:
l_0x64:
l_0x63:
l_0x62:
l_0x61:
l_0x60:
l_0x5f:
l_0x5e:
l_0x5d:
l_0x5c:
l_0x5b:
l_0x5a:
l_0x59:
l_0x58:
l_0x57:
l_0x56:
l_0x55:
l_0x54:
l_0x53:
l_0x52:
l_0x51:
l_0x50:
l_0x4f:
l_0x4e:
l_0x4d:
l_0x4c:
l_0x4b:
l_0x4a:
l_0x49:
l_0x48:
l_0x47:
l_0x46:
l_0x45:
l_0x44:
l_0x43:
l_0x42:
l_0x41:
l_0x40:
l_0x3e:
l_0x3d:
l_0x3c:
l_0x3b:
l_0x3a:
l_0x39:
l_0x38:
l_0x37:
l_0x36:
l_0x35:
l_0x34:
l_0x33:
l_0x32:
l_0x31:
l_0x30:
l_0x20:
l_0x1f:
l_0x1e:
l_0x1d:
l_0x1c:
l_0x1b:
l_0x1a:
l_0x19:
l_0x18:
l_0x17:
l_0x16:
l_0x15:
l_0x14:
l_0x13:
l_0x12:
l_0x11:
l_0x10:
l_0x0f:
l_0x0e:
l_0x0d:
l_0x0c:
l_0x0b:
l_0x0a:
l_0x09:
l_0x08:
l_0x07:
l_0x06:
l_0x05:
l_0x04:
l_0x03:
l_0x02:
l_0x01:
l_0x00:

  return - code;
}

int Run6809_0x11()
{
 __label__ 
l_0x00, l_0x01, l_0x02, l_0x03, l_0x04, l_0x05, l_0x06, l_0x07, l_0x08, l_0x09, l_0x0a, l_0x0b, l_0x0c,
l_0x0d, l_0x0e, l_0x0f, l_0x10, l_0x11, l_0x12, l_0x13, l_0x14, l_0x15, l_0x16, l_0x17, l_0x18, l_0x19,
l_0x1a, l_0x1b, l_0x1c, l_0x1d, l_0x1e, l_0x1f, l_0x20, l_0x21, l_0x22, l_0x23, l_0x24, l_0x25, l_0x26,
l_0x27, l_0x28, l_0x29, l_0x2a, l_0x2b, l_0x2c, l_0x2d, l_0x2e, l_0x2f, l_0x30, l_0x31, l_0x32, l_0x33,
l_0x34, l_0x35, l_0x36, l_0x37, l_0x38, l_0x39, l_0x3a, l_0x3b, l_0x3c, l_0x3d, l_0x3e, l_0x3f, l_0x40,
l_0x41, l_0x42, l_0x43, l_0x44, l_0x45, l_0x46, l_0x47, l_0x48, l_0x49, l_0x4a, l_0x4b, l_0x4c, l_0x4d,
l_0x4e, l_0x4f, l_0x50, l_0x51, l_0x52, l_0x53, l_0x54, l_0x55, l_0x56, l_0x57, l_0x58, l_0x59, l_0x5a,
l_0x5b, l_0x5c, l_0x5d, l_0x5e, l_0x5f, l_0x60, l_0x61, l_0x62, l_0x63, l_0x64, l_0x65, l_0x66, l_0x67,
l_0x68, l_0x69, l_0x6a, l_0x6b, l_0x6c, l_0x6d, l_0x6e, l_0x6f, l_0x70, l_0x71, l_0x72, l_0x73, l_0x74,
l_0x75, l_0x76, l_0x77, l_0x78, l_0x79, l_0x7a, l_0x7b, l_0x7c, l_0x7d, l_0x7e, l_0x7f, l_0x80, l_0x81,
l_0x82, l_0x83, l_0x84, l_0x85, l_0x86, l_0x87, l_0x88, l_0x89, l_0x8a, l_0x8b, l_0x8c, l_0x8d, l_0x8e,
l_0x8f, l_0x90, l_0x91, l_0x92, l_0x93, l_0x94, l_0x95, l_0x96, l_0x97, l_0x98, l_0x99, l_0x9a, l_0x9b,
l_0x9c, l_0x9d, l_0x9e, l_0x9f, l_0xa0, l_0xa1, l_0xa2, l_0xa3, l_0xa4, l_0xa5, l_0xa6, l_0xa7, l_0xa8,
l_0xa9, l_0xaa, l_0xab, l_0xac, l_0xad, l_0xae, l_0xaf, l_0xb0, l_0xb1, l_0xb2, l_0xb3, l_0xb4, l_0xb5,
l_0xb6, l_0xb7, l_0xb8, l_0xb9, l_0xba, l_0xbb, l_0xbc, l_0xbd, l_0xbe, l_0xbf, l_0xc0, l_0xc1, l_0xc2,
l_0xc3, l_0xc4, l_0xc5, l_0xc6, l_0xc7, l_0xc8, l_0xc9, l_0xca, l_0xcb, l_0xcc, l_0xcd, l_0xce, l_0xcf,
l_0xd0, l_0xd1, l_0xd2, l_0xd3, l_0xd4, l_0xd5, l_0xd6, l_0xd7, l_0xd8, l_0xd9, l_0xda, l_0xdb, l_0xdc,
l_0xdd, l_0xde, l_0xdf, l_0xe0, l_0xe1, l_0xe2, l_0xe3, l_0xe4, l_0xe5, l_0xe6, l_0xe7, l_0xe8, l_0xe9,
l_0xea, l_0xeb, l_0xec, l_0xed, l_0xee, l_0xef, l_0xf0, l_0xf1, l_0xf2, l_0xf3, l_0xf4, l_0xf5, l_0xf6,
l_0xf7, l_0xf8, l_0xf9, l_0xfa, l_0xfb, l_0xfc, l_0xfd, l_0xfe, l_0xff; 
    static const void* const a_jump_table[256] = 
	  { &&
l_0x00, && l_0x01, && l_0x02, && l_0x03, && l_0x04, && l_0x05, && l_0x06, && l_0x07, && l_0x08, && l_0x09, && l_0x0a, && l_0x0b, && l_0x0c, &&
l_0x0d, && l_0x0e, && l_0x0f, && l_0x10, && l_0x11, && l_0x12, && l_0x13, && l_0x14, && l_0x15, && l_0x16, && l_0x17, && l_0x18, && l_0x19, &&
l_0x1a, && l_0x1b, && l_0x1c, && l_0x1d, && l_0x1e, && l_0x1f, && l_0x20, && l_0x21, && l_0x22, && l_0x23, && l_0x24, && l_0x25, && l_0x26, &&
l_0x27, && l_0x28, && l_0x29, && l_0x2a, && l_0x2b, && l_0x2c, && l_0x2d, && l_0x2e, && l_0x2f, && l_0x30, && l_0x31, && l_0x32, && l_0x33, &&
l_0x34, && l_0x35, && l_0x36, && l_0x37, && l_0x38, && l_0x39, && l_0x3a, && l_0x3b, && l_0x3c, && l_0x3d, && l_0x3e, && l_0x3f, && l_0x40, &&
l_0x41, && l_0x42, && l_0x43, && l_0x44, && l_0x45, && l_0x46, && l_0x47, && l_0x48, && l_0x49, && l_0x4a, && l_0x4b, && l_0x4c, && l_0x4d, &&
l_0x4e, && l_0x4f, && l_0x50, && l_0x51, && l_0x52, && l_0x53, && l_0x54, && l_0x55, && l_0x56, && l_0x57, && l_0x58, && l_0x59, && l_0x5a, &&
l_0x5b, && l_0x5c, && l_0x5d, && l_0x5e, && l_0x5f, && l_0x60, && l_0x61, && l_0x62, && l_0x63, && l_0x64, && l_0x65, && l_0x66, && l_0x67, &&
l_0x68, && l_0x69, && l_0x6a, && l_0x6b, && l_0x6c, && l_0x6d, && l_0x6e, && l_0x6f, && l_0x70, && l_0x71, && l_0x72, && l_0x73, && l_0x74, &&
l_0x75, && l_0x76, && l_0x77, && l_0x78, && l_0x79, && l_0x7a, && l_0x7b, && l_0x7c, && l_0x7d, && l_0x7e, && l_0x7f, && l_0x80, && l_0x81, &&
l_0x82, && l_0x83, && l_0x84, && l_0x85, && l_0x86, && l_0x87, && l_0x88, && l_0x89, && l_0x8a, && l_0x8b, && l_0x8c, && l_0x8d, && l_0x8e, &&
l_0x8f, && l_0x90, && l_0x91, && l_0x92, && l_0x93, && l_0x94, && l_0x95, && l_0x96, && l_0x97, && l_0x98, && l_0x99, && l_0x9a, && l_0x9b, &&
l_0x9c, && l_0x9d, && l_0x9e, && l_0x9f, && l_0xa0, && l_0xa1, && l_0xa2, && l_0xa3, && l_0xa4, && l_0xa5, && l_0xa6, && l_0xa7, && l_0xa8, &&
l_0xa9, && l_0xaa, && l_0xab, && l_0xac, && l_0xad, && l_0xae, && l_0xaf, && l_0xb0, && l_0xb1, && l_0xb2, && l_0xb3, && l_0xb4, && l_0xb5, &&
l_0xb6, && l_0xb7, && l_0xb8, && l_0xb9, && l_0xba, && l_0xbb, && l_0xbc, && l_0xbd, && l_0xbe, && l_0xbf, && l_0xc0, && l_0xc1, && l_0xc2, &&
l_0xc3, && l_0xc4, && l_0xc5, && l_0xc6, && l_0xc7, && l_0xc8, && l_0xc9, && l_0xca, && l_0xcb, && l_0xcc, && l_0xcd, && l_0xce, && l_0xcf, &&
l_0xd0, && l_0xd1, && l_0xd2, && l_0xd3, && l_0xd4, && l_0xd5, && l_0xd6, && l_0xd7, && l_0xd8, && l_0xd9, && l_0xda, && l_0xdb, && l_0xdc, &&
l_0xdd, && l_0xde, && l_0xdf, && l_0xe0, && l_0xe1, && l_0xe2, && l_0xe3, && l_0xe4, && l_0xe5, && l_0xe6, && l_0xe7, && l_0xe8, && l_0xe9, &&
l_0xea, && l_0xeb, && l_0xec, && l_0xed, && l_0xee, && l_0xef, && l_0xf0, && l_0xf1, && l_0xf2, && l_0xf3, && l_0xf4, && l_0xf5, && l_0xf6, &&
l_0xf7, && l_0xf8, && l_0xf9, && l_0xfa, && l_0xfb, && l_0xfc, && l_0xfd, && l_0xfe, && l_0xff };
  unsigned char code;
  code = (unsigned char)Mgetc(PC++);
	goto *a_jump_table[code];

  l_0x3f: Swi(3); return 20;                           /* SWI3    */
  l_0x83: EXT; Cmpw(&U, W); return 5;                  /* CMPU #$ */
  l_0x8c: EXT; Cmpw(&S, W); return 5;                  /* CMPS #$ */
  l_0x93: DIR; Cmpw(&U, Mgetw(DA)); return 7;          /* CMPU /$ */
  l_0x9c: DIR; Cmpw(&S, Mgetw(DA)); return 7;          /* CMPS /$ */
  l_0xa3: IND; Cmpw(&U, Mgetw(W)); return 7 + n;       /* CMPU IX */
  l_0xac: IND; Cmpw(&S, Mgetw(W)); return 7 + n;       /* CMPS IX */
  l_0xb3: EXT; Cmpw(&U, Mgetw(W)); return 8;           /* CMPU $  */
  l_0xbc: EXT; Cmpw(&S, Mgetw(W)); return 8;           /* CMPS $  */
l_0xff:
l_0xfe:
l_0xfd:
l_0xfc:
l_0xfb:
l_0xfa:
l_0xf9:
l_0xf8:
l_0xf7:
l_0xf6:
l_0xf5:
l_0xf4:
l_0xf3:
l_0xf2:
l_0xf1:
l_0xf0:
l_0xef:
l_0xee:
l_0xed:
l_0xec:
l_0xeb:
l_0xea:
l_0xe9:
l_0xe8:
l_0xe7:
l_0xe6:
l_0xe5:
l_0xe4:
l_0xe3:
l_0xe2:
l_0xe1:
l_0xe0:
l_0xdf:
l_0xde:
l_0xdd:
l_0xdc:
l_0xdb:
l_0xda:
l_0xd9:
l_0xd8:
l_0xd7:
l_0xd6:
l_0xd5:
l_0xd4:
l_0xd3:
l_0xd2:
l_0xd1:
l_0xd0:
l_0xcf:
l_0xce:
l_0xcd:
l_0xcc:
l_0xcb:
l_0xca:
l_0xc9:
l_0xc8:
l_0xc7:
l_0xc6:
l_0xc5:
l_0xc4:
l_0xc3:
l_0xc2:
l_0xc1:
l_0xc0:
l_0xbf:
l_0xbe:
l_0xbd:
l_0xbb:
l_0xba:
l_0xb9:
l_0xb8:
l_0xb7:
l_0xb6:
l_0xb5:
l_0xb4:
l_0xb2:
l_0xb1:
l_0xb0:
l_0xaf:
l_0xae:
l_0xad:
l_0xab:
l_0xaa:
l_0xa9:
l_0xa8:
l_0xa7:
l_0xa6:
l_0xa5:
l_0xa4:
l_0xa2:
l_0xa1:
l_0xa0:
l_0x9f:
l_0x9e:
l_0x9d:
l_0x9b:
l_0x9a:
l_0x99:
l_0x98:
l_0x97:
l_0x96:
l_0x95:
l_0x94:
l_0x92:
l_0x91:
l_0x90:
l_0x8f:
l_0x8e:
l_0x8d:
l_0x8b:
l_0x8a:
l_0x89:
l_0x88:
l_0x87:
l_0x86:
l_0x85:
l_0x84:
l_0x82:
l_0x81:
l_0x80:
l_0x7f:
l_0x7e:
l_0x7d:
l_0x7c:
l_0x7b:
l_0x7a:
l_0x79:
l_0x78:
l_0x77:
l_0x76:
l_0x75:
l_0x74:
l_0x73:
l_0x72:
l_0x71:
l_0x70:
l_0x6f:
l_0x6e:
l_0x6d:
l_0x6c:
l_0x6b:
l_0x6a:
l_0x69:
l_0x68:
l_0x67:
l_0x66:
l_0x65:
l_0x64:
l_0x63:
l_0x62:
l_0x61:
l_0x60:
l_0x5f:
l_0x5e:
l_0x5d:
l_0x5c:
l_0x5b:
l_0x5a:
l_0x59:
l_0x58:
l_0x57:
l_0x56:
l_0x55:
l_0x54:
l_0x53:
l_0x52:
l_0x51:
l_0x50:
l_0x4f:
l_0x4e:
l_0x4d:
l_0x4c:
l_0x4b:
l_0x4a:
l_0x49:
l_0x48:
l_0x47:
l_0x46:
l_0x45:
l_0x44:
l_0x43:
l_0x42:
l_0x41:
l_0x40:
l_0x3e:
l_0x3d:
l_0x3c:
l_0x3b:
l_0x3a:
l_0x39:
l_0x38:
l_0x37:
l_0x36:
l_0x35:
l_0x34:
l_0x33:
l_0x32:
l_0x31:
l_0x30:
l_0x2f:
l_0x2e:
l_0x2d:
l_0x2c:
l_0x2b:
l_0x2a:
l_0x29:
l_0x28:
l_0x27:
l_0x26:
l_0x25:
l_0x24:
l_0x23:
l_0x22:
l_0x21:
l_0x20:
l_0x1f:
l_0x1e:
l_0x1d:
l_0x1c:
l_0x1b:
l_0x1a:
l_0x19:
l_0x18:
l_0x17:
l_0x16:
l_0x15:
l_0x14:
l_0x13:
l_0x12:
l_0x11:
l_0x10:
l_0x0f:
l_0x0e:
l_0x0d:
l_0x0c:
l_0x0b:
l_0x0a:
l_0x09:
l_0x08:
l_0x07:
l_0x06:
l_0x05:
l_0x04:
l_0x03:
l_0x02:
l_0x01:
l_0x00:

  return -code;
}

// Execute one operation at PC address and set PC to next opcode address //////
/*
Return value is set to :
- cycle count for the executed instruction when operation code is legal
- negative value (-code) when operation code is illegal
*/
int Run6809()
{
 __label__ 
l_0x00, l_0x01, l_0x02, l_0x03, l_0x04, l_0x05, l_0x06, l_0x07, l_0x08, l_0x09, l_0x0a, l_0x0b, l_0x0c,
l_0x0d, l_0x0e, l_0x0f, l_0x10, l_0x11, l_0x12, l_0x13, l_0x14, l_0x15, l_0x16, l_0x17, l_0x18, l_0x19,
l_0x1a, l_0x1b, l_0x1c, l_0x1d, l_0x1e, l_0x1f, l_0x20, l_0x21, l_0x22, l_0x23, l_0x24, l_0x25, l_0x26,
l_0x27, l_0x28, l_0x29, l_0x2a, l_0x2b, l_0x2c, l_0x2d, l_0x2e, l_0x2f, l_0x30, l_0x31, l_0x32, l_0x33,
l_0x34, l_0x35, l_0x36, l_0x37, l_0x38, l_0x39, l_0x3a, l_0x3b, l_0x3c, l_0x3d, l_0x3e, l_0x3f, l_0x40,
l_0x41, l_0x42, l_0x43, l_0x44, l_0x45, l_0x46, l_0x47, l_0x48, l_0x49, l_0x4a, l_0x4b, l_0x4c, l_0x4d,
l_0x4e, l_0x4f, l_0x50, l_0x51, l_0x52, l_0x53, l_0x54, l_0x55, l_0x56, l_0x57, l_0x58, l_0x59, l_0x5a,
l_0x5b, l_0x5c, l_0x5d, l_0x5e, l_0x5f, l_0x60, l_0x61, l_0x62, l_0x63, l_0x64, l_0x65, l_0x66, l_0x67,
l_0x68, l_0x69, l_0x6a, l_0x6b, l_0x6c, l_0x6d, l_0x6e, l_0x6f, l_0x70, l_0x71, l_0x72, l_0x73, l_0x74,
l_0x75, l_0x76, l_0x77, l_0x78, l_0x79, l_0x7a, l_0x7b, l_0x7c, l_0x7d, l_0x7e, l_0x7f, l_0x80, l_0x81,
l_0x82, l_0x83, l_0x84, l_0x85, l_0x86, l_0x87, l_0x88, l_0x89, l_0x8a, l_0x8b, l_0x8c, l_0x8d, l_0x8e,
l_0x8f, l_0x90, l_0x91, l_0x92, l_0x93, l_0x94, l_0x95, l_0x96, l_0x97, l_0x98, l_0x99, l_0x9a, l_0x9b,
l_0x9c, l_0x9d, l_0x9e, l_0x9f, l_0xa0, l_0xa1, l_0xa2, l_0xa3, l_0xa4, l_0xa5, l_0xa6, l_0xa7, l_0xa8,
l_0xa9, l_0xaa, l_0xab, l_0xac, l_0xad, l_0xae, l_0xaf, l_0xb0, l_0xb1, l_0xb2, l_0xb3, l_0xb4, l_0xb5,
l_0xb6, l_0xb7, l_0xb8, l_0xb9, l_0xba, l_0xbb, l_0xbc, l_0xbd, l_0xbe, l_0xbf, l_0xc0, l_0xc1, l_0xc2,
l_0xc3, l_0xc4, l_0xc5, l_0xc6, l_0xc7, l_0xc8, l_0xc9, l_0xca, l_0xcb, l_0xcc, l_0xcd, l_0xce, l_0xcf,
l_0xd0, l_0xd1, l_0xd2, l_0xd3, l_0xd4, l_0xd5, l_0xd6, l_0xd7, l_0xd8, l_0xd9, l_0xda, l_0xdb, l_0xdc,
l_0xdd, l_0xde, l_0xdf, l_0xe0, l_0xe1, l_0xe2, l_0xe3, l_0xe4, l_0xe5, l_0xe6, l_0xe7, l_0xe8, l_0xe9,
l_0xea, l_0xeb, l_0xec, l_0xed, l_0xee, l_0xef, l_0xf0, l_0xf1, l_0xf2, l_0xf3, l_0xf4, l_0xf5, l_0xf6,
l_0xf7, l_0xf8, l_0xf9, l_0xfa, l_0xfb, l_0xfc, l_0xfd, l_0xfe, l_0xff; 
    static const void* const a_jump_table[256] = 
	  { &&
l_0x00, && l_0x01, && l_0x02, && l_0x03, && l_0x04, && l_0x05, && l_0x06, && l_0x07, && l_0x08, && l_0x09, && l_0x0a, && l_0x0b, && l_0x0c, &&
l_0x0d, && l_0x0e, && l_0x0f, && l_0x10, && l_0x11, && l_0x12, && l_0x13, && l_0x14, && l_0x15, && l_0x16, && l_0x17, && l_0x18, && l_0x19, &&
l_0x1a, && l_0x1b, && l_0x1c, && l_0x1d, && l_0x1e, && l_0x1f, && l_0x20, && l_0x21, && l_0x22, && l_0x23, && l_0x24, && l_0x25, && l_0x26, &&
l_0x27, && l_0x28, && l_0x29, && l_0x2a, && l_0x2b, && l_0x2c, && l_0x2d, && l_0x2e, && l_0x2f, && l_0x30, && l_0x31, && l_0x32, && l_0x33, &&
l_0x34, && l_0x35, && l_0x36, && l_0x37, && l_0x38, && l_0x39, && l_0x3a, && l_0x3b, && l_0x3c, && l_0x3d, && l_0x3e, && l_0x3f, && l_0x40, &&
l_0x41, && l_0x42, && l_0x43, && l_0x44, && l_0x45, && l_0x46, && l_0x47, && l_0x48, && l_0x49, && l_0x4a, && l_0x4b, && l_0x4c, && l_0x4d, &&
l_0x4e, && l_0x4f, && l_0x50, && l_0x51, && l_0x52, && l_0x53, && l_0x54, && l_0x55, && l_0x56, && l_0x57, && l_0x58, && l_0x59, && l_0x5a, &&
l_0x5b, && l_0x5c, && l_0x5d, && l_0x5e, && l_0x5f, && l_0x60, && l_0x61, && l_0x62, && l_0x63, && l_0x64, && l_0x65, && l_0x66, && l_0x67, &&
l_0x68, && l_0x69, && l_0x6a, && l_0x6b, && l_0x6c, && l_0x6d, && l_0x6e, && l_0x6f, && l_0x70, && l_0x71, && l_0x72, && l_0x73, && l_0x74, &&
l_0x75, && l_0x76, && l_0x77, && l_0x78, && l_0x79, && l_0x7a, && l_0x7b, && l_0x7c, && l_0x7d, && l_0x7e, && l_0x7f, && l_0x80, && l_0x81, &&
l_0x82, && l_0x83, && l_0x84, && l_0x85, && l_0x86, && l_0x87, && l_0x88, && l_0x89, && l_0x8a, && l_0x8b, && l_0x8c, && l_0x8d, && l_0x8e, &&
l_0x8f, && l_0x90, && l_0x91, && l_0x92, && l_0x93, && l_0x94, && l_0x95, && l_0x96, && l_0x97, && l_0x98, && l_0x99, && l_0x9a, && l_0x9b, &&
l_0x9c, && l_0x9d, && l_0x9e, && l_0x9f, && l_0xa0, && l_0xa1, && l_0xa2, && l_0xa3, && l_0xa4, && l_0xa5, && l_0xa6, && l_0xa7, && l_0xa8, &&
l_0xa9, && l_0xaa, && l_0xab, && l_0xac, && l_0xad, && l_0xae, && l_0xaf, && l_0xb0, && l_0xb1, && l_0xb2, && l_0xb3, && l_0xb4, && l_0xb5, &&
l_0xb6, && l_0xb7, && l_0xb8, && l_0xb9, && l_0xba, && l_0xbb, && l_0xbc, && l_0xbd, && l_0xbe, && l_0xbf, && l_0xc0, && l_0xc1, && l_0xc2, &&
l_0xc3, && l_0xc4, && l_0xc5, && l_0xc6, && l_0xc7, && l_0xc8, && l_0xc9, && l_0xca, && l_0xcb, && l_0xcc, && l_0xcd, && l_0xce, && l_0xcf, &&
l_0xd0, && l_0xd1, && l_0xd2, && l_0xd3, && l_0xd4, && l_0xd5, && l_0xd6, && l_0xd7, && l_0xd8, && l_0xd9, && l_0xda, && l_0xdb, && l_0xdc, &&
l_0xdd, && l_0xde, && l_0xdf, && l_0xe0, && l_0xe1, && l_0xe2, && l_0xe3, && l_0xe4, && l_0xe5, && l_0xe6, && l_0xe7, && l_0xe8, && l_0xe9, &&
l_0xea, && l_0xeb, && l_0xec, && l_0xed, && l_0xee, && l_0xef, && l_0xf0, && l_0xf1, && l_0xf2, && l_0xf3, && l_0xf4, && l_0xf5, && l_0xf6, &&
l_0xf7, && l_0xf8, && l_0xf9, && l_0xfa, && l_0xfb, && l_0xfc, && l_0xfd, && l_0xfe, && l_0xff };
  n = 0; //par defaut pas de cycles supplementaires
  unsigned char code = (unsigned char)Mgetc(PC++);
	goto *a_jump_table[code];

 //switch(code) {
  l_0x00: DIR; Mputc(DA, Neg(Mgetc(DA))); return 6;      /* NEG  /$ */
  l_0x01: DIR; return 3;                            /*undoc BRN     */
  l_0x10: return Run6809_0x10();
  l_0x11: return Run6809_0x11();
//l_0x02: 
// if(CC&C1){Mputc(wd, Com(Mgetc(DA))); return 6;}     /*undoc COM  /$ */
//      else{Mputc(DA, Neg(Mgetc(DA))); return 6;}     /*undoc NEG  /$ */
  l_0x03: DIR; Mputc(DA, Com(Mgetc(DA))); return 6;      /* COM  /$ */
  l_0x04: DIR; Mputc(DA, Lsr(Mgetc(DA))); return 6;      /* LSR  /$ */
//l_0x05: Mputc(DA, Lsr(Mgetc(DA))); return 6;      /*undoc LSR  /$ */
  l_0x06: DIR; Mputc(DA, Ror(Mgetc(DA))); return 6;      /* ROR  /$ */
  l_0x07: DIR; Mputc(DA, Asr(Mgetc(DA))); return 6;      /* ASR  /$ */
  l_0x08: DIR; Mputc(DA, Asl(Mgetc(DA))); return 6;      /* ASL  /$ */
  l_0x09: DIR; Mputc(DA, Rol(Mgetc(DA))); return 6;      /* ROL  /$ */
  l_0x0a: DIR; Mputc(DA, Dec(Mgetc(DA))); return 6;      /* DEC  /$ */
  l_0x0c: DIR; Mputc(DA, Inc(Mgetc(DA))); return 6;      /* INC  /$ */
  l_0x0d: DIR; Tstc(Mgetc(DA)); return 6;                /* TST  /$ */
  l_0x0e: DIR; PC = DA; return 3;                        /* JMP  /$ */
  l_0x0f: DIR; Mputc(DA, Clr()); return 6;               /* CLR  /$ */

  l_0x12: return 2;                                      /* NOP     */
  l_0x13: return 4;                                      /* SYNC    */
  l_0x16: PC += Mgetw(PC) + 2; return 5;                 /* LBRA    */
  l_0x17: EXT; Pshs(0x80); PC += W; return 9;            /* LBSR    */
  l_0x19: Daa(); return 2;                               /* DAA     */
  l_0x1a: CC |= Mgetc(PC++); return 3;                   /* ORCC #$ */
  l_0x1c: CC &= Mgetc(PC++); return 3;                   /* ANDC #$ */
  l_0x1d: Tstw(D = B); return 2;                         /* SEX     */
  l_0x1e: Exg(Mgetc(PC++)); return 8;                    /* EXG     */
  l_0x1f: Tfr(Mgetc(PC++)); return 6;                    /* TFR     */

  l_0x20: BRANCH; PC++; return 3;                        /* BRA     */
  l_0x21: PC++; return 3;                                /* BRN     */
  l_0x22: if(BHI) BRANCH; PC++; return 3;                /* BHI     */
  l_0x23: if(BLS) BRANCH; PC++; return 3;                /* BLS     */
  l_0x24: if(BCC) BRANCH; PC++; return 3;                /* BCC     */
  l_0x25: if(BCS) BRANCH; PC++; return 3;                /* BCS     */
  l_0x26: if(BNE) BRANCH; PC++; return 3;                /* BNE     */
  l_0x27: if(BEQ) BRANCH; PC++; return 3;                /* BEQ     */
  l_0x28: if(BVC) BRANCH; PC++; return 3;                /* BVC     */
  l_0x29: if(BVS) BRANCH; PC++; return 3;                /* BVS     */
  l_0x2a: if(BPL) BRANCH; PC++; return 3;                /* BPL     */
  l_0x2b: if(BMI) BRANCH; PC++; return 3;                /* BMI     */
  l_0x2c: if(BGE) BRANCH; PC++; return 3;                /* BGE     */
  l_0x2d: if(BLT) BRANCH; PC++; return 3;                /* BLT     */
  l_0x2e: if(BGT) BRANCH; PC++; return 3;                /* BGT     */
  l_0x2f: if(BLE) BRANCH; PC++; return 3;                /* BLE     */

  l_0x30: IND; X = W; SETZERO; return 4 + n;           /* LEAX    */
  l_0x31: IND; Y = W; SETZERO; return 4 + n;           /* LEAY    */
  //d'apres Prehisto, LEAX et LEAY positionnent aussi le bit N de CC
  //il faut donc modifier l'émulation de ces deux instructions
  l_0x32: IND; S = W; return 4 + n; /*CC not set*/       /* LEAS    */
  l_0x33: IND; U = W; return 4 + n; /*CC not set*/       /* LEAU    */
  l_0x34: Pshs(Mgetc(PC++)); return 5 + n;               /* PSHS    */
  l_0x35: Puls(Mgetc(PC++)); return 5 + n;               /* PULS    */
  l_0x36: Pshu(Mgetc(PC++)); return 5 + n;               /* PSHU    */
  l_0x37: Pulu(Mgetc(PC++)); return 5 + n;               /* PULU    */
  l_0x39: Puls(0x80); return 5;                          /* RTS     */
  l_0x3a: X += B & 0xff; return 3;                       /* ABX     */
  l_0x3b: Rti(); return 4 + n;                           /* RTI     */
  l_0x3c: CC &= Mgetc(PC++); CC |= E1; return 20;        /* CWAI    */
  l_0x3d: Mul(); return 11;                              /* MUL     */
  l_0x3f: Swi(1); return 19;                             /* SWI     */

  l_0x40: A = Neg(A); return 2;                          /* NEGA    */
  l_0x43: A = Com(A); return 2;                          /* COMA    */
  l_0x44: A = Lsr(A); return 2;                          /* LSRA    */
  l_0x46: A = Ror(A); return 2;                          /* RORA    */
  l_0x47: A = Asr(A); return 2;                          /* ASRA    */
  l_0x48: A = Asl(A); return 2;                          /* ASLA    */
  l_0x49: A = Rol(A); return 2;                          /* ROLA    */
  l_0x4a: A = Dec(A); return 2;                          /* DECA    */
  l_0x4c: A = Inc(A); return 2;                          /* INCA    */
  l_0x4d: Tstc(A); return 2;                             /* TSTA    */
  l_0x4f: A = Clr(); return 2;                           /* CLRA    */

  l_0x50: B = Neg(B); return 2;                          /* NEGB    */
  l_0x53: B = Com(B); return 2;                          /* COMB    */
  l_0x54: B = Lsr(B); return 2;                          /* LSRB    */
  l_0x56: B = Ror(B); return 2;                          /* RORB    */
  l_0x57: B = Asr(B); return 2;                          /* ASRB    */
  l_0x58: B = Asl(B); return 2;                          /* ASLB    */
  l_0x59: B = Rol(B); return 2;                          /* ROLB    */
  l_0x5a: B = Dec(B); return 2;                          /* DECB    */
  l_0x5c: B = Inc(B); return 2;                          /* INCB    */
  l_0x5d: Tstc(B); return 2;                             /* TSTB    */
  l_0x5f: B = Clr(); return 2;                           /* CLRB    */

  l_0x60: IND; Mputc(W, Neg(Mgetc(W))); return 6 + n;    /* NEG  IX */
  l_0x63: IND; Mputc(W, Com(Mgetc(W))); return 6 + n;    /* COM  IX */
  l_0x64: IND; Mputc(W, Lsr(Mgetc(W))); return 6 + n;    /* LSR  IX */
  l_0x66: IND; Mputc(W, Ror(Mgetc(W))); return 6 + n;    /* ROR  IX */
  l_0x67: IND; Mputc(W, Asr(Mgetc(W))); return 6 + n;    /* ASR  IX */
  l_0x68: IND; Mputc(W, Asl(Mgetc(W))); return 6 + n;    /* ASL  IX */
  l_0x69: IND; Mputc(W, Rol(Mgetc(W))); return 6 + n;    /* ROL  IX */
  l_0x6a: IND; Mputc(W, Dec(Mgetc(W))); return 6 + n;    /* DEC  IX */
  l_0x6c: IND; Mputc(W, Inc(Mgetc(W))); return 6 + n;    /* INC  IX */
  l_0x6d: IND; Tstc(Mgetc(W)); return 6 + n;             /* TST  IX */
  l_0x6e: IND; PC = W; return 3 + n;                     /* JMP  IX */
  l_0x6f: IND; Mputc(W, Clr()); return 6 + n;            /* CLR  IX */

  l_0x70: EXT; Mputc(W, Neg(Mgetc(W))); return 7;        /* NEG  $  */
  l_0x73: EXT; Mputc(W, Com(Mgetc(W))); return 7;        /* COM  $  */
  l_0x74: EXT; Mputc(W, Lsr(Mgetc(W))); return 7;        /* LSR  $  */
  l_0x76: EXT; Mputc(W, Ror(Mgetc(W))); return 7;        /* ROR  $  */
  l_0x77: EXT; Mputc(W, Asr(Mgetc(W))); return 7;        /* ASR  $  */
  l_0x78: EXT; Mputc(W, Asl(Mgetc(W))); return 7;        /* ASL  $  */
  l_0x79: EXT; Mputc(W, Rol(Mgetc(W))); return 7;        /* ROL  $  */
  l_0x7a: EXT; Mputc(W, Dec(Mgetc(W))); return 7;        /* DEC  $  */
  l_0x7c: EXT; Mputc(W, Inc(Mgetc(W))); return 7;        /* INC  $  */
  l_0x7d: EXT; Tstc(Mgetc(W)); return 7;                 /* TST  $  */
  l_0x7e: EXT; PC = W; return 4;                         /* JMP  $  */
  l_0x7f: EXT; Mputc(W, Clr()); return 7;                /* CLR  $  */

  l_0x80: Subc(&A, Mgetc(PC++)); return 2;               /* SUBA #$ */
  l_0x81: Cmpc(&A, Mgetc(PC++)); return 2;               /* CMPA #$ */
  l_0x82: Sbc(&A, Mgetc(PC++)); return 2;                /* SBCA #$ */
  l_0x83: EXT; Subw(&D, W); return 4;                    /* SUBD #$ */
  l_0x84: Tstc(A &= Mgetc(PC++)); return 2;              /* ANDA #$ */
  l_0x85: Tstc(A & Mgetc(PC++)); return 2;               /* BITA #$ */
  l_0x86: Tstc(A = Mgetc(PC++)); return 2;               /* LDA  #$ */
  l_0x88: Tstc(A ^= Mgetc(PC++)); return 2;              /* EORA #$ */
  l_0x89: Adc(&A, Mgetc(PC++)); return 2;                /* ADCA #$ */
  l_0x8a: Tstc(A |= Mgetc(PC++)); return 2;              /* ORA  #$ */
  l_0x8b: Addc(&A, Mgetc(PC++)); return 2;               /* ADDA #$ */
  l_0x8c: EXT; Cmpw(&X, W); return 4;                    /* CMPX #$ */
  l_0x8d: DIR; Pshs(0x80); PC += DD; return 7;           /* BSR     */
  l_0x8e: EXT; Tstw(X = W); return 3;                    /* LDX  #$ */

  l_0x90: DIR; Subc(&A, Mgetc(DA)); return 4;            /* SUBA /$ */
  l_0x91: DIR; Cmpc(&A, Mgetc(DA)); return 4;            /* CMPA /$ */
  l_0x92: DIR; Sbc(&A, Mgetc(DA)); return 4;             /* SBCA /$ */
  l_0x93: DIR; Subw(&D, Mgetw(DA));return 6;             /* SUBD /$ */
  l_0x94: DIR; Tstc(A &= Mgetc(DA)); return 4;           /* ANDA /$ */
  l_0x95: DIR; Tstc(A & Mgetc(DA)); return 4;            /* BITA /$ */
  l_0x96: DIR; Tstc(A = Mgetc(DA)); return 4;            /* LDA  /$ */
  l_0x97: DIR; Mputc(DA, A); Tstc(A); return 4;          /* STA  /$ */
  l_0x98: DIR; Tstc(A ^= Mgetc(DA)); return 4;           /* EORA /$ */
  l_0x99: DIR; Adc(&A, Mgetc(DA)); return 4;             /* ADCA /$ */
  l_0x9a: DIR; Tstc(A |= Mgetc(DA)); return 4;           /* ORA  /$ */
  l_0x9b: DIR; Addc(&A, Mgetc(DA)); return 4;            /* ADDA /$ */
  l_0x9c: DIR; Cmpw(&X, Mgetw(DA)); return 6;            /* CMPX /$ */
  l_0x9d: DIR; Pshs(0x80); PC = DA; return 7;            /* JSR  /$ */
  l_0x9e: DIR; Tstw(X = Mgetw(DA)); return 5;            /* LDX  /$ */
  l_0x9f: DIR; Mputw(DA, X); Tstw(X); return 5;          /* STX  /$ */

  l_0xa0: IND; Subc(&A, Mgetc(W)); return 4 + n;         /* SUBA IX */
  l_0xa1: IND; Cmpc(&A, Mgetc(W)); return 4 + n;         /* CMPA IX */
  l_0xa2: IND; Sbc(&A, Mgetc(W)); return 4 + n;          /* SBCA IX */
  l_0xa3: IND; Subw(&D, Mgetw(W)); return 6 + n;         /* SUBD IX */
  l_0xa4: IND; Tstc(A &= Mgetc(W)); return 4 + n;        /* ANDA IX */
  l_0xa5: IND; Tstc(Mgetc(W) & A); return 4 + n;         /* BITA IX */
  l_0xa6: IND; Tstc(A = Mgetc(W)); return 4 + n;         /* LDA  IX */
  l_0xa7: IND; Mputc(W,A); Tstc(A); return 4 + n;        /* STA  IX */
  l_0xa8: IND; Tstc(A ^= Mgetc(W)); return 4 + n;        /* EORA IX */
  l_0xa9: IND; Adc(&A, Mgetc(W)); return 4 + n;          /* ADCA IX */
  l_0xaa: IND; Tstc(A |= Mgetc(W)); return 4 + n;        /* ORA  IX */
  l_0xab: IND; Addc(&A, Mgetc(W)); return 4 + n;         /* ADDA IX */
  l_0xac: IND; Cmpw(&X, Mgetw(W)); return 4 + n;         /* CMPX IX */
  l_0xad: IND; Pshs(0x80); PC = W; return 5 + n;         /* JSR  IX */
  l_0xae: IND; Tstw(X = Mgetw(W)); return 5 + n;         /* LDX  IX */
  l_0xaf: IND; Mputw(W, X); Tstw(X); return 5 + n;       /* STX  IX */

  l_0xb0: EXT; Subc(&A, Mgetc(W)); return 5;             /* SUBA $  */
  l_0xb1: EXT; Cmpc(&A, Mgetc(W)); return 5;             /* CMPA $  */
  l_0xb2: EXT; Sbc(&A, Mgetc(W)); return 5;              /* SBCA $  */
  l_0xb3: EXT; Subw(&D, Mgetw(W)); return 7;             /* SUBD $  */
  l_0xb4: EXT; Tstc(A &= Mgetc(W)); return 5;            /* ANDA $  */
  l_0xb5: EXT; Tstc(A & Mgetc(W)); return 5;             /* BITA $  */
  l_0xb6: EXT; Tstc(A = Mgetc(W)); return 5;             /* LDA  $  */
  l_0xb7: EXT; Mputc(W, A); Tstc(A); return 5;           /* STA  $  */
  l_0xb8: EXT; Tstc(A ^= Mgetc(W)); return 5;            /* EORA $  */
  l_0xb9: EXT; Adc(&A, Mgetc(W)); return 5;              /* ADCA $  */
  l_0xba: EXT; Tstc(A |= Mgetc(W)); return 5;            /* ORA  $  */
  l_0xbb: EXT; Addc(&A, Mgetc(W)); return 5;             /* ADDA $  */
  l_0xbc: EXT; Cmpw(&X, Mgetw(W)); return 7;             /* CMPX $  */
  l_0xbd: EXT; Pshs(0x80); PC = W; return 8;             /* JSR  $  */
  l_0xbe: EXT; Tstw(X = Mgetw(W)); return 6;             /* LDX  $  */
  l_0xbf: EXT; Mputw(W, X); Tstw(X); return 6;           /* STX  $  */

  l_0xc0: Subc(&B, Mgetc(PC++)); return 2;               /* SUBB #$ */
  l_0xc1: Cmpc(&B, Mgetc(PC++)); return 2;               /* CMPB #$ */
  l_0xc2: Sbc(&B, Mgetc(PC++)); return 2;                /* SBCB #$ */
  l_0xc3: EXT; Addw(&D, W); return 4;                    /* ADDD #$ */
  l_0xc4: Tstc(B &= Mgetc(PC++)); return 2;              /* ANDB #$ */
  l_0xc5: Tstc(B & Mgetc(PC++)); return 2;               /* BITB #$ */
  l_0xc6: Tstc(B = Mgetc(PC++)); return 2;               /* LDB  #$ */
  l_0xc8: Tstc(B ^= Mgetc(PC++)); return 2;              /* EORB #$ */
  l_0xc9: Adc(&B, Mgetc(PC++)); return 2;                /* ADCB #$ */
  l_0xca: Tstc(B |= Mgetc(PC++)); return 2;              /* ORB  #$ */
  l_0xcb: Addc(&B, Mgetc(PC++));return 2;                /* ADDB #$ */
  l_0xcc: EXT; Tstw(D = W); return 3;                    /* LDD  #$ */
  l_0xce: EXT; Tstw(U = W); return 3;                    /* LDU  #$ */

  l_0xd0: DIR; Subc(&B, Mgetc(DA)); return 4;            /* SUBB /$ */
  l_0xd1: DIR; Cmpc(&B, Mgetc(DA)); return 4;            /* CMPB /$ */
  l_0xd2: DIR; Sbc(&B, Mgetc(DA)); return 4;             /* SBCB /$ */
  l_0xd3: DIR; Addw(&D, Mgetw(DA)); return 6;            /* ADDD /$ */
  l_0xd4: DIR; Tstc(B &= Mgetc(DA)); return 4;           /* ANDB /$ */
  l_0xd5: DIR; Tstc(Mgetc(DA) & B); return 4;            /* BITB /$ */
  l_0xd6: DIR; Tstc(B = Mgetc(DA)); return 4;            /* LDB  /$ */
  l_0xd7: DIR; Mputc(DA,B); Tstc(B); return 4;           /* STB  /$ */
  l_0xd8: DIR; Tstc(B ^= Mgetc(DA)); return 4;           /* EORB /$ */
  l_0xd9: DIR; Adc(&B, Mgetc(DA)); return 4;             /* ADCB /$ */
  l_0xda: DIR; Tstc(B |= Mgetc(DA)); return 4;           /* ORB  /$ */
  l_0xdb: DIR; Addc(&B, Mgetc(DA)); return 4;            /* ADDB /$ */
  l_0xdc: DIR; Tstw(D = Mgetw(DA)); return 5;            /* LDD  /$ */
  l_0xdd: DIR; Mputw(DA, D); Tstw(D); return 5;          /* STD  /$ */
  l_0xde: DIR; Tstw(U = Mgetw(DA)); return 5;            /* LDU  /$ */
  l_0xdf: DIR; Mputw(DA, U); Tstw(U); return 5;          /* STU  /$ */

  l_0xe0: IND; Subc(&B, Mgetc(W)); return 4 + n;         /* SUBB IX */
  l_0xe1: IND; Cmpc(&B, Mgetc(W)); return 4 + n;         /* CMPB IX */
  l_0xe2: IND; Sbc(&B, Mgetc(W)); return 4 + n;          /* SBCB IX */
  l_0xe3: IND; Addw(&D, Mgetw(W)); return 6 + n;         /* ADDD IX */
  l_0xe4: IND; Tstc(B &= Mgetc(W)); return 4 + n;        /* ANDB IX */
  l_0xe5: IND; Tstc(Mgetc(W) & B); return 4 + n;         /* BITB IX */
  l_0xe6: IND; Tstc(B = Mgetc(W)); return 4 + n;         /* LDB  IX */
  l_0xe7: IND; Mputc(W, B); Tstc(B); return 4 + n;       /* STB  IX */
  l_0xe8: IND; Tstc(B ^= Mgetc(W)); return 4 + n;        /* EORB IX */
  l_0xe9: IND; Adc(&B, Mgetc(W)); return 4 + n;          /* ADCB IX */
  l_0xea: IND; Tstc(B |= Mgetc(W)); return 4 + n;        /* ORB  IX */
  l_0xeb: IND; Addc(&B, Mgetc(W)); return 4 + n;         /* ADDB IX */
  l_0xec: IND; Tstw(D = Mgetw(W)); return 5 + n;         /* LDD  IX */
  l_0xed: IND; Mputw(W, D); Tstw(D); return 5 + n;       /* STD  IX */
  l_0xee: IND; Tstw(U = Mgetw(W)); return 5 + n;         /* LDU  IX */
  l_0xef: IND; Mputw(W, U); Tstw(U); return 5 + n;       /* STU  IX */

  l_0xf0: EXT; Subc(&B, Mgetc(W)); return 5;             /* SUBB $  */
  l_0xf1: EXT; Cmpc(&B, Mgetc(W)); return 5;             /* CMPB $  */
  l_0xf2: EXT; Sbc(&B, Mgetc(W)); return 5;              /* SBCB $  */
  l_0xf3: EXT; Addw(&D, Mgetw(W)); return 7;             /* ADDD $  */
  l_0xf4: EXT; Tstc(B &= Mgetc(W)); return 5;            /* ANDB $  */
  l_0xf5: EXT; Tstc(B & Mgetc(W)); return 5;             /* BITB $  */
  l_0xf6: EXT; Tstc(B = Mgetc(W)); return 5;             /* LDB  $  */
  l_0xf7: EXT; Mputc(W, B); Tstc(B); return 5;           /* STB  $  */
  l_0xf8: EXT; Tstc(B ^= Mgetc(W)); return 5;            /* EORB $  */
  l_0xf9: EXT; Adc(&B, Mgetc(W)); return 5;              /* ADCB $  */
  l_0xfa: EXT; Tstc(B |= Mgetc(W)); return 5;            /* ORB  $  */
  l_0xfb: EXT; Addc(&B, Mgetc(W)); return 5;             /* ADDB $  */
  l_0xfc: EXT; Tstw(D = Mgetw(W)); return 6;             /* LDD  $  */
  l_0xfd: EXT; Mputw(W, D); Tstw(D); return 6;           /* STD  $  */
  l_0xfe: EXT; Tstw(U = Mgetw(W)); return 6;             /* LDU  $  */
  l_0xff: EXT; Mputw(W, U); Tstw(U); return 6;           /* STU  $  */
  l_0xcf:
  l_0xcd:
  l_0xc7:
  l_0x8f:
  l_0x87:
  l_0x7b:
  l_0x75:
  l_0x72:
  l_0x71:
  l_0x6b:
  l_0x65:
  l_0x62:
  l_0x61:
  l_0x5e:
  l_0x5b:
  l_0x55:
  l_0x52:
  l_0x51:
  l_0x4e:
  l_0x4b:
  l_0x45:
  l_0x42:
  l_0x41:
  l_0x3e:
  l_0x38:
  l_0x1b:
  l_0x18:
  l_0x15:
  l_0x14:
  l_0x0b:
  l_0x05:
  l_0x02: return -code;
 //} 
}
