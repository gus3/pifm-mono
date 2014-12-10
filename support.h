// Created by Oliver Mattos and Oskar Weigl.
// Code quality = Totally hacked together.

// Hakced into more manageable pieces by gus3.

// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License version 2 as publised
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// without any warranty, without even the implied warranty of merchantability
// or fitness for a particular purpose.  See the GNU GPL for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.  If not, you may write to the Free Software Foundation,
// Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef __SUPPORT_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <math.h>
#include <fcntl.h>
#include <assert.h>
#include <malloc.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>

extern "C" {

// constants

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

#define PI 3.14159265

#define CM_GP0CTL (0x7e101070)
#define GPFSEL0 (0x7E200000)
#define CM_GP0DIV (0x7e101074)
#define CLKBASE (0x7E101000)
#define DMABASE (0x7E007000)
#define PWMBASE  (0x7e20C000) /* PWM controller */

#define BUFFERINSTRUCTIONS 65536

// structures

struct GPCTL {
	char SRC		 : 4;
	char ENAB		: 1;
	char KILL		: 1;
	char			 : 1;
	char BUSY		: 1;
	char FLIP		: 1;
	char MASH		: 2;
	unsigned int	 : 13;
	char PASSWD	  : 8;
};

struct CB {
	volatile unsigned int TI;
	volatile unsigned int SOURCE_AD;
	volatile unsigned int DEST_AD;
	volatile unsigned int TXFR_LEN;
	volatile unsigned int STRIDE;
	volatile unsigned int NEXTCONBK;
	volatile unsigned int RES1;
	volatile unsigned int RES2;
	
};

struct DMAregs {
	volatile unsigned int CS;
	volatile unsigned int CONBLK_AD;
	volatile unsigned int TI;
	volatile unsigned int SOURCE_AD;
	volatile unsigned int DEST_AD;
	volatile unsigned int TXFR_LEN;
	volatile unsigned int STRIDE;
	volatile unsigned int NEXTCONBK;
	volatile unsigned int DEBUG;
};

struct PageInfo {
	void* p;  // physical address
	void* v;   // virtual address
};

// data declarations

extern struct PageInfo constPage;   
extern struct PageInfo instrPage;
extern struct PageInfo instrs[BUFFERINSTRUCTIONS];

extern int  mem_fd;
extern char *gpio_mem, *gpio_map;

// I/O access
extern volatile unsigned *gpio;
extern volatile unsigned *allof7e;

// macros

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET *(gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio+10) // clears bits which are 1 ignores bits which are 0
#define GPIO_GET *(gpio+13)  // gets   bit levels on pins

#define ACCESS(base) *(volatile int*)((int)allof7e+base-0x7e000000)
#define SETBIT(base, bit) ACCESS(base) |= 1<<bit
#define CLRBIT(base, bit) ACCESS(base) &= ~(1<<bit)

// function prototypes

void getRealMemPage(void** vAddr, void** pAddr);

void freeRealMemPage(void* vAddr);

void setup_fm();

void setupDMA( float centerFreq );

void unSetupDMA();

void handSig(int dunno);

}; // extern "C"

#endif // __SUPPORT_H
