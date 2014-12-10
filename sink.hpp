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

#ifndef __SINK_HPP
// Class declarations:

class SampleSink {
public:
	virtual void consume(float* data, int dataLen);  // floating point samples
	virtual void consume(void* data, int dataLen);  // raw data, len in bytes.
};

class Outputter : public SampleSink {
public:
	int bufPtr;
	float clocksPerSample;
	const int sleeptime;
	float fracerror;
	float timeErr;
	
	Outputter(float rate);
	void consume(float* data, int num);
};

class PreEmp : public SampleSink {
public:
	float fmconstant;
	float dataold;
	SampleSink* next;
	
	PreEmp(float rate, SampleSink* next);
	void consume(float* data, int num);
};


class NullSink: public SampleSink {
public:
	
	NullSink();
	void consume(float* data, int num);
};

// decodes a mono wav file
class Mono: public SampleSink {
public:
	SampleSink* next;
	
	Mono(SampleSink* next);
	void consume(void* data, int num);
};

// end of class declarations

#endif // __SINK_HPP
