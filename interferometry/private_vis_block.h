/*
 * Copyright (c) 2015, The University of Oxford
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the University of Oxford nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef OSKAR_PRIVATE_VIS_BLOCK_H_
#define OSKAR_PRIVATE_VIS_BLOCK_H_

#include <oskar_mem.h>

/*
 * This structure holds visibility data for all baselines, and a set of times
 * and channels.
 *
 * Space for autocorrelation data will always be available, but
 * autocorrelation values may be zero if they are not computed.
 *
 * The dimension order is fixed. The polarisation dimension is implicit in the
 * data type (matrix or scalar) and is therefore the fastest varying.
 * From slowest to fastest varying, the remaining dimensions are:
 *
 * - Time (slowest)
 * - Channel
 * - Baseline (fastest)
 *
 * Note that it is different to that used by earlier versions of OSKAR,
 * where the order of the time and channel dimensions was swapped.
 * In addition, the Measurement Set format swaps the order of the channel
 * and baseline dimensions (so the dimension order there is
 * time, baseline, channel).
 *
 * The number of polarisations is determined by the choice of matrix or
 * scalar amplitude types. Matrix amplitude types represent 4 polarisation
 * dimensions whereas scalar types represent a single polarisation.
 */
struct oskar_VisBlock
{
    int dim_size[4];                  /* Maximum dimension sizes: time, channel, baseline, station. */
    double freq_range_hz[2];          /* Frequencies in the block, in Hz. */
    double time_range_mjd_utc_sec[2]; /* Times in MJD(UTC) seconds. */

    /* Amplitude array has size num_baselines * num_times * num_channels. */
    /* Polarisation dimension is implicit, and therefore fastest varying. */
    /* [complex / complex matrix] */
    oskar_Mem* amplitude;             /* Visibility amplitude. */

    /* Coordinate arrays have sizes num_baselines * num_times. */
    /* [real] */
    oskar_Mem* baseline_uu_metres;    /* Baseline coordinates, in metres. */
    oskar_Mem* baseline_vv_metres;    /* Baseline coordinates, in metres. */
    oskar_Mem* baseline_ww_metres;    /* Baseline coordinates, in metres. */

    /* Dimension arrays have sizes num_baselines. */
    /* [int] */
    /* These should be used to keep track of things for
     * baseline-dependent averaging. */
    oskar_Mem* baseline_num_time_averages;    /* Number of times per baseline. */
    oskar_Mem* baseline_num_channel_averages; /* Number of channels per baseline. */

    /* These arrays hold antenna/station indices for each baseline, and are
     * only used when writing data to a Measurement Set. They are
     * initialised in host memory on creation, and are not saved otherwise. */
    oskar_Mem *a1, *a2;
};

#ifndef OSKAR_VIS_BLOCK_TYPEDEF_
#define OSKAR_VIS_BLOCK_TYPEDEF_
typedef struct oskar_VisBlock oskar_VisBlock;
#endif /* OSKAR_VIS_BLOCK_TYPEDEF_ */

#endif /* OSKAR_PRIVATE_VIS_BLOCK_H_ */