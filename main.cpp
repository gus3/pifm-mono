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

void playWav(char* filename, float samplerate)
{
	int fp= STDIN_FILENO;
	if(filename[0]!='-') fp = open(filename, O_RDONLY);
	
	// the data buffer that gets "consumed" by SampleSink's
	char data[1024];
	
	// data passed from the outside in; each argument is the "next" SampleSink
	SampleSink* ss = new Mono(new PreEmp(samplerate, new Outputter(samplerate)));
	
//	for (int i=0; i<22; i++)
//		read(fp, &data, 2);  // read past header
	read(fp, &data, 44); // read past header
	
	int readBytes;
	while (readBytes = read(fp, &data, 1024))
		ss->consume(data, readBytes);
	close(fp);
}

int main(int argc, char **argv)
{
	float freq = 103.3, srate = 22050;
	char *input;
	
	// parsing args -- not all cases break!
	switch(argc) {
		case 4:	// filename, freq, samplerate
			srate = atof(argv[3]);
		case 3:	// filename, freq
			freq = atof(argv[2]);
		case 2:
			if ((srate > 0) && (freq > 0)) break;
		// if we don't break, there was a problem parsing samplerate or frequency
		default:	// no args, or bad args
			fprintf(stderr, "Usage: %s file.wav [freq] [samplerate]\n", argv[0]);
			fprintf(stderr, "Default frequency is 103.3MHz. Default samplerate is 22050.\n");
			fprintf(stderr, "Specifying '-' for file.wav reads from stdin.\n");
			fprintf(stderr, "This version handles only m16le (mono, 16-bit, little-endian) files.\n");
			return 255;
	}
	setup_fm();
	setupDMA(freq);
	playWav(argv[1], srate);
	
	return 0;

} // main

