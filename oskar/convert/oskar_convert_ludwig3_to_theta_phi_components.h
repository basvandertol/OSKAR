/*
 * Copyright (c) 2014-2019, The University of Oxford
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

#ifndef OSKAR_CONVERT_LUDWIG3_TO_THETA_PHI_COMPONENTS_H_
#define OSKAR_CONVERT_LUDWIG3_TO_THETA_PHI_COMPONENTS_H_

/**
 * @file oskar_convert_ludwig3_to_theta_phi_components.h
 */

#include <oskar_global.h>
#include <mem/oskar_mem.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief
 * Convert vector components from the Ludwig-3 system to the theta-phi system.
 *
 * @details
 * This function converts vector components from the Ludwig-3 system (H/V)
 * to the spherical theta-phi system.
 *
 * @param[in] phi          The phi angles, in radians.
 * @param[in,out] vec      The Ludwig-3 (input) and spherical (output) components.
 * @param[in,out] status   Status return code.
 */
OSKAR_EXPORT
void oskar_convert_ludwig3_to_theta_phi_components(
        int num_points, const oskar_Mem* phi, int stride, int offset,
        oskar_Mem* vec, int* status);

#ifdef __cplusplus
}
#endif

#endif /* OSKAR_CONVERT_LUDWIG3_TO_THETA_PHI_COMPONENTS_H_ */
