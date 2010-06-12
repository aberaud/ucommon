// Copyright (C) 1995-1999 David Sugar, Tycho Softworks.
// Copyright (C) 1999-2005 Open Source Telecom Corp.
// Copyright (C) 2005-2010 David Sugar, Tycho Softworks.
//
// This file is part of GNU uCommon C++.
//
// GNU uCommon C++ is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// GNU uCommon C++ is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GNU uCommon C++.  If not, see <http://www.gnu.org/licenses/>.

#include "local.h"
#define _USE_MATH_DEFINES
#include <math.h>

#ifndef	M_PI
#define	M_PI	3.14159265358979323846
#endif

using namespace UCOMMON_NAMESPACE;

volatile timeout_t audio::global = 20;

static uint8_t table_u2a[256] = {
	0x2a, 0x2b, 0x28, 0x29, 0x2e, 0x2f, 0x2c, 0x2d,
	0x22, 0x23, 0x20, 0x21, 0x26, 0x27, 0x24, 0x25,
	0x3a, 0x3b, 0x38, 0x39, 0x3e, 0x3f, 0x3c, 0x3d,
	0x32, 0x33, 0x30, 0x31, 0x36, 0x37, 0x34, 0x35,
	0x0b, 0x08, 0x09, 0x0e, 0x0f, 0x0c, 0x0d, 0x02,
	0x03, 0x00, 0x01, 0x06, 0x07, 0x04, 0x05, 0x1a,
	0x1b, 0x18, 0x19, 0x1e, 0x1f, 0x1c, 0x1d, 0x12,
	0x13, 0x10, 0x11, 0x16, 0x17, 0x14, 0x15, 0x6b,
	0x68, 0x69, 0x6e, 0x6f, 0x6c, 0x6d, 0x62, 0x63,
	0x60, 0x61, 0x66, 0x67, 0x64, 0x65, 0x7b, 0x79,
	0x7e, 0x7f, 0x7c, 0x7d, 0x72, 0x73, 0x70, 0x71,
	0x76, 0x77, 0x74, 0x75, 0x4b, 0x49, 0x4f, 0x4d,
	0x42, 0x40, 0x41, 0x46, 0x47, 0x47, 0x45, 0x5a,
	0x5a, 0x58, 0x58, 0x59, 0x5f, 0x5c, 0x5d, 0x5d,
	0x52, 0x53, 0x53, 0x50, 0x50, 0x51, 0x51, 0x56,
	0x57, 0x57, 0x57, 0x54, 0x54, 0x55, 0xd5, 0xd5,
	0xaa, 0xab, 0xa8, 0xa9, 0xae, 0xaf, 0xac, 0xad,
	0xa2, 0xa3, 0xa0, 0xa1, 0xa6, 0xa7, 0xa4, 0xa5,
	0xba, 0xbb, 0xb8, 0xb9, 0xbe, 0xbf, 0xbc, 0xbd,
	0xb2, 0xb3, 0xb0, 0xb1, 0xb6, 0xb7, 0xb4, 0xb5,
	0x8b, 0x88, 0x88, 0x8e, 0x8f, 0x8f, 0x8d, 0x82,
	0x83, 0x80, 0x80, 0x81, 0x86, 0x84, 0x84, 0x85,
	0x9b, 0x98, 0x99, 0x9e, 0x9f, 0x9c, 0x9d, 0x92,
	0x93, 0x90, 0x91, 0x96, 0x97, 0x94, 0x95, 0xeb,
	0xe8, 0xe9, 0xee, 0xef, 0xec, 0xed, 0xe2, 0xe3,
	0xe0, 0xe1, 0xe6, 0xe7, 0xe4, 0xe5, 0xfb, 0xf8,
	0xfe, 0xff, 0xfc, 0xfd, 0xf2, 0xf3, 0xf0, 0xf1,
	0xf6, 0xf7, 0xf4, 0xf5, 0xca, 0xc8, 0xcf, 0xcd,
	0xc2, 0xc3, 0xc0, 0xc1, 0xc6, 0xc7, 0xc4, 0xc5,
	0xda, 0xdb, 0xd8, 0xd9, 0xde, 0xdf, 0xdc, 0xdd,
	0xd2, 0xd2, 0xd3, 0xd3, 0xd0, 0xd0, 0xd1, 0xd1,
	0xd6, 0xd6, 0xd7, 0xd7, 0xd4, 0xd4, 0xd5, 0xd5};

static uint8_t table_a2u[256] = {
	0x29, 0x2a, 0x27, 0x28, 0x2d, 0x2e, 0x2b, 0x2c,
	0x21, 0x22, 0x22, 0x20, 0x25, 0x26, 0x23, 0x24,
	0x39, 0x3a, 0x37, 0x38, 0x3d, 0x3e, 0x3b, 0x3c,
	0x31, 0x32, 0x2f, 0x30, 0x35, 0x36, 0x33, 0x34,
	0x0a, 0x0b, 0x08, 0x09, 0x0e, 0x0f, 0x0c, 0x0d,
	0x02, 0x03, 0x00, 0x01, 0x06, 0x07, 0x04, 0x05,
	0x1a, 0x1b, 0x18, 0x19, 0x1e, 0x1f, 0x1c, 0x1d,
	0x12, 0x13, 0x10, 0x11, 0x16, 0x17, 0x14, 0x15,
	0x61, 0x62, 0x60, 0x60, 0x60, 0x66, 0x63, 0x65,
	0x65, 0x5d, 0x5d, 0x5c, 0x5c, 0x5f, 0x5f, 0x5e,
	0x74, 0x76, 0x70, 0x72, 0x7c, 0x7d, 0x77, 0x7a,
	0x6a, 0x6b, 0x68, 0x68, 0x6d, 0x6f, 0x6f, 0x6c,
	0x48, 0x49, 0x46, 0x47, 0x4c, 0x4d, 0x4a, 0x4b,
	0x40, 0x41, 0x41, 0x3f, 0x44, 0x45, 0x42, 0x43,
	0x56, 0x57, 0x54, 0x55, 0x5a, 0x5b, 0x58, 0x59,
	0x59, 0x4f, 0x4f, 0x4e, 0x52, 0x53, 0x50, 0x51,
	0xaa, 0xab, 0xa7, 0xa8, 0xae, 0xaf, 0xac, 0xac,
	0xa2, 0xa2, 0xa2, 0xa0, 0xa0, 0xa6, 0xa3, 0xa5,
	0xb9, 0xba, 0xb7, 0xb8, 0xbd, 0xbe, 0xbb, 0xbc,
	0xb1, 0xb2, 0xb2, 0xb0, 0xb5, 0xb6, 0xb3, 0xb4,
	0x8a, 0x8b, 0x88, 0x89, 0x8e, 0x8f, 0x8c, 0x8d,
	0x82, 0x83, 0x80, 0x81, 0x86, 0x87, 0x84, 0x85,
	0x9a, 0x9b, 0x98, 0x99, 0x9e, 0x9f, 0x9c, 0x9d,
	0x92, 0x93, 0x90, 0x91, 0x96, 0x97, 0x94, 0x95,
	0xe2, 0xe3, 0xe0, 0xe1, 0xe6, 0xe7, 0xe4, 0xe5,
	0xdd, 0xdd, 0xdc, 0xdc, 0xdc, 0xdf, 0xdf, 0xde,
	0xf5, 0xf7, 0xf1, 0xf3, 0xfd, 0xff, 0xf9, 0xfb,
	0xea, 0xeb, 0xe8, 0xe9, 0xee, 0xef, 0xec, 0xed,
	0xc8, 0xc9, 0xc6, 0xc7, 0xcc, 0xcd, 0xca, 0xcb,
	0xc0, 0xc1, 0xc1, 0xbf, 0xc4, 0xc5, 0xc2, 0xc3,
	0xd6, 0xd7, 0xd4, 0xd5, 0xda, 0xdb, 0xd8, 0xd9,
	0xcf, 0xcf, 0xcf, 0xce, 0xd2, 0xd3, 0xd0, 0xd1};

static int16_t table_tolinear[256] = {
    -32124, -31100, -30076, -29052, -28028, -27004, -25980, -24956,
    -23932, -22908, -21884, -20860, -19836, -18812, -17788, -16764,
    -15996, -15484, -14972, -14460, -13948, -13436, -12924, -12412,
    -11900, -11388, -10876, -10364,  -9852,  -9340,  -8828,  -8316,
     -7932,  -7676,  -7420,  -7164,  -6908,  -6652,  -6396,  -6140,
     -5884,  -5628,  -5372,  -5116,  -4860,  -4604,  -4348,  -4092,
     -3900,  -3772,  -3644,  -3516,  -3388,  -3260,  -3132,  -3004,
     -2876,  -2748,  -2620,  -2492,  -2364,  -2236,  -2108,  -1980,
     -1884,  -1820,  -1756,  -1692,  -1628,  -1564,  -1500,  -1436,
     -1372,  -1308,  -1244,  -1180,  -1116,  -1052,   -988,   -924,
      -876,   -844,   -812,   -780,   -748,   -716,   -684,   -652,
      -620,   -588,   -556,   -524,   -492,   -460,   -428,   -396,
      -372,   -356,   -340,   -324,   -308,   -292,   -276,   -260,
      -244,   -228,   -212,   -196,   -180,   -164,   -148,   -132,
      -120,   -112,   -104,    -96,    -88,    -80,    -72,    -64,
       -56,    -48,    -40,    -32,    -24,    -16,     -8,      0,
     32124,  31100,  30076,  29052,  28028,  27004,  25980,  24956,
     23932,  22908,  21884,  20860,  19836,  18812,  17788,  16764,
     15996,  15484,  14972,  14460,  13948,  13436,  12924,  12412,
     11900,  11388,  10876,  10364,   9852,   9340,   8828,   8316,
      7932,   7676,   7420,   7164,   6908,   6652,   6396,   6140,
      5884,   5628,   5372,   5116,   4860,   4604,   4348,   4092,
      3900,   3772,   3644,   3516,   3388,   3260,   3132,   3004,
      2876,   2748,   2620,   2492,   2364,   2236,   2108,   1980,
      1884,   1820,   1756,   1692,   1628,   1564,   1500,   1436,
      1372,   1308,   1244,   1180,   1116,   1052,    988,    924,
       876,    844,    812,    780,    748,    716,    684,    652,
       620,    588,    556,    524,    492,    460,    428,    396,
       372,    356,    340,    324,    308,    292,    276,    260,
       244,    228,    212,    196,    180,    164,    148,    132,
       120,    112,    104,     96,     88,     80,     72,     64,
        56,     48,     40,     32,     24,     16,      8,      0};

static int table_toulaw[256] = {
        0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
        4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
        5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7};

void audio::init(void)
{
	static volatile bool initialized = false;

	if(initialized)
		return;

	initialized = true;
	__ccaudio__::g711_init();
}

void audio::u2a(encoded_t ulaw, unsigned size)
{
	while(size--) {
		*ulaw = table_u2a[*ulaw];
		++ulaw;
   }
}

void audio::a2u(encoded_t alaw, unsigned size)
{
	while(size--) {
		*alaw = table_a2u[*alaw];
		++alaw;
	}
}

audio::linear_t audio::expand(encoded_t ulaw, unsigned size, linear_t target)
{
	linear_t done;

	if(!target)
		target = (linear_t)ulaw;

	done = target;
	while(size--)
		*(target++) = table_tolinear[*(ulaw++)];

	return done;
}

audio::encoded_t audio::repack(encoded_t ulaw, unsigned size, linear_t source)
{
	register int sample, sign, exponent, mantissa, retval;

	if(!source)
		source = (linear_t)ulaw;

	while(size--) {
		sample = source[size];
		sign = ( sample >> 8) & 0x80;
		if(sign != 0) 
			sample = -sample;
		sample += 0x84;
		exponent = table_toulaw[(sample >> 7) & 0xff];
		mantissa = (sample >> (exponent + 3)) & 0x0f;
		retval = ~(sign | (exponent << 4) | mantissa);
		if(!retval)
			retval = 0x02;
		ulaw[size] = retval;
	}
	return ulaw;
}

void audio::release(framer_t framer)
{
	if(framer) {
		if(framer->status() != IDLE)
			framer->release();
		delete framer;
	}
}

int16_t audio::dbm(float dbm)
{
    double l = pow(10.0, (dbm - M_PI) /20.0) * (32768.0*0.70711);
    return (int16_t)(l*l);
}

float audio::dbm(int16_t l)
{
    return (float)(log10(sqrt((float)l) / (32768.0*0.70711)) * 20.0 + M_PI);
}

void audio::framer::release(void)
{
	state = IDLE;
}

audio::encoded_t audio::framer::buf(void)
{
	return NULL;
}

unsigned audio::framer::put(void)
{
	return 0;
}

unsigned audio::framer::get(encoded_t data)
{
	return 0;
}

audio::encoded_t audio::framer::get(void)
{
	return NULL;
}

unsigned audio::framer::put(encoded_t data)
{
	return 0;
}

unsigned audio::framer::push(encoded_t data, unsigned size)
{
	return 0;
}

unsigned audio::framer::pull(encoded_t data, unsigned size)
{
	return 0;
}

bool audio::framer::trim(timeout_t backup)
{
	return false;
}

bool audio::framer::seek(timeout_t position)
{
	return false;
}

timeout_t audio::framer::skip(timeout_t offset)
{
	return 0;
}

bool audio::framer::flush(void)
{
	return true;
}

bool audio::framer::fill(void)
{
	return false;
}

bool audio::framer::rewind(void)
{
	return false;
}

bool audio::framer::append(void)
{
	return false;
}

timeout_t audio::framer::length(void)
{
	return 0;
}

timeout_t audio::framer::locate(void)
{
	return 0;
}

audio::framer::operator bool()
{
	return false;
}

