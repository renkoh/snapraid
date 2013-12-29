/*
 * Copyright (C) 2013 Andrea Mazzoleni
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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

#ifndef __RAID_H
#define __RAID_H

/**
 * RAID mode supporting up to 6 parities.
 *
 * It requires SSSE3 to get good performance with triple or more
 * parities.
 *
 * This is the default mode used by SnapRAID.
 */
#define RAID_MODE_CAUCHY 0

/**
 * RAID mode supporting up to 3 parities,
 *
 * It has a fast triple parity implementation even without SSSE3, but it
 * cannot go beyond it.
 * This is mostly intended for low end CPUs like ARM and AMD Athlon.
 */
#define RAID_MODE_VANDERMONDE 1

/**
 * Max level of parity supported in the default mode RAID_MODE_CAUCHY.
 */
#define RAID_PARITY_MAX 6

/**
 * Maximum number of data disks.
 */
#define RAID_DATA_MAX 251

/**
 * Initializes the RAID system.
 */
void raid_init(void);

/**
 * Sets the mode to use. One of RAID_MODE_*.
 *
 * You can change mode at any time, and it will affect next calls to raid_par(),
 * raid_rec() and raid_recpar().
 *
 * The two modes are compatible for the first two levels of parity.
 * The third one is different.
 */
void raid_mode(int mode);

/**
 * Sets the zero buffer to use in recovering.
 *
 * Before calling raid_rec_dataonly() and raid_recpar() you must provide a memory
 * buffer filled with zero with the same size of the blocks to recover.
 */
void raid_zero(void *zero);

/**
 * Sets an additional buffer used by raid_rec_dataonly() to avoid to overwrite
 * unused parities.
 */
void raid_waste(void *zero);

/**
 * Computes the parity.
 *
 * \param nd Number of data disks.
 * \param np Number of parities to compute.
 * \param size Size of the blocks pointed by ::v. It must be a multipler of 64.
 * \param v Vector of pointers to the blocks for disks and parities.
 *   It has (::nd + ::np) elements. The first elements are the blocks for
 *   data, following with the parity blocks.
 *   Each blocks has ::size bytes.
 */
void raid_par(int nd, int np, size_t size, void **v);

/**
 * Recovers failures of data and parity blocks.
 *
 * All the data and parity blocks marked as bad in the ::id and ::ip vector
 * are recovered and recomputed.
 *
 * The parities blocks to use for recovering are automatically selected from
 * the ones NOT present in the ::ip vector.
 *
 * Ensure to have ::nrd + ::nrp <= ::np, otherwise recovering is not possible.
 *
 * \param nrd Number of failed data disks to recover.
 * \param id[] Vector of ::nrd indexes of the data disks to recover.
 *   The indexes start from 0. They must be in order.
 * \param nrp Number of failed parity disks to recover.
 * \param ip[] Vector of ::nrp indexes of the parity disks to recover.
 *   The indexes start from 0. They must be in order.
 *   All the parities not specified here are assumed correct, and they are
 *   not recomputed.
 * \param nd Number of data disks.
 * \param np Number of parity disks.
 * \param size Size of the blocks pointed by ::v. It must be a multipler of 64.
 * \param v Vector of pointers to the blocks for disks and parities.
 *   It has (::nd + ::np) elements. The first elements are the blocks
 *   for data, following with the parity blocks.
 *   Each blocks has ::size bytes.
 */
void raid_rec(int nrd, const int *id, int nrp, int *ip, int nd, int np, size_t size, void **v);

/**
 * Recovers failures of data blocks using the specified parities.
 *
 * The data blocks marked as bad in the ::id vector are recovered.
 *
 * If you have provided an additional buffer with raid_waste(), the
 * parity blocks are not modified. Without this buffer, the content of
 * parities blocks not specified in the ::ip vector will be destroyed.
 *
 * \param nr Number of failed data disks to recover.
 * \param id[] Vector of ::nr indexes of the data disks to recover.
 *   The indexes start from 0. They must be in order.
 * \param ip[] Vector of ::nr indexes of the parity disks to use for recovering.
 *   The indexes start from 0. They must be in order.
 * \param nd Number of data disks.
 * \param size Size of the blocks pointed by ::v. It must be a multipler of 64.
 * \param v Vector of pointers to the blocks for disks and parities.
 *   It has (::nd + ::ip[::nr - 1] + 1) elements. The first elements are the
 *   blocks for data, following with the parity blocks.
 *   Each blocks has ::size bytes.
 */
void raid_rec_dataonly(int nr, const int *id, const int *ip, int nd, size_t size, void **v);

/**
 * Sorts a small vector of integers.
 *
 * If you have block indexes not in order, you can use this function to sort
 * them before callign raid_rec().
 *
 * \param n Number of integers. No more than RAID_PARITY_MAX.
 * \paran v Vector of integers.
 */
void raid_sort(int n, int *v);

#endif

