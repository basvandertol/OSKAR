/*
 * Copyright (c) 2013-2020, The University of Oxford
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

#include "telescope/station/private_station.h"
#include "telescope/station/oskar_station.h"
#include "math/oskar_random_gaussian.h"

#ifdef __cplusplus
extern "C" {
#endif

void oskar_station_override_element_phases(oskar_Station* station, int feed,
        unsigned int seed, double phase_std_rad, int* status)
{
    int i;
    if (*status || !station) return;
    if (oskar_station_mem_location(station) != OSKAR_CPU)
    {
        *status = OSKAR_ERR_BAD_LOCATION;
        return;
    }
    const int num = station->num_elements;
    if (oskar_station_has_child(station))
    {
        /* Recursive call to find the last level (i.e. the element data). */
        for (i = 0; i < num; ++i)
        {
            oskar_station_override_element_phases(
                    oskar_station_child(station, i),
                    feed, seed, phase_std_rad, status);
        }
    }
    else
    {
        /* Override element data at last level. */
        oskar_Mem* ptr;
        double r[2];
        const int type = oskar_station_precision(station);
        const int id = oskar_station_unique_id(station);
        ptr = station->element_phase_offset_rad[feed];
        if (!ptr)
        {
            station->element_phase_offset_rad[feed] = oskar_mem_create(
                    type, station->mem_location, num, status);
            ptr = station->element_phase_offset_rad[feed];
        }
        if (type == OSKAR_DOUBLE)
        {
            double* phase = oskar_mem_double(ptr, status);
            for (i = 0; i < num; ++i)
            {
                oskar_random_gaussian2(seed, i, id, r);
                phase[i] = phase_std_rad * r[0];
            }
        }
        else if (type == OSKAR_SINGLE)
        {
            float* phase = oskar_mem_float(ptr, status);
            for (i = 0; i < num; ++i)
            {
                oskar_random_gaussian2(seed, i, id, r);
                phase[i] = phase_std_rad * r[0];
            }
        }
    }
}

#ifdef __cplusplus
}
#endif
