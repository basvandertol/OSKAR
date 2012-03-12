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

#ifndef OSKAR_STATION_MODEL_SET_ELEMENT_COORDS_H_
#define OSKAR_STATION_MODEL_SET_ELEMENT_COORDS_H_

/**
 * @file oskar_station_model_set_element_coords.h
 */

#include "oskar_global.h"
#include "station/oskar_StationModel.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief
 * Sets the coordinates of a element in the station model.
 *
 * @details
 * This function sets the coordinates of the specified element in the station
 * model, transferring data to the GPU if necessary.
 *
 * @param[in] dst     Station model structure to copy into.
 * @param[in] index   Element array index to set.
 * @param[in] x       Element x position.
 * @param[in] y       Element y position.
 * @param[in] z       Element z position.
 * @param[in] delta_x Element x delta.
 * @param[in] delta_y Element y delta.
 * @param[in] delta_z Element z delta.
 */
OSKAR_EXPORT
int oskar_station_model_set_element_coords(oskar_StationModel* dst,
        int index, double x, double y, double z, double delta_x,
        double delta_y, double delta_z);

#ifdef __cplusplus
}
#endif

#endif /* OSKAR_STATION_MODEL_SET_ELEMENT_COORDS_H_ */