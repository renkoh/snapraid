/*
 * Copyright (C) 2011,2013 Andrea Mazzoleni
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CPU_H
#define __CPU_H

#if defined(__i386__) || defined(__x86_64__)

static inline void cpuid(uint32_t func, uint32_t* reg)
{
	asm volatile(
#if defined(__i386__) && defined(__PIC__)
		/* allow compilation in PIC mode saving ebx */
		"xchgl %%ebx, %1\n"
		"cpuid\n"
		"xchgl %%ebx, %1\n"
		: "=a" (reg[0]), "=r" (reg[1]), "=c" (reg[2]), "=d" (reg[3])
		: "0" (func)
#else
		"cpuid\n"
		: "=a" (reg[0]), "=b" (reg[1]), "=c" (reg[2]), "=d" (reg[3])
		: "0" (func)
#endif
	);
}

#define CPU_VENDOR_MAX 13

static inline void cpu_info(char* vendor, unsigned* family, unsigned* model)
{
	uint32_t reg[4];
	unsigned f, ef, m, em;

	cpuid(0, reg);

	((uint32_t*)vendor)[0] = reg[1];
	((uint32_t*)vendor)[1] = reg[3];
	((uint32_t*)vendor)[2] = reg[2];
	vendor[12] = 0;

	cpuid(1, reg);

	f = (reg[0] >> 8) & 0xF;
	ef = (reg[0] >> 20) & 0xFF;
	m = (reg[0] >> 4) & 0xF;
	em = (reg[0] >> 16) & 0xF;

	if (strcmp(vendor, "AuthenticAMD") == 0) {
		if (f < 15) {
			*family = f;
			*model = m;
		} else {
			*family = f + ef;
			*model = m + (em << 4);
		}
	} else {
		*family = f + ef;
		*model = m + (em << 4);
	}
}

static inline int cpu_has_mmx(void)
{
	uint32_t reg[4];

	cpuid(1, reg);

	return (reg[3] >> 23) & 1;
}

static inline int cpu_has_sse2(void)
{
	uint32_t reg[4];

	cpuid(1, reg);

	return (reg[3] >> 26) & 1;
}

static inline int cpu_has_ssse3(void)
{
	uint32_t reg[4];

	cpuid(1, reg);

	return (reg[2] >> 9) & 1;
}

static inline int cpu_has_sse42(void)
{
	uint32_t reg[4];

	cpuid(1, reg);

	return (reg[2] >> 20) & 1;
}

static inline int cpu_has_avx(void)
{
	uint32_t reg[4];

	cpuid(1, reg);

	return (reg[2] >> 28) & 1;
}

/**
 * Check if the processor has a slow MULT implementation.
 * If yes, it's better to use a hash not based on multiplication.
 */
static inline int cpu_has_slowmult(void)
{
	char vendor[CPU_VENDOR_MAX];
	unsigned family;
	unsigned model;

	cpu_info(vendor, &family, &model);

	if (strcmp(vendor, "GenuineIntel") == 0) {
		/* Intel(R) Atom(TM) CPU D525 @ 1.80GHz (info from user)
		 * Compiler gcc 4.7.2
		 * CPU GenuineIntel, family 6, model 28, flags mmx sse2 slowmult
		 * Memory is little-endian 64-bit
		 * Speed test with 8 disk and 262144 buffer size...
		 * memset0 1849 [MB/s]
		 * HASH Murmur3 378 [MB/s] (extremely slower than Spooky2)
		 * HASH Spooky2 3413 [MB/s]
		 * RAID5 int32x2 707 [MB/s]
		 * RAID5 mmxx2 1264 [MB/s]
		 * RAID5 mmxx4 1910 [MB/s]
		 * RAID5 sse2x2 2204 [MB/s]
		 * RAID5 sse2x4 2980 [MB/s]
		 * RAID6 int32x2 296 [MB/s]
		 * RAID6 mmxx2 543 [MB/s]
		 * RAID6 sse2x2 1068 [MB/s]
		 * RAID6 sse2x4 1601 [MB/s]
		 */
		if (family == 6 && model == 28)
			return 1;
	}

	return 0;
}

/**
 * Check if the processor has a slow extended set of SSE registers.
 * If yes, it's better to unroll without using the second part of registers.
 */
static inline int cpu_has_slowextendedreg(void)
{
	char vendor[CPU_VENDOR_MAX];
	unsigned family;
	unsigned model;

	cpu_info(vendor, &family, &model);

	if (strcmp(vendor, "AuthenticAMD") == 0) {
		/* AMD Bulldozer (from user)
		 * CPU AuthenticAMD, family 21, model 19, flags mmx sse2
		 * Memory is little-endian 64-bit
		 * Speed test with 8 disk and 262144 buffer size, for a total of 2560 KiB...
		 * memset0 5721 [MB/s]
		 * CRC table 1080 [MB/s]
		 * CRC intel-crc32 2845 [MB/s]
		 * HASH Murmur3 2970 [MB/s]
		 * HASH Spooky2 7503 [MB/s]
		 * RAID5 int32x2 4595 [MB/s]
		 * RAID5 mmxx2 5856 [MB/s]
		 * RAID5 mmxx4 6157 [MB/s]
		 * RAID5 sse2x2 7151 [MB/s]
		 * RAID5 sse2x4 8447 [MB/s]
		 * RAID5 sse2x8 8155 [MB/s] (slower than sse2x4)
		 * RAID6 int32x2 1892 [MB/s]
		 * RAID6 mmxx2 3744 [MB/s]
		 * RAID6 sse2x2 4922 [MB/s]
		 * RAID6 sse2x4 4464 [MB/s] (slower than sse2x2)
		 */
		if (family == 21)
			return 1;
	}

	return 0;
}


/* other cases */

/* Intel(R) Core(TM) i7-3740QM CPU @ 2.70GHz
 * Compiler gcc 4.7.3
 * CPU GenuineIntel, family 6, model 58, flags mmx sse2
 * Memory is little-endian 32-bit
 * memset0 34685 [MB/s]
 * Murmur3 4170 [MB/s]
 * Spooky2 2599 [MB/s]
 * RAID5 int32x2 8919 [MB/s]
 * RAID5 mmxx2 17170 [MB/s]
 * RAID5 sse2x2 27478 [MB/s]
 * RAID6 int32x2 1953 [MB/s]
 * RAID6 mmxx2 7560 [MB/s]
 * RAID6 sse2x2 13930 [MB/s]
 */

/* Intel(R) Core(TM) i7-3740QM CPU @ 2.70GHz
 * Compiler gcc 4.7.3
 * CPU GenuineIntel, family 6, model 58, flags mmx sse2
 * Memory is little-endian 64-bit
 * memset0 26164 [MB/s]
 * Murmur3 4469 [MB/s]
 * Spooky2 12834 [MB/s]
 * RAID5 int32x2 10775 [MB/s]
 * RAID5 mmxx2 19644 [MB/s]
 * RAID5 sse2x2 29168 [MB/s]
 * RAID6 int32x2 3288 [MB/s]
 * RAID6 mmxx2 8374 [MB/s]
 * RAID6 sse2x2 15547 [MB/s]
 */

/* Intel(R) Core(TM) i5 CPU 650 @ 3.20GHz
 * Compiler gcc 4.7.1
 * CPU GenuineIntel, family 6, model 37, flags mmx sse2
 * Memory is little-endian 32-bit
 * memset0 25468 [MB/s]
 * Murmur3 3357 [MB/s]
 * Spooky2 1861 [MB/s]
 * RAID5 int32x2 5043 [MB/s]
 * RAID5 mmxx2 9864 [MB/s]
 * RAID5 sse2x2 16896 [MB/s]
 * RAID6 int32x2 1100 [MB/s]
 * RAID6 mmxx2 5056 [MB/s]
 * RAID6 sse2x2 8726 [MB/s]
 */

/* AMD Athlon(tm) 64 X2 Dual Core Processor 3600+ 1913.377 MHz
 * Compiler gcc 4.7.3
 * CPU AuthenticAMD, family 15, model 107, flags mmx sse2
 * Memory is little-endian 32-bit
 * memset 2414 [MB/s]
 * Murmur3 1213 [MB/s]
 * Spooky2 947 [MB/s]
 * RAID5 int32x2 532 [MB/s]
 * RAID5 mmxx2 1121 [MB/s]
 * RAID5 sse2x2 2061 [MB/s]
 * RAID6 int32x2 280 [MB/s]
 * RAID6 mmxx2 632 [MB/s]
 * RAID6 sse2x2 909 [MB/s]
 */

/* ARM Feroceon 88FR131 rev 1 (v5l)
 * Compiler gcc 4.7.2
 * CPU is not a x86/x64
 * Memory is little-endian 32-bit
 * memset 763 [MB/s]
 * Murmur3 237 [MB/s]
 * Spooky2 216 [MB/s]
 * RAID5 int32x2 233 [MB/s]
 * RAID6 int32x2 103 [MB/s]
 */

/* Intel(R) Xeon(R) CPU E3-1270 V2 @ 3.50GHz $
 * Compiler gcc 4.8.1
 * CPU GenuineIntel, family 6, model 58, flags mmx sse2
 * Memory is little-endian 64-bit
 * memset 27842 [MB/s]
 * Murmur3 4884 [MB/s]
 * Spooky2 14039 [MB/s]
 * RAID5 int32x2 11038 [MB/s]
 * RAID5 mmxx2 20055 [MB/s]
 * RAID5 sse2x2 30703 [MB/s]
 * RAID6 int32x2 3267 [MB/s]
 * RAID6 mmxx2 8883 [MB/s]
 * RAID6 sse2x2 16433 [MB/s]
 */

/* Bobcat/Zacade (info from x264 -> full family 20)
 * AMD E-350 Processor (info from user)
 * Compiler gcc 4.7.3
 * CPU AuthenticAMD, family 20, model 1, flags mmx sse2
 * Memory is little-endian 64-bit
 * memset 2137 [MB/s]
 * Murmur3 1140 [MB/s]
 * Spooky2 2326 [MB/s]
 * RAID5 int32x2 1853 [MB/s]
 * RAID5 mmxx2 2019 [MB/s]
 * RAID5 sse2x2 2908 [MB/s]
 * RAID6 int32x2 884 [MB/s]
 * RAID6 mmxx2 1502 [MB/s]
 * RAID6 sse2x2 1168 [MB/s] (slow version with prefetchnta)
 */

/* AMD Turion(tm) II Neo N40L Dual-Core Processor (info from user)
 * Compiler gcc 4.7.1
 * CPU AuthenticAMD, family 16, model 6, flags mmx sse2
 * Memory is little-endian 64-bit
 * memset 4910 [MB/s]
 * Murmur3 1160 [MB/s]
 * Spooky2 3994 [MB/s]
 * RAID5 int32x2 530 [MB/s]
 * RAID5 mmxx2 1114 [MB/s]
 * RAID5 sse2x1 2127 [MB/s]
 * RAID5 sse2x2 2804 [MB/s]
 * RAID6 int32x2 316 [MB/s]
 * RAID6 mmxx2 655 [MB/s]
 * RAID6 sse2x2 639 [MB/s] (slow version with prefetchnta)
 */

/* AMD Athlon(tm) II X4 620 (info from user)
 * Compiler gcc 4.7.3
 * CPU AuthenticAMD, family 16, model 5, flags mmx sse2
 * Memory is little-endian 64-bit
 * memset 2845 [MB/s]
 * Murmur3 2262 [MB/s]
 * Murmur3x64 3500 [MB/s]
 * Spooky2 4362 [MB/s]
 * Spooky2x86 4000 [MB/s]
 * RAID5 int32x2 1064 [MB/s]
 * RAID5 mmxx2 2064 [MB/s]
 * RAID5 sse2x2 3289 [MB/s]
 * RAID6 int32x2 639 [MB/s]
 * RAID6 mmxx2 1325 [MB/s]
 * RAID6 sse2x2 915 [MB/s] (slow version with prefetchnta)
 */

/* ARM @ 1.2GHz (info from user)
 * /proc/cpuinfo
 * model name      : Feroceon 88FR131 rev 1 (v5l)
 * BogoMIPS        : 1196.85
 * Features        : swp half thumb fastmult edsp
 * CPU implementer : 0x56
 * CPU architecture: 5TE
 * CPU variant     : 0x2
 * CPU part        : 0x131
 * CPU revision    : 1
 * Hardware        : QNAP TS-41x
 *
 * Compiler gcc 4.7.2
 * CPU is not a x86/x64
 * Memory is little-endian 32-bit
 * Speed test with 4 disk and 262144 buffer size...
 * memset 763 [MB/s]
 * Murmur3 237 [MB/s]
 * Spooky2 216 [MB/s]
 * Spooky2x86 418 [MB/s] (dropped experiment of a new hash)
 * RAID5 int32x2 233 [MB/s]
 * RAID6 int32x2 103 [MB/s]
 */
#endif

#endif

