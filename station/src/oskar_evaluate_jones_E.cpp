/*
 * Copyright (c) 2011, The University of Oxford
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

#include "oskar_global.h"

#include "station/oskar_evaluate_jones_E.h"

#include "sky/oskar_SkyModel.h"
#include "sky/oskar_ra_dec_to_hor_lmn.h"
#include "sky/oskar_cuda_ra_dec_to_hor_lmn.h"

#include "interferometry/oskar_TelescopeModel.h"
#include "math/oskar_Jones.h"
#include "math/oskar_jones_get_station_pointer.h"
#include "station/oskar_evaluate_station_beam.h"
#include "station/oskar_WorkE.h"


#ifdef __cplusplus
extern "C"
#endif
int oskar_evaluate_jones_E(oskar_Jones* E, const oskar_SkyModel* sky,
        const oskar_TelescopeModel* telescope, const double /*gast*/,
        oskar_WorkE* work)
{
    // Consistency and validation checks on input arguments.
    // -------------------------------------------------------------------------
    if (E == NULL || sky == NULL || telescope == NULL)
        return OSKAR_ERR_INVALID_ARGUMENT;

    if (telescope->num_stations == 0)
        return OSKAR_ERR_DIMENSION_MISMATCH;

    if (telescope->station == NULL)
        return OSKAR_ERR_INVALID_ARGUMENT;

    // TODO input argument validation
    // - Check the sky, telescope and Jones for type consistency.
    //    - double / single precision consistency
    //    - format of E (scalar, matrix) with presence of element pattern data.
    // - Work out what checking has to be done here and what can be delayed to
    //   next level wrapper functions.


    // Evaluate the station beam for each station for each source position.
    // ------------------------------------------------------------------------
    if (telescope->identical_stations && telescope->use_common_sky)
    {
        // Evaluate horizontal l,m,n once and the station beam for
        // station 0 and copy the result into the data for other stations
        // in E.
        oskar_StationModel* station0 = &telescope->station[0];

        // Evaluate the horizontal l,m,m coordinates for beam phase centre
        // and sources.
        // TODO --> oskar_evalute_beam_hor_lmn(station0, gast, work);
        // oskar_cuda_ra_dec_to_hor_lmn_d(m, ra, dec, lst, lat, hor_l, hor_m, hor_n);
        // TODO --> oskar_evalute_source_hor_lmn(sky, station0, gast, work);

        // work:
        //  weights
        //  signal
        //  hor_l;
        //  hor_m;
        //  hor_n;

        // Evaluate the station beam.
        oskar_Mem E0; // Pointer to the row of E for station 0.
        oskar_jones_get_station_pointer(&E0, E, 0);
        oskar_evaluate_station_beam(&E0, sky, station0, work);

        //  (zero sources below horizon)
    }
    else
    {
        if (telescope->use_common_sky)
        {
            // Evaluate horizontal l,m,n once and use it for evaluating
            // the station beam for each station.
            // TODO --> oskar_evalute_source_hor_lmn(sky, station0, gast, work);
            // TODO --> oskar_evalute_beam_hor_lmn(station0, gast, work);

            // loop over stations to evaluate E.
            for (int i = 0; i < telescope->num_stations; ++i)
            {
                //oskar_StationModel* station = &telescope->station[i];
                //oskar_evaluate_station_beam(&E0, sky, station, EWork);
            }
        }
        else
        {
            // Evaluate horizontal l,m,n and the station beam for each station.
            // loop over stations to evaluate E.
            for (int i = 0; i < telescope->num_stations; ++i)
            {
                //oskar_StationModel* station = &telescope->station[i];

            }
        }
    }
    return 0;
}

