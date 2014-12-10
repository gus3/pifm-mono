// Created by Oliver Mattos and Oskar Weigl.
// Code quality = Totally hacked together.

// Hacked into more manageable pieces by gus3.

// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License version 2 as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// without any warranty, without even the implied warranty of merchantability
// or fitness for a particular purpose.	See the GNU GPL for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.	If not, you may write to the Free Software Foundation,
// Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

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

#include "support.h"

extern "C" {

// storage definitions
struct PageInfo constPage, instrPage, instrs[BUFFERINSTRUCTIONS];
int mem_fd;
char *gpio_mem, *gpio_map;
volatile unsigned *gpio, *allof7e;

// function definitions

void getRealMemPage(void** vAddr, void** pAddr) {
	void* a = valloc(4096);

	((int*)a)[0] = 1;	// use page to force allocation.
	mlock(a, 4096);	// lock into ram.
	*vAddr = a;	// yay - we know the virtual address
	
	unsigned long long frameinfo;
	
	int fp = open("/proc/self/pagemap", O_RDONLY);
	lseek(fp, ((int)a)/4096*8, SEEK_SET);
	read(fp, &frameinfo, sizeof(frameinfo));
	
	*pAddr = (void*)((int)(frameinfo*4096));
}

void freeRealMemPage(void* vAddr) {
	munlock(vAddr, 4096);	// unlock ram.
	free(vAddr);
}

void setup_fm()
{
	/* open /dev/mem */
	if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
		printf("can't open /dev/mem \n");
		exit (-1);
	}
	
	allof7e = (unsigned *)mmap(
		NULL,
		0x01000000,	//len
		PROT_READ|PROT_WRITE,
		MAP_SHARED,
		mem_fd,
		0x20000000	//base
	);

	if ((int)allof7e==-1) exit(-1);

	SETBIT(GPFSEL0 , 14);
	CLRBIT(GPFSEL0 , 13);
	CLRBIT(GPFSEL0 , 12);
 
	struct GPCTL setupword = {6/*SRC*/, 1, 0, 0, 0, 1,0x5a};

	ACCESS(CM_GP0CTL) = *((int*)&setupword);
}

void unSetupDMA(){
	printf("exiting\n");
	struct DMAregs* DMA0 = (struct DMAregs*)&(ACCESS(DMABASE));
	DMA0->CS =1<<31;	// reset dma controller
	
}

void handSig(int dunno) {
	exit(0);
}

void setupDMA( float centerFreq ){
	atexit(unSetupDMA);
	signal (SIGINT, handSig);
	signal (SIGTERM, handSig);
	signal (SIGHUP, handSig);
	signal (SIGQUIT, handSig);

	// allocate a few pages of ram
	getRealMemPage(&constPage.v, &constPage.p);
	
	int centerFreqDivider = (int)((500.0 / centerFreq) * (float)(1<<12) + 0.5);
	
	// make data page contents - it's essientially 1024 different commands for the
	// DMA controller to send to the clock module at the correct time.
	for (int i=0; i<1024; i++)
	((int*)(constPage.v))[i] = (0x5a << 24) + centerFreqDivider - 512 + i;
	
	int instrCnt = 0;
	
	while (instrCnt<BUFFERINSTRUCTIONS) {
		getRealMemPage(&instrPage.v, &instrPage.p);
		
		// make copy instructions
		struct CB* instr0= (struct CB*)instrPage.v;
		
		for (int i=0; i<4096/sizeof(struct CB); i++) {
			instrs[instrCnt].v = (void*)((int)instrPage.v + sizeof(struct CB)*i);
			instrs[instrCnt].p = (void*)((int)instrPage.p + sizeof(struct CB)*i);
			instr0->SOURCE_AD = (unsigned int)constPage.p+2048;
			instr0->DEST_AD = PWMBASE+0x18 /* FIF1 */;
			instr0->TXFR_LEN = 4;
			instr0->STRIDE = 0;
			//instr0->NEXTCONBK = (int)instrPage.p + sizeof(struct CB)*(i+1);
			instr0->TI = (1/* DREQ	*/<<6) | (5 /* PWM */<<16) |	(1<<26/* no wide*/) ;
			instr0->RES1 = 0;
			instr0->RES2 = 0;
			
			if (!(i%2)) {
				instr0->DEST_AD = CM_GP0DIV;
				instr0->STRIDE = 4;
				instr0->TI = (1<<26/* no wide*/) ;
			}
			
			if (instrCnt!=0) ((struct CB*)(instrs[instrCnt-1].v))->NEXTCONBK = (int)instrs[instrCnt].p;
			instr0++;
			instrCnt++;
		}
	}
	((struct CB*)(instrs[BUFFERINSTRUCTIONS-1].v))->NEXTCONBK = (int)instrs[0].p;
	
	// set up a clock for the PWM
	ACCESS(CLKBASE + 40*4 /*PWMCLK_CNTL*/) = 0x5A000026;
	usleep(1000);
	ACCESS(CLKBASE + 41*4 /*PWMCLK_DIV*/)	= 0x5A002800;
	ACCESS(CLKBASE + 40*4 /*PWMCLK_CNTL*/) = 0x5A000016;
	usleep(1000); 

	// set up pwm
	ACCESS(PWMBASE + 0x0 /* CTRL*/) = 0;
	usleep(1000);
	ACCESS(PWMBASE + 0x4 /* status*/) = -1;	// clear errors
	usleep(1000);
	ACCESS(PWMBASE + 0x0 /* CTRL*/) = -1; //(1<<13 /* Use fifo */) | (1<<10 /* repeat */) | (1<<9 /* serializer */) | (1<<8 /* enable ch */) ;
	usleep(1000);
	ACCESS(PWMBASE + 0x8 /* DMAC*/) = (1<<31 /* DMA enable */) | 0x0707;
	
	//activate dma
	struct DMAregs* DMA0 = (struct DMAregs*)&(ACCESS(DMABASE));
	DMA0->CS =1<<31;	// reset
	DMA0->CONBLK_AD=0; 
	DMA0->TI=0; 
	DMA0->CONBLK_AD = (unsigned int)(instrPage.p);
	DMA0->CS =(1<<0)|(255 <<16);	// enable bit = 0, clear end flag = 1, prio=19-16
}


}; // extern "C"
