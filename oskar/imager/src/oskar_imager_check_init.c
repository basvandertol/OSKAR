/*
 * Copyright (c) 2016-2019, The University of Oxford
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

#include "imager/private_imager.h"
#include "imager/oskar_imager.h"

#include "imager/private_imager_init_dft.h"
#include "imager/private_imager_init_fft.h"
#include "imager/private_imager_init_wproj.h"
#include "utility/oskar_timer.h"

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

void oskar_imager_check_init(oskar_Imager* h, int* status)
{
    if (*status) return;

    /* Allocate empty weights grids if required. */
    if (!h->weights_grids && h->num_planes > 0)
    {
        int i;
        h->weights_grids = (oskar_Mem**)
                calloc(h->num_planes, sizeof(oskar_Mem*));
        for (i = 0; i < h->num_planes; ++i)
            h->weights_grids[i] = oskar_mem_create(h->imager_prec,
                    OSKAR_CPU, 0, status);
    }

    /* Don't continue if we're in "coords only" mode. */
    if (h->coords_only || h->init) return;

    oskar_log_section(h->log, 'M', "Initialising algorithm...");
    oskar_timer_resume(h->tmr_init);
    switch (h->algorithm)
    {
    case OSKAR_ALGORITHM_DFT_2D:
    case OSKAR_ALGORITHM_DFT_3D:
        oskar_imager_init_dft(h, status);
        break;
    case OSKAR_ALGORITHM_FFT:
        oskar_imager_init_fft(h, status);
        break;
    case OSKAR_ALGORITHM_WPROJ:
        oskar_imager_init_wproj(h, status);
        break;
    default:
        *status = OSKAR_ERR_FUNCTION_NOT_AVAILABLE;
    }
    oskar_timer_pause(h->tmr_init);
    h->init = 1;
}

#ifdef __cplusplus
}
#endif
