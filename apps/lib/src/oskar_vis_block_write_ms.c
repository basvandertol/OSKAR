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

#include "apps/lib/oskar_vis_block_write_ms.h"

#include <oskar_measurement_set.h>
#include <oskar_vis_block.h>
#include <oskar_vis_header.h>

#ifdef __cplusplus
extern "C" {
#endif

void oskar_vis_block_write_ms(const oskar_VisBlock* blk,
        const oskar_VisHeader* header, oskar_MeasurementSet* ms, int* status)
{
    const oskar_Mem *in_acorr, *in_xcorr, *in_uu, *in_vv, *in_ww;
    oskar_Mem *temp_vis = 0, *temp_uu = 0, *temp_vv = 0, *temp_ww = 0;
    double dt_dump, t_start_mjd, t_start_sec;
    int a1, a2, num_baselines_in, num_baselines_out, num_pols_in, num_pols_out;
    int num_channels, num_stations, num_times;
    int have_autocorr, b, c, i, i_out, prec, start_row, t;
    const int *baseline_s1, *baseline_s2;
    const void *xcorr, *acorr;
    void* out;

    /* Check all inputs. */
    if (!blk || !header || !ms || !status)
    {
        oskar_set_invalid_argument(status);
        return;
    }

    /* Check if safe to proceed. */
    if (*status) return;

    /* Pull data from visibility structure. */
    num_pols_out     = oskar_ms_num_pols(ms);
    num_pols_in      = oskar_vis_block_num_pols(blk);
    num_stations     = oskar_vis_block_num_stations(blk);
    num_baselines_in = oskar_vis_block_num_baselines(blk);
    num_channels     = oskar_vis_block_num_channels(blk);
    num_times        = oskar_vis_block_num_times(blk);
    dt_dump          = oskar_vis_header_time_inc_sec(header);
    t_start_mjd      = oskar_vis_block_time_start_mjd_utc(blk);
    in_acorr         = oskar_vis_block_auto_correlations_const(blk);
    in_xcorr         = oskar_vis_block_cross_correlations_const(blk);
    in_uu            = oskar_vis_block_baseline_uu_metres_const(blk);
    in_vv            = oskar_vis_block_baseline_vv_metres_const(blk);
    in_ww            = oskar_vis_block_baseline_ww_metres_const(blk);
    baseline_s1      = oskar_vis_block_baseline_station1_const(blk);
    baseline_s2      = oskar_vis_block_baseline_station2_const(blk);
    have_autocorr    = oskar_vis_block_has_auto_correlations(blk);
    prec             = oskar_mem_precision(in_xcorr);
    t_start_sec      = t_start_mjd * 86400.0;

    /* Get number of output baselines. */
    num_baselines_out = num_baselines_in;
    if (have_autocorr)
        num_baselines_out += num_stations;

    /* Check polarisation dimension consistency:
     * num_pols_in can be less than num_pols_out, but not vice-versa. */
    if (num_pols_in > num_pols_out)
    {
        *status = OSKAR_ERR_DIMENSION_MISMATCH;
        return;
    }

    /* Set size of the main table. */
    start_row = oskar_ms_num_rows(ms);
    oskar_ms_set_num_rows(ms, start_row + num_times * num_baselines_out);

    /* Add visibilities and u,v,w coordinates. */
    temp_vis = oskar_mem_create(prec | OSKAR_COMPLEX, OSKAR_CPU,
            num_baselines_out * num_channels * num_pols_out, status);
    temp_uu = oskar_mem_create(prec, OSKAR_CPU, num_baselines_out, status);
    temp_vv = oskar_mem_create(prec, OSKAR_CPU, num_baselines_out, status);
    temp_ww = oskar_mem_create(prec, OSKAR_CPU, num_baselines_out, status);
    xcorr   = oskar_mem_void_const(in_xcorr);
    acorr   = oskar_mem_void_const(in_acorr);
    out     = oskar_mem_void(temp_vis);
    if (prec == OSKAR_DOUBLE)
    {
        const double *uu_in, *vv_in, *ww_in;
        double *uu_out, *vv_out, *ww_out;
        uu_in = oskar_mem_double_const(in_uu, status);
        vv_in = oskar_mem_double_const(in_vv, status);
        ww_in = oskar_mem_double_const(in_ww, status);
        uu_out = oskar_mem_double(temp_uu, status);
        vv_out = oskar_mem_double(temp_vv, status);
        ww_out = oskar_mem_double(temp_ww, status);
        for (t = 0; t < num_times; ++t)
        {
            /* Construct the baseline coordinates for the given time. */
            if (have_autocorr)
            {
                for (a1 = 0, b = 0, i_out = 0; a1 < num_stations; ++a1)
                {
                    uu_out[i_out] = 0.0;
                    vv_out[i_out] = 0.0;
                    ww_out[i_out] = 0.0;
                    for (a2 = a1 + 1, ++i_out;
                            a2 < num_stations; ++a2, ++b, ++i_out)
                    {
                        uu_out[i_out] = uu_in[b];
                        vv_out[i_out] = vv_in[b];
                        ww_out[i_out] = ww_in[b];
                    }
                }
            }
            else
            {
                oskar_mem_copy_contents(temp_uu, in_uu, 0,
                        t * num_baselines_in, num_baselines_in, status);
                oskar_mem_copy_contents(temp_vv, in_vv, 0,
                        t * num_baselines_in, num_baselines_in, status);
                oskar_mem_copy_contents(temp_ww, in_ww, 0,
                        t * num_baselines_in, num_baselines_in, status);
            }

            /* Construct the amplitude data for the given time. */
            if (num_pols_in == 4)
            {
                for (a1 = 0, b = 0, i_out = 0; a1 < num_stations; ++a1)
                {
                    if (have_autocorr)
                    {
                        for (c = 0; c < num_channels; ++c)
                        {
                            i = num_stations * (t * num_channels + c) + a1;
                            ((double4c*)out)[i_out++] =
                                    ((const double4c*)acorr)[i];
                        }
                    }
                    for (a2 = a1 + 1; a2 < num_stations; ++b, ++a2)
                    {
                        for (c = 0; c < num_channels; ++c)
                        {
                            i = num_baselines_in * (t * num_channels + c) + b;
                            ((double4c*)out)[i_out++] =
                                    ((const double4c*)xcorr)[i];
                        }
                    }
                }
            }
            else if (num_pols_in == 1 && num_pols_out == 1)
            {
                for (a1 = 0, b = 0, i_out = 0; a1 < num_stations; ++a1)
                {
                    if (have_autocorr)
                    {
                        for (c = 0; c < num_channels; ++c)
                        {
                            i = num_stations * (t * num_channels + c) + a1;
                            ((double2*)out)[i_out++] =
                                    ((const double2*)acorr)[i];
                        }
                    }
                    for (a2 = a1 + 1; a2 < num_stations; ++b, ++a2)
                    {
                        for (c = 0; c < num_channels; ++c)
                        {
                            i = num_baselines_in * (t * num_channels + c) + b;
                            ((double2*)out)[i_out++] =
                                    ((const double2*)xcorr)[i];
                        }
                    }
                }
            }
            else
            {
                double2 vis_amp, *out_;
                out_ = (double2*)out;
                for (a1 = 0, b = 0, i_out = 0; a1 < num_stations; ++a1)
                {
                    if (have_autocorr)
                    {
                        for (c = 0; c < num_channels; ++c, i_out += 4)
                        {
                            i = num_stations * (t * num_channels + c) + a1;
                            vis_amp = ((const double2*)acorr)[i];
                            out_[i_out + 0] = vis_amp;  /* XX */
                            out_[i_out + 1].x = 0.0;    /* XY */
                            out_[i_out + 1].y = 0.0;    /* XY */
                            out_[i_out + 2].x = 0.0;    /* YX */
                            out_[i_out + 2].y = 0.0;    /* YX */
                            out_[i_out + 3] = vis_amp;  /* YY */
                        }
                    }
                    for (a2 = a1 + 1; a2 < num_stations; ++b, ++a2)
                    {
                        for (c = 0; c < num_channels; ++c, i_out += 4)
                        {
                            i = num_baselines_in * (t * num_channels + c) + b;
                            vis_amp = ((const double2*)xcorr)[i];
                            out_[i_out + 0] = vis_amp;  /* XX */
                            out_[i_out + 1].x = 0.0;    /* XY */
                            out_[i_out + 1].y = 0.0;    /* XY */
                            out_[i_out + 2].x = 0.0;    /* YX */
                            out_[i_out + 2].y = 0.0;    /* YX */
                            out_[i_out + 3] = vis_amp;  /* YY */
                        }
                    }
                }
            }
            oskar_ms_write_all_for_time_d(ms, start_row + t * num_baselines_out,
                    num_baselines_out, uu_out, vv_out, ww_out,
                    (const double*)out, baseline_s1, baseline_s2,
                    oskar_vis_header_time_average_sec(header),
                    dt_dump, t_start_sec + (t * dt_dump));
        }
    }
    else if (prec == OSKAR_SINGLE)
    {
        const float *uu_in, *vv_in, *ww_in;
        float *uu_out, *vv_out, *ww_out;
        uu_in = oskar_mem_float_const(in_uu, status);
        vv_in = oskar_mem_float_const(in_vv, status);
        ww_in = oskar_mem_float_const(in_ww, status);
        uu_out = oskar_mem_float(temp_uu, status);
        vv_out = oskar_mem_float(temp_vv, status);
        ww_out = oskar_mem_float(temp_ww, status);
        for (t = 0; t < num_times; ++t)
        {
            /* Construct the baseline coordinates for the given time. */
            if (have_autocorr)
            {
                for (a1 = 0, b = 0, i_out = 0; a1 < num_stations; ++a1)
                {
                    uu_out[i_out] = 0.0f;
                    vv_out[i_out] = 0.0f;
                    ww_out[i_out] = 0.0f;
                    for (a2 = a1 + 1, ++i_out;
                            a2 < num_stations; ++a2, ++b, ++i_out)
                    {
                        uu_out[i_out] = uu_in[b];
                        vv_out[i_out] = vv_in[b];
                        ww_out[i_out] = ww_in[b];
                    }
                }
            }
            else
            {
                oskar_mem_copy_contents(temp_uu, in_uu, 0,
                        t * num_baselines_in, num_baselines_in, status);
                oskar_mem_copy_contents(temp_vv, in_vv, 0,
                        t * num_baselines_in, num_baselines_in, status);
                oskar_mem_copy_contents(temp_ww, in_ww, 0,
                        t * num_baselines_in, num_baselines_in, status);
            }

            /* Construct the amplitude data for the given time. */
            if (num_pols_in == 4)
            {
                for (a1 = 0, b = 0, i_out = 0; a1 < num_stations; ++a1)
                {
                    if (have_autocorr)
                    {
                        for (c = 0; c < num_channels; ++c)
                        {
                            i = num_stations * (t * num_channels + c) + a1;
                            ((float4c*)out)[i_out++] =
                                    ((const float4c*)acorr)[i];
                        }
                    }
                    for (a2 = a1 + 1; a2 < num_stations; ++b, ++a2)
                    {
                        for (c = 0; c < num_channels; ++c)
                        {
                            i = num_baselines_in * (t * num_channels + c) + b;
                            ((float4c*)out)[i_out++] =
                                    ((const float4c*)xcorr)[i];
                        }
                    }
                }
            }
            else if (num_pols_in == 1 && num_pols_out == 1)
            {
                for (a1 = 0, b = 0, i_out = 0; a1 < num_stations; ++a1)
                {
                    if (have_autocorr)
                    {
                        for (c = 0; c < num_channels; ++c)
                        {
                            i = num_stations * (t * num_channels + c) + a1;
                            ((float2*)out)[i_out++] =
                                    ((const float2*)acorr)[i];
                        }
                    }
                    for (a2 = a1 + 1; a2 < num_stations; ++b, ++a2)
                    {
                        for (c = 0; c < num_channels; ++c)
                        {
                            i = num_baselines_in * (t * num_channels + c) + b;
                            ((float2*)out)[i_out++] =
                                    ((const float2*)xcorr)[i];
                        }
                    }
                }
            }
            else
            {
                float2 vis_amp, *out_;
                out_ = (float2*)out;
                for (a1 = 0, b = 0, i_out = 0; a1 < num_stations; ++a1)
                {
                    if (have_autocorr)
                    {
                        for (c = 0; c < num_channels; ++c, i_out += 4)
                        {
                            i = num_stations * (t * num_channels + c) + a1;
                            vis_amp = ((const float2*)acorr)[i];
                            out_[i_out + 0] = vis_amp;  /* XX */
                            out_[i_out + 1].x = 0.0;    /* XY */
                            out_[i_out + 1].y = 0.0;    /* XY */
                            out_[i_out + 2].x = 0.0;    /* YX */
                            out_[i_out + 2].y = 0.0;    /* YX */
                            out_[i_out + 3] = vis_amp;  /* YY */
                        }
                    }
                    for (a2 = a1 + 1; a2 < num_stations; ++b, ++a2)
                    {
                        for (c = 0; c < num_channels; ++c, i_out += 4)
                        {
                            i = num_baselines_in * (t * num_channels + c) + b;
                            vis_amp = ((const float2*)xcorr)[i];
                            out_[i_out + 0] = vis_amp;  /* XX */
                            out_[i_out + 1].x = 0.0;    /* XY */
                            out_[i_out + 1].y = 0.0;    /* XY */
                            out_[i_out + 2].x = 0.0;    /* YX */
                            out_[i_out + 2].y = 0.0;    /* YX */
                            out_[i_out + 3] = vis_amp;  /* YY */
                        }
                    }
                }
            }
            oskar_ms_write_all_for_time_f(ms, start_row + t * num_baselines_out,
                    num_baselines_out, uu_out, vv_out, ww_out,
                    (const float*)out, baseline_s1, baseline_s2,
                    oskar_vis_header_time_average_sec(header),
                    dt_dump, t_start_sec + (t * dt_dump));
        }
    }
    else
    {
        *status = OSKAR_ERR_BAD_DATA_TYPE;
    }

    /* Cleanup. */
    oskar_mem_free(temp_vis, status);
    oskar_mem_free(temp_uu, status);
    oskar_mem_free(temp_vv, status);
    oskar_mem_free(temp_ww, status);
}

#ifdef __cplusplus
}
#endif
