/**************************************************************************\
*                                                                          *
*  Middle Square Weyl Sequence Random Number Generator                     *
*                                                                          *
*  msws() - returns a 32 bit unsigned int [0,0xffffffff]                   *
*                                                                          *
*  The state vector consists of three 64 bit words:  x, w, and s           *
*                                                                          *
*  x - random output [0,0xffffffff]                                        *
*  w - Weyl sequence (period 2^64)                                         *
*  s - an odd constant                                                     *
*                                                                          *
*  The Weyl sequence is added to the square of x.  The middle is extracted *
*  by shifting right 32 bits:                                              *
*                                                                          *
*  x *= x; x += (w += s); return x = (x>>32) | (x<<32);                    *
*                                                                          *
*  The constant s should be set to a random 64-bit pattern with the upper  *
*  32 bits non-zero and the least significant bit set to 1.  There are     *
*  on the order of 2^63 numbers of this type.  Since the stream length     *
*  is 2^64, this provides about 2^127 random numbers in total.             *
*                                                                          *
*  Note:  This version of the RNG includes an idea proposed by             *
*  Richard P. Brent (creator of the xorgens RNG).  Brent suggested         *
*  adding the Weyl sequence after squaring instead of before squaring.     *
*  This provides a basis for uniformity in the output.                     *
*                                                                          *
*  Copyright (c) 2014-2017 Bernard Widynski                                *
*                                                                          *
*  This code can be used under the terms of the GNU General Public License *
*  as published by the Free Software Foundation, either version 3 of the   *
*  License, or any later version. See the GPL license at URL               *
*  http://www.gnu.org/licenses                                             *
*                                                                          *
\**************************************************************************/

/**************************************************************************\
*                                                                          *
*  Modifications applied by LoRd_MuldeR <mulder2@gmx.de>:                  *
*                                                                          *
*  1. Added function to get random 32-Bit value with upper limit           *
*  2. Added function to get random 64-Bit value                            *
*  3. Added function to get random byte-sequence of arbitrary length       *
*  4. Improved initialization (seeding) function                           *
*  5. Made all functions thread-safe by avoiding the use of static vars    *
*  6. Implemented C++ wrapper class, for convenience                       *
*                                                                          *
\**************************************************************************/

#define __STDC_FORMAT_MACROS

#ifdef _MSC_VER
#define _CRT_RAND_S
#define GETPID() _getpid()
#include <io.h>
#include <process.h>
#else
#define GETPID() getpid()
#endif

#ifdef __linux__
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <fcntl.h>

#include "msws.h"

static const uint16_t VERSION[3] = { 1U, 0U, 0U };
static const uint32_t INFINITE = 0U;

static const char *file_name(const char *path)
{
	const char *ptr;
	while (ptr == strchr(path, '/'))  path = ++ptr;
	while (ptr == strchr(path, '\\')) path = ++ptr;
	return path;
}

static uint32_t mkseed(void)
{
	uint32_t seed = 0x8FF46D8E;
#ifdef _MSC_VER
	rand_s(&seed);
#endif
#ifdef __linux__
	const int fd = open("/dev/urandom", O_RDONLY);
	if(fd >= 0)
	{
		size_t rd = read(fd, &seed, sizeof(uint32_t));
		close(fd);
	}
#endif
	seed ^= (((uint32_t)time(NULL)) << 16U);
	seed ^= (((uint32_t)GETPID()) & 0xFFFF);
	return seed;
}

int main(int argc, char *argv[])
{
	bool hex_format = true;
	int arg_offset = 1, rnd_mode = 0;

#ifdef _MSC_VER
	_setmode(_fileno(stdout), _O_BINARY);
#endif

	if ((argc > 1) && (!(strcmp(argv[1], "-h") && strcmp(argv[1], "/?") && strcmp(argv[1], "-?") && strcmp(argv[1], "--help"))))
	{
		printf("\nMiddle Square Weyl Sequence Random Number Generator v%u.%u.%u [%s]\n\n", VERSION[0], VERSION[1], VERSION[2], __DATE__);
		printf("Copyright (c) 2014-2017 Bernard Widynski\n");
		printf("Copyright (c) 2017 LoRd_MuldeR <mulder2@gmx.de>\n\n");
		printf("This program is free software: you can redistribute it and/or modify\n");
		printf("it under the terms of the GNU General Public License as published by\n");
		printf("the Free Software Foundation, either version 3 of the License, or\n");
		printf("(at your option) any later version.\n\n");
		printf("This program is distributed in the hope that it will be useful,\n");
		printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
		printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");
		printf("Usage:\n");
		printf("   %s [switches] [<count> [<seed>]]\n\n", file_name(argv[0]));
		printf("Switches:\n");
		printf("   --uint64 : Output unsigned 64-Bit numeric values (default: unsigned 32-Bit)\n");
		printf("   --decfmt : Output numeric values in decimal format (default: hexadecimal)\n");
		printf("   --binary : Output stream of \"raw\" bytes instead of printing numeric values\n\n");
		printf("Options:\n");
		printf("   <count> : Set the number of values or bytes to generate (default: infinite)\n");
		printf("   <seed>  : Set the value to seed the PRNG (default: seed from system RNG)\n\n");
		printf("NOTE: The same 'seed' value always re-generates the same sequence. Use\n");
		printf("different 'seed' values to generate different sequences. If the 'seed' value\n");
		printf("is *not* specified, a pseudo-random 'seed' is requested from the system.\n\n");
		return EXIT_SUCCESS;
	}

	for(int i = 1; i < argc; ++i)
	{
		if (!strncmp(argv[i], "--", 2U))
		{
			if (!strcmp(argv[i], "--uint64"))
			{
				rnd_mode = 1;
				arg_offset = i + 1;
				continue;
			}
			else if (!strcmp(argv[i], "--binary"))
			{
				rnd_mode = 2;
				arg_offset = i + 1;
				continue;
			}
			else if (!strcmp(argv[i], "--decfmt"))
			{
				hex_format = false;
				arg_offset = i + 1;
				continue;
			}
			else
			{
				fprintf(stderr, "Bad argument: %s\n", argv[i]);
				return EXIT_FAILURE;
			}
		}
		break;
	}

	const uint32_t cntr = (argc > arg_offset) ? (uint32_t)atoll(argv[arg_offset++]) : INFINITE;
	const uint32_t seed = (argc > arg_offset) ? (uint32_t)atoll(argv[arg_offset++]) : mkseed();

	msws::rng rng(seed);
	
	switch (rnd_mode)
	{
	case 0:
		if (cntr == INFINITE)
		{
			for (;;)
			{
				printf(hex_format ? "%08" PRIX32 "\n" : "%08" PRIu32 "\n", rng.uint32());
			}
		}
		else
		{
			for (uint32_t i = 0U; i < cntr; ++i)
			{
				printf(hex_format ? "%08" PRIX32 "\n" : "%08" PRIu32 "\n", rng.uint32());
			}
		}
		break;
	case 1:
		if (cntr == INFINITE)
		{
			for (;;)
			{
				printf(hex_format ? "%016" PRIX64 "\n" : "%016" PRIu64 "\n", rng.uint64());
			}
		}
		else
		{
			for (uint32_t i = 0U; i < cntr; ++i)
			{
				printf(hex_format ? "%016" PRIX64 "\n" : "%016" PRIu64 "\n", rng.uint64());
			}
		}
		break;
	case 2:
		{
			static const size_t BUFF_SIZE = 4096;
			uint8_t buffer[BUFF_SIZE];
			if (cntr == INFINITE)
			{
				for (;;)
				{
					rng.bytes(buffer, BUFF_SIZE);
					if (fwrite(buffer, sizeof(uint8_t), BUFF_SIZE, stdout) != BUFF_SIZE)
					{
						break; /*EOF*/
					}
				}
			}
			else
			{
				uint32_t remain = cntr, bytes = 0U;
				do
				{
					rng.bytes(buffer, bytes = (remain > BUFF_SIZE) ? BUFF_SIZE : remain);
					if (fwrite(buffer, sizeof(uint8_t), bytes, stdout) != bytes)
					{
						break; /*EOF*/
					}
				}
				while ((remain -= bytes) > 0U);
			}
		}
		break;
	}

	return EXIT_SUCCESS;
}
