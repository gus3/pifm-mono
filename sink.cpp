// Created by Oliver Mattos and Oskar Weigl.
// Code quality = Totally hacked together.

// Hacked into more manageable pieces by gus3.

// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License version 2 as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// without any warranty, without even the implied warranty of merchantability
// or fitness for a particular purpose.  See the GNU GPL for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.  If not, you may write to the Free Software Foundation,
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
#include "sink.hpp"

// This file is for class definitions.

// These two are virtual, but not pure virtual. As currently structured,
// making them pure virtual wouldn't hurt.

void SampleSink::consume(float* data, int dataLen){}  // floating point samples
void SampleSink::consume(void* data, int dataLen){}  // raw data, len in bytes.

Outputter::Outputter(float rate):
	sleeptime((float)1e6 * BUFFERINSTRUCTIONS/4/rate/2),   // sleep time is half of the time to empty the buffer
	fracerror(0),
	timeErr(0) {
	clocksPerSample = 22500.0 / rate * 1373.5;  // for timing, determined by experiment
	bufPtr=0;
}

void Outputter::consume(float* data, int num) {
	for (int i=0; i<num; i++) {
		float value = data[i]*8;  // modulation index (AKA volume!)
	   
		// dump raw baseband data to stdout for audacity analysis.
		//write(1, &value, 4);
		
		// debug code.  Replaces data with a set of tones.
		//static int debugCount;
		//debugCount++;
		//value = (debugCount & 0x1000)?0.5:0;  // two different tests
		//value += 0.2 * ((debugCount & 0x8)?1.0:-1.0);   // tone
		//if (debugCount & 0x2000) value = 0;   // silence 
		// end debug code
		
		value += fracerror;  // error that couldn't be encoded from last time.
		
		int intval = (int)(round(value));  // integer component
		float frac = (value - (float)intval + 1)/2;
		unsigned int fracval = round(frac*clocksPerSample); // the fractional component
		
		// we also record time error so that if one sample is output
		// for slightly too long, the next sample will be shorter.
		timeErr = timeErr - int(timeErr) + clocksPerSample;
		
		fracerror = (frac - (float)fracval*(1.0-2.3/clocksPerSample)/clocksPerSample)*2;  // error to feed back for delta sigma
		
		// Note, the 2.3 constant is because our PWM isn't perfect.
		// There is a finite time for the DMA controller to load a new value from memory,
		// Therefore the width of each pulse we try to insert has a constant added to it.
		// That constant is about 2.3 bytes written to the serializer, or about 18 cycles.  We use delta sigma
		// to correct for this error and the pwm timing quantization error.
		
		// To reduce noise, rather than just rounding to the nearest clock we can use, we PWM between
		// the two nearest values.
		
		// delay if necessary.  We can also print debug stuff here while not breaking timing.
		static int time;
		time++;
			
		while( (ACCESS(DMABASE + 0x04 /* CurBlock*/) & ~ 0x7F) ==  (int)(instrs[bufPtr].p)) {
			usleep(sleeptime);  // are we anywhere in the next 4 instructions?
		}
		
		// Create DMA command to set clock controller to output FM signal for PWM "LOW" time.
		((struct CB*)(instrs[bufPtr].v))->SOURCE_AD = (int)constPage.p + 2048 + intval*4 - 4 ;
		bufPtr++;
		
		// Create DMA command to delay using serializer module for suitable time.
		((struct CB*)(instrs[bufPtr].v))->TXFR_LEN = (int)timeErr-fracval;
		bufPtr++;
		
		// Create DMA command to set clock controller to output FM signal for PWM "HIGH" time.
		((struct CB*)(instrs[bufPtr].v))->SOURCE_AD = (int)constPage.p + 2048 + intval*4 + 4;
		bufPtr++;
		
		// Create DMA command for more delay.
		((struct CB*)(instrs[bufPtr].v))->TXFR_LEN = fracval;
		bufPtr=(bufPtr+1) % (BUFFERINSTRUCTIONS);
	}
}

PreEmp::PreEmp(float rate, SampleSink* next): 
	fmconstant(rate * 75.0e-6), // for pre-emphisis filter.  75us time constant
	dataold(0),
	next(next) { }	// construction handled in initializers
	
	
void PreEmp::consume(float* data, int num) {
	for (int i=0; i<num; i++) {
		float value = data[i];
		
		float sample = value + (dataold-value) / (1-fmconstant);  // fir of 1 + s tau
		
		next->consume(&sample, 1);
		
		dataold = value;
	}
}

NullSink::NullSink() { }
	
void NullSink::consume(float* data, int num) {}   // throws away data

Mono::Mono(SampleSink* next): next(next) { }
	
void Mono::consume(void* data, int num) {	// expects num%2 == 0
	for (int i=0; i<num/2; i++) {
		float l = (float)(((short*)data)[i]) / 32768.0;
		next->consume( &l, 1);
	}
}
