/*
 * Copyright (c) 2011-2015, The University of Oxford
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

#include "ms/oskar_measurement_set.h"

#include <ms/MeasurementSets.h>
#include <tables/Tables.h>
#include <casa/Arrays/Vector.h>
#include <casa/Arrays/Cube.h>

#include <string>
#include <sstream>
#include <vector>
#include <ctime>
#include <cfloat>
#include <cstdlib>

using namespace casa;

/*=============================================================================
 * Local (static) functions
 *---------------------------------------------------------------------------*/

static std::vector<std::string> split_string(const std::string& s, char delim)
{
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> v;
    while (std::getline(ss, item, delim))
    {
        v.push_back(item);
    }
    return v;
}

static double current_utc_to_mjd()
{
    int a, y, m, jdn;
    double day_fraction;
    time_t unix_time;
    struct tm* time_s;

    // Get system UTC.
    unix_time = std::time(NULL);
    time_s = std::gmtime(&unix_time);

    // Compute Julian Day Number (Note: all integer division).
    // Note that tm_mon is in range 0-11, so must add 1.
    a = (14 - (time_s->tm_mon + 1)) / 12;
    y = (time_s->tm_year + 1900) + 4800 - a;
    m = (time_s->tm_mon + 1) + 12 * a - 3;
    jdn = time_s->tm_mday + (153 * m + 2) / 5 + (365 * y) + (y / 4) - (y / 100)
            + (y / 400) - 32045;

    // Compute day fraction.
    day_fraction = time_s->tm_hour / 24.0 + time_s->tm_min / 1440.0 +
            time_s->tm_sec / 86400.0;
    return jdn + day_fraction - 2400000.5 - 0.5;
}

/*=============================================================================
 * Private implementation
 *---------------------------------------------------------------------------*/

struct oskar_MeasurementSet
{
    casa::MeasurementSet* ms;   // Pointer to the Measurement Set.
    casa::MSColumns* msc;       // Pointer to the sub-tables.
    casa::MSMainColumns* msmc;  // Pointer to the main columns.
    std::string app_name;
    bool write_autocorr;
    unsigned int num_pols;
    unsigned int num_channels;
    unsigned int num_stations;
    unsigned int num_receptors;
    double ref_freq;
    double chan_width;
    double phase_centre_ra;
    double phase_centre_dec;
    double start_time;
    double end_time;
    double time_inc_sec;

    oskar_MeasurementSet() : ms(0), msc(0), msmc(0), app_name(std::string()),
            write_autocorr(false), num_pols(0), num_channels(0),
            num_stations(0), num_receptors(2), ref_freq(0.0), chan_width(0.0),
            phase_centre_ra(0.0), phase_centre_dec(0.0), start_time(DBL_MAX),
            end_time(-DBL_MAX), time_inc_sec(0.0) {}
    ~oskar_MeasurementSet();

    void add_band(int pol_id, unsigned int num_channels, double ref_freq,
            const Vector<double>& chan_freqs,
            const Vector<double>& chan_widths);
    void add_field(double ra_rad, double dec_rad);
    void add_history(String message, String origin, double time,
            Vector<String> app_params);
    void add_pol(unsigned int num_pols);
    void add_scratch_cols(bool add_model, bool add_corrected);
    void close();
    void copy_column(String source, String dest);
    bool create(const char* filename, const char* app_name,
            double ra_rad, double dec_rad, unsigned int num_pols,
            unsigned int num_channels, double ref_freq, double chan_width,
            unsigned int num_stations, int write_autocorr, int write_crosscorr);
    template<typename T> void copy_scalar(const String& column,
            unsigned int start_row, unsigned int num_rows,
            size_t data_size_bytes, void* data, size_t* required_size,
            int* status) const;
    template<typename T> void copy_array(const String& column,
            unsigned int start_row, unsigned int num_rows,
            size_t data_size_bytes, void* data, size_t* required_size,
            int* status) const;
    void get_column(const String& column, unsigned int start_row,
            unsigned int num_rows, size_t data_size_bytes,
            void* data, size_t* required_size, int* status) const;
    void get_time_range();
    static bool is_otf_model_defined(const int field,
            const MeasurementSet& ms, String& key, int& source_row);
    static bool is_otf_model_defined(const String& key, const MeasurementSet& ms);
    unsigned int num_rows() const;
    bool open(const char* filename);
    static void remove_otf_model(MeasurementSet& ms);
    static void remove_record_by_key(MeasurementSet& ms, const String& key);
    void set_antenna_feeds();
    void set_num_rows(unsigned int num);
    void set_time_range();
};

/*=============================================================================
 * Public interface
 *---------------------------------------------------------------------------*/

void oskar_ms_add_history(oskar_MeasurementSet* p, const char* origin,
        const char* str, size_t size)
{
    if (!str || size == 0) return;

    // Construct a string from the char array and split on each newline.
    std::vector<std::string> v = split_string(std::string(str, size), '\n');

    // Add to the HISTORY table.
    int num_lines = v.size();
    double current_utc = 86400.0 * current_utc_to_mjd();
    for (int i = 0; i < num_lines; ++i)
    {
        p->add_history(String(v[i]), String(origin), current_utc,
                Vector<String>());
    }
}

void oskar_ms_add_scratch_columns(oskar_MeasurementSet* p, int add_model,
        int add_corrected)
{
    p->add_scratch_cols(add_model, add_corrected);
}

void oskar_ms_copy_column(oskar_MeasurementSet* p, const char* source,
        const char* dest)
{
    p->copy_column(String(source), String(dest));
}

void oskar_ms_set_station_coords_d(oskar_MeasurementSet* p,
        unsigned int num_stations, const double* x, const double* y,
        const double* z)
{
    if (!p->ms || !p->msc) return;
    if (num_stations != p->num_stations) return;

    Vector<Double> pos(3, 0.0);
    for (unsigned int a = 0; a < num_stations; ++a)
    {
        pos(0) = x[a]; pos(1) = y[a]; pos(2) = z[a];
        p->msc->antenna().position().put(a, pos);
        p->msc->antenna().mount().put(a, "ALT-AZ");
        p->msc->antenna().dishDiameter().put(a, 1);
        p->msc->antenna().flagRow().put(a, false);
    }
}

void oskar_ms_set_station_coords_f(oskar_MeasurementSet* p,
        unsigned int num_stations, const float* x, const float* y,
        const float* z)
{
    if (!p->ms || !p->msc) return;
    if (num_stations != p->num_stations) return;

    Vector<Double> pos(3, 0.0);
    for (unsigned int a = 0; a < num_stations; ++a)
    {
        pos(0) = x[a]; pos(1) = y[a]; pos(2) = z[a];
        p->msc->antenna().position().put(a, pos);
        p->msc->antenna().mount().put(a, "ALT-AZ");
        p->msc->antenna().dishDiameter().put(a, 1);
        p->msc->antenna().flagRow().put(a, false);
    }
}

void oskar_ms_close(oskar_MeasurementSet* p)
{
    if (p) delete p; // Calls destructor, which closes everything.
}

oskar_MeasurementSet* oskar_ms_create(const char* filename,
        const char* app_name, double ra_rad, double dec_rad,
        unsigned int num_pols, unsigned int num_channels,
        double ref_freq, double chan_width, unsigned int num_stations,
        int write_autocorr, int write_crosscorr)
{
    oskar_MeasurementSet* p = new oskar_MeasurementSet;
    if (p->create(filename, app_name, ra_rad, dec_rad, num_pols,
            num_channels, ref_freq, chan_width, num_stations, write_autocorr,
            write_crosscorr))
        return p;
    delete p;
    return 0;
}

void oskar_ms_get_column(const oskar_MeasurementSet* p, const char* column,
        unsigned int start_row, unsigned int num_rows,
        size_t data_size_bytes, void* data, size_t* required_size_bytes,
        int* status)
{
    p->get_column(String(column), start_row, num_rows,
            data_size_bytes, data, required_size_bytes, status);
}

unsigned int oskar_ms_num_pols(const oskar_MeasurementSet* p)
{
    return p->num_pols;
}

unsigned int oskar_ms_num_channels(const oskar_MeasurementSet* p)
{
    return p->num_channels;
}

unsigned int oskar_ms_num_rows(const oskar_MeasurementSet* p)
{
    return p->num_rows();
}

unsigned int oskar_ms_num_stations(const oskar_MeasurementSet* p)
{
    return p->num_stations;
}

oskar_MeasurementSet* oskar_ms_open(const char* filename)
{
    oskar_MeasurementSet* p = new oskar_MeasurementSet;
    if (p->open(filename))
        return p;
    delete p;
    return 0;
}

double oskar_ms_phase_centre_ra_rad(const oskar_MeasurementSet* p)
{
    return p->phase_centre_ra;
}

double oskar_ms_phase_centre_dec_rad(const oskar_MeasurementSet* p)
{
    return p->phase_centre_dec;
}

double oskar_ms_ref_freq_hz(const oskar_MeasurementSet* p)
{
    return p->ref_freq;
}

double oskar_ms_channel_width_hz(const oskar_MeasurementSet* p)
{
    return p->chan_width;
}

double oskar_ms_start_time_mjd(const oskar_MeasurementSet* p)
{
    return p->start_time / 86400.0;
}

double oskar_ms_time_inc_sec(const oskar_MeasurementSet* p)
{
    return p->time_inc_sec;
}

void oskar_ms_write_all_for_time_d(oskar_MeasurementSet* p,
        unsigned int start_row, unsigned int num_baselines,
        const double* u, const double* v, const double* w, const double* vis,
        const int* ant1, const int* ant2, double exposure, double interval,
        double time)
{
    MSMainColumns* msmc = p->msmc;
    if (!msmc) return;

    // Allocate storage for a (u,v,w) coordinate and a visibility weight.
    unsigned int num_pols = p->num_pols;
    unsigned int num_channels = p->num_channels;
    Vector<Double> uvw(3);
    Matrix<Complex> vis_data(num_pols, num_channels);
    Vector<Float> weight(num_pols, 1.0);
    Vector<Float> sigma(num_pols, 1.0);

    // Get references to columns.
    ArrayColumn<Double>& col_uvw = msmc->uvw();
    ArrayColumn<Complex>& col_data = msmc->data();
    ScalarColumn<Int>& col_antenna1 = msmc->antenna1();
    ScalarColumn<Int>& col_antenna2 = msmc->antenna2();
    ArrayColumn<Float>& col_weight = msmc->weight();
    ArrayColumn<Float>& col_sigma = msmc->sigma();
    ScalarColumn<Double>& col_exposure = msmc->exposure();
    ScalarColumn<Double>& col_interval = msmc->interval();
    ScalarColumn<Double>& col_time = msmc->time();
    ScalarColumn<Double>& col_timeCentroid = msmc->timeCentroid();

    // Loop over rows / visibilities.
    for (unsigned int r = 0; r < num_baselines; ++r)
    {
        unsigned int row = r + start_row;

        // Get a pointer to the start of the visibility matrix for this row.
        const double* vis_row = vis + (2 * num_pols * num_channels) * r;

        // Fill the visibility matrix (polarisation and channel data).
        for (unsigned int c = 0; c < num_channels; ++c)
        {
            for (unsigned int p = 0; p < num_pols; ++p)
            {
                unsigned int b = 2 * (p + c * num_pols);
                vis_data(p, c) = Complex(vis_row[b], vis_row[b + 1]);
            }
        }

        // Write the data to the Measurement Set.
        uvw(0) = u[r]; uvw(1) = v[r]; uvw(2) = w[r];
        col_uvw.put(row, uvw);
        col_antenna1.put(row, ant1[r]);
        col_antenna2.put(row, ant2[r]);
        col_data.put(row, vis_data);
        col_weight.put(row, weight);
        col_sigma.put(row, sigma);
        col_exposure.put(row, exposure);
        col_interval.put(row, interval);
        col_time.put(row, time);
        col_timeCentroid.put(row, time);
    }

    // Check/update time range.
    if (time < p->start_time) p->start_time = time - interval/2.0;
    if (time > p->end_time) p->end_time = time + interval/2.0;
    p->time_inc_sec = interval;
}

void oskar_ms_write_baselines_d(oskar_MeasurementSet* p,
        unsigned int time_index, unsigned int num_baselines,
        const double* uu, const double* vv, const double* ww, const int* ant1,
        const int* ant2, double exposure, double interval, double time)
{
    MSMainColumns* msmc = p->msmc;
    if (!msmc) return;

    // Allocate storage for a (u,v,w) coordinate and a visibility weight.
    unsigned int num_pols = p->num_pols;
    unsigned int num_channels = p->num_channels;
    Vector<Double> uvw(3);
    Vector<Float> weight(num_pols, 1.0);
    Vector<Float> sigma(num_pols, 1.0);

    // Get references to columns.
    ArrayColumn<Double>& col_uvw = msmc->uvw();
    ScalarColumn<Int>& col_antenna1 = msmc->antenna1();
    ScalarColumn<Int>& col_antenna2 = msmc->antenna2();
    ArrayColumn<Float>& col_weight = msmc->weight();
    ArrayColumn<Float>& col_sigma = msmc->sigma();
    ScalarColumn<Double>& col_exposure = msmc->exposure();
    ScalarColumn<Double>& col_interval = msmc->interval();
    ScalarColumn<Double>& col_time = msmc->time();
    ScalarColumn<Double>& col_timeCentroid = msmc->timeCentroid();

    // Add new rows if required.
    if (oskar_ms_num_rows(p) < (time_index + 1) * num_baselines)
        oskar_ms_set_num_rows(p, (time_index + 1) * num_baselines);

    // Loop over rows to add.
    for (unsigned int r = 0; r < num_baselines; ++r)
    {
        unsigned int row = r + (time_index * num_baselines);

        // Write the data to the Measurement Set.
        uvw(0) = uu[r]; uvw(1) = vv[r]; uvw(2) = ww[r];
        col_uvw.put(row, uvw);
        col_antenna1.put(row, ant1[r]);
        col_antenna2.put(row, ant2[r]);
        col_weight.put(row, weight);
        col_sigma.put(row, sigma);
        col_exposure.put(row, exposure);
        col_interval.put(row, interval);
        col_time.put(row, time);
        col_timeCentroid.put(row, time);
    }

    // Check/update time range.
    if (time < p->start_time) p->start_time = time - interval/2.0;
    if (time > p->end_time) p->end_time = time + interval/2.0;
    p->time_inc_sec = interval;
}

void oskar_ms_write_baselines_f(oskar_MeasurementSet* p,
        unsigned int time_index, unsigned int num_baselines,
        const float* uu, const float* vv, const float* ww, const int* ant1,
        const int* ant2, double exposure, double interval, double time)
{
    MSMainColumns* msmc = p->msmc;
    if (!msmc) return;

    // Allocate storage for a (u,v,w) coordinate and a visibility weight.
    unsigned int num_pols = p->num_pols;
    unsigned int num_channels = p->num_channels;
    Vector<Double> uvw(3);
    Vector<Float> weight(num_pols, 1.0);
    Vector<Float> sigma(num_pols, 1.0);

    // Get references to columns.
    ArrayColumn<Double>& col_uvw = msmc->uvw();
    ScalarColumn<Int>& col_antenna1 = msmc->antenna1();
    ScalarColumn<Int>& col_antenna2 = msmc->antenna2();
    ArrayColumn<Float>& col_weight = msmc->weight();
    ArrayColumn<Float>& col_sigma = msmc->sigma();
    ScalarColumn<Double>& col_exposure = msmc->exposure();
    ScalarColumn<Double>& col_interval = msmc->interval();
    ScalarColumn<Double>& col_time = msmc->time();
    ScalarColumn<Double>& col_timeCentroid = msmc->timeCentroid();

    // Add new rows if required.
    if (oskar_ms_num_rows(p) < (time_index + 1) * num_baselines)
        oskar_ms_set_num_rows(p, (time_index + 1) * num_baselines);

    // Loop over rows to add.
    for (unsigned int r = 0; r < num_baselines; ++r)
    {
        unsigned int row = r + (time_index * num_baselines);

        // Write the data to the Measurement Set.
        uvw(0) = uu[r]; uvw(1) = vv[r]; uvw(2) = ww[r];
        col_uvw.put(row, uvw);
        col_antenna1.put(row, ant1[r]);
        col_antenna2.put(row, ant2[r]);
        col_weight.put(row, weight);
        col_sigma.put(row, sigma);
        col_exposure.put(row, exposure);
        col_interval.put(row, interval);
        col_time.put(row, time);
        col_timeCentroid.put(row, time);
    }

    // Check/update time range.
    if (time < p->start_time) p->start_time = time - interval/2.0;
    if (time > p->end_time) p->end_time = time + interval/2.0;
    p->time_inc_sec = interval;
}

void oskar_ms_write_vis_d(oskar_MeasurementSet* p,
        unsigned int start_time, unsigned int start_channel,
        unsigned int num_times, unsigned int num_channels,
        unsigned int num_baselines, const double* vis)
{
    MSMainColumns* msmc = p->msmc;
    if (!msmc) return;

    // Allocate storage for the block of visibility data.
    unsigned int num_pols = p->num_pols;
    IPosition shape(3, num_pols, num_channels, num_times * num_baselines);
    Array<Complex> vis_data(shape);
    Cube<Complex> cube;
    cube.reference(vis_data);

    // Copy OSKAR visibility data into the CASA container.
    for (unsigned int t = 0; t < num_times; ++t)
    {
        for (unsigned int c = 0; c < num_channels; ++c)
        {
            for (unsigned int b = 0; b < num_baselines; ++b)
            {
                for (unsigned int p = 0; p < num_pols; ++p)
                {
                    unsigned int i = 2 * (num_pols * (num_baselines *
                            (t * num_channels + c) + b) + p);
                    cube(p, c, t * num_baselines + b) =
                            Complex(vis[i], vis[i + 1]);
                }
            }
        }
    }

    // Create the slicers for the column.
    IPosition start1(1, start_time * num_baselines);
    IPosition length1(1, num_times * num_baselines);
    Slicer row_range(start1, length1);
    IPosition start2(2, 0, start_channel);
    IPosition length2(2, num_pols, num_channels);
    Slicer array_section(start2, length2);

    // Write visibilities to DATA column.
    ArrayColumn<Complex>& col_data = msmc->data();
    col_data.putColumnRange(row_range, array_section, vis_data);
}

void oskar_ms_write_vis_f(oskar_MeasurementSet* p,
        unsigned int start_time, unsigned int start_channel,
        unsigned int num_times, unsigned int num_channels,
        unsigned int num_baselines, const float* vis)
{
    MSMainColumns* msmc = p->msmc;
    if (!msmc) return;

    // Allocate storage for the block of visibility data.
    unsigned int num_pols = p->num_pols;
    IPosition shape(3, num_pols, num_channels, num_times * num_baselines);
    Array<Complex> vis_data(shape);
    Cube<Complex> cube;
    cube.reference(vis_data);

    // Copy OSKAR visibility data into the CASA container.
    for (unsigned int t = 0; t < num_times; ++t)
    {
        for (unsigned int c = 0; c < num_channels; ++c)
        {
            for (unsigned int b = 0; b < num_baselines; ++b)
            {
                for (unsigned int p = 0; p < num_pols; ++p)
                {
                    unsigned int i = 2 * (num_pols * (num_baselines *
                            (t * num_channels + c) + b) + p);
                    cube(p, c, t * num_baselines + b) =
                            Complex(vis[i], vis[i + 1]);
                }
            }
        }
    }

    // Create the slicers for the column.
    IPosition start1(1, start_time * num_baselines);
    IPosition length1(1, num_times * num_baselines);
    Slicer row_range(start1, length1);
    IPosition start2(2, 0, start_channel);
    IPosition length2(2, num_pols, num_channels);
    Slicer array_section(start2, length2);

    // Write visibilities to DATA column.
    ArrayColumn<Complex>& col_data = msmc->data();
    col_data.putColumnRange(row_range, array_section, vis_data);
}

void oskar_ms_set_num_rows(oskar_MeasurementSet* p, unsigned int num)
{
    p->set_num_rows(num);
}


/*=============================================================================
 * Private
 *---------------------------------------------------------------------------*/

oskar_MeasurementSet::~oskar_MeasurementSet()
{
    close();
}

void oskar_MeasurementSet::add_band(int pol_id, unsigned int num_channels,
        double ref_freq, const Vector<double>& chan_freqs,
        const Vector<double>& chan_widths)
{
    if (!ms || !msc) return;

    // Add a row to the DATA_DESCRIPTION subtable.
    unsigned int row = ms->dataDescription().nrow();
    ms->dataDescription().addRow();
    msc->dataDescription().spectralWindowId().put(row, row);
    msc->dataDescription().polarizationId().put(row, pol_id);
    msc->dataDescription().flagRow().put(row, false);

    // Get total bandwidth from maximum and minimum.
    Vector<double> startFreqs = chan_freqs - chan_widths / 2.0;
    Vector<double> endFreqs = chan_freqs + chan_widths / 2.0;
    double totalBandwidth = max(endFreqs) - min(startFreqs);

    // Add a row to the SPECTRAL_WINDOW sub-table.
    ms->spectralWindow().addRow();
    MSSpWindowColumns& s = msc->spectralWindow();
    s.measFreqRef().put(row, MFrequency::TOPO);
    s.chanFreq().put(row, chan_freqs);
    s.refFrequency().put(row, ref_freq);
    s.chanWidth().put(row, chan_widths);
    s.effectiveBW().put(row, chan_widths);
    s.resolution().put(row, chan_widths);
    s.flagRow().put(row, false);
    s.freqGroup().put(row, 0);
    s.freqGroupName().put(row, "");
    s.ifConvChain().put(row, 0);
    s.name().put(row, "");
    s.netSideband().put(row, 0);
    s.numChan().put(row, num_channels);
    s.totalBandwidth().put(row, totalBandwidth);
}

void oskar_MeasurementSet::add_field(double ra_rad, double dec_rad)
{
    if (!ms || !msc) return;

    // Set up the field info.
    MVDirection radec(Quantity(ra_rad, "rad"), Quantity(dec_rad, "rad"));
    Vector<MDirection> direction(1);
    direction(0) = MDirection(radec, MDirection::J2000);

    // Add a row to the FIELD table.
    int row = ms->field().nrow();
    ms->field().addRow();
    msc->field().delayDirMeasCol().put(row, direction);
    msc->field().phaseDirMeasCol().put(row, direction);
    msc->field().referenceDirMeasCol().put(row, direction);
    phase_centre_ra = ra_rad;
    phase_centre_dec = dec_rad;
}

void oskar_MeasurementSet::add_history(String message, String origin,
        double time, Vector<String> app_params)
{
    if (!ms || !msc) return;

    int row = ms->history().nrow();
    ms->history().addRow(1);
    MSHistoryColumns& c = msc->history();
    c.message().put(row, message);
    c.application().put(row, app_name.c_str());
    c.priority().put(row, "INFO");
    c.origin().put(row, origin);
    c.time().put(row, time);
    c.observationId().put(row, -1);
    c.appParams().put(row, app_params);
    c.cliCommand().put(row, Vector<String>()); // Required!
}

void oskar_MeasurementSet::add_pol(unsigned int num_pols)
{
    if (!ms || !msc) return;

    // Set up the correlation type, based on number of polarisations.
    Vector<Int> corr_type(num_pols);
    corr_type(0) = Stokes::XX; // Can't be Stokes I if num_pols = 1! (Throws exception.)
    if (num_pols == 2)
    {
        corr_type(1) = Stokes::YY;
    }
    else if (num_pols == 4)
    {
        corr_type(1) = Stokes::XY;
        corr_type(2) = Stokes::YX;
        corr_type(3) = Stokes::YY;
    }

    // Set up the correlation product, based on number of polarisations.
    Matrix<Int> corr_product(2, num_pols);
    for (unsigned int i = 0; i < num_pols; ++i)
    {
        corr_product(0, i) = Stokes::receptor1(Stokes::type(corr_type(i)));
        corr_product(1, i) = Stokes::receptor2(Stokes::type(corr_type(i)));
    }

    // Create a new row, and fill the columns.
    unsigned int row = ms->polarization().nrow();
    ms->polarization().addRow();
    msc->polarization().corrType().put(row, corr_type);
    msc->polarization().corrProduct().put(row, corr_product);
    msc->polarization().numCorr().put(row, num_pols);
}

// Method based on CASA VisSetUtil.cc
void oskar_MeasurementSet::add_scratch_cols(bool add_model, bool add_corrected)
{
    if (!ms) return;

    // Check if columns need adding.
    add_model = add_model &&
            !(ms->tableDesc().isColumn("MODEL_DATA"));
    add_corrected = add_corrected &&
            !(ms->tableDesc().isColumn("CORRECTED_DATA"));

    // Return if there's nothing to be done.
    if (!add_model && !add_corrected)
        return;

    // Remove SORTED_TABLE, because old SORTED_TABLE won't see the new columns.
    if (ms->keywordSet().isDefined("SORT_COLUMNS"))
        ms->rwKeywordSet().removeField("SORT_COLUMNS");
    if (ms->keywordSet().isDefined("SORTED_TABLE"))
        ms->rwKeywordSet().removeField("SORTED_TABLE");

    // Remove any OTF model data from the MS.
    if (add_model)
        remove_otf_model(*ms);

    // Define a column accessor to the observed data.
    ROTableColumn* data;
    if (ms->tableDesc().isColumn(MS::columnName(MS::FLOAT_DATA)))
        data = new ROArrayColumn<Float>(*ms, MS::columnName(MS::FLOAT_DATA));
    else
        data = new ROArrayColumn<Complex>(*ms, MS::columnName(MS::DATA));

    // Check if the data column is tiled and, if so, get the tile shape used.
    TableDesc td = ms->actualTableDesc();
    const ColumnDesc& column_desc = td[data->columnDesc().name()];
    String dataManType = column_desc.dataManagerType();
    String dataManGroup = column_desc.dataManagerGroup();
    IPosition dataTileShape;
    bool tiled = dataManType.contains("Tiled");
    bool simpleTiling = false;

    if (tiled)
    {
        ROTiledStManAccessor tsm(*ms, dataManGroup);
        unsigned int num_hypercubes = tsm.nhypercubes();

        // Find tile shape.
        int highestProduct = -INT_MAX, highestId = 0;
        for (unsigned int i = 0; i < num_hypercubes; i++)
        {
            int product = tsm.getTileShape(i).product();
            if (product > 0 && (product > highestProduct))
            {
                highestProduct = product;
                highestId = i;
            }
        }
        dataTileShape = tsm.getTileShape(highestId);
        simpleTiling = (dataTileShape.nelements() == 3);
    }

    if (!tiled || !simpleTiling)
    {
        // Untiled, or tiled at a higher than expected dimensionality.
        // Use a canonical tile shape of 1 MB size.
        MSSpWindowColumns msspwcol(ms->spectralWindow());
        int max_num_channels = max(msspwcol.numChan().getColumn());
        int tileSize = max_num_channels / 10 + 1;
        int nCorr = data->shape(0)(0);
        dataTileShape = IPosition(3, nCorr,
                tileSize, 131072/nCorr/tileSize + 1);
    }
    delete data;

    if (add_model)
    {
        // Add the MODEL_DATA column.
        TableDesc tdModel;
        String col = MS::columnName(MS::MODEL_DATA);
        tdModel.addColumn(ArrayColumnDesc<Complex>(col, "model data", 2));
        td.addColumn(ArrayColumnDesc<Complex>(col, "model data", 2));
        MeasurementSet::addColumnToDesc(tdModel,
                MeasurementSet::MODEL_DATA, 2);
        TiledShapeStMan tsm("ModelTiled", dataTileShape);
        ms->addColumn(tdModel, tsm);
    }
    if (add_corrected)
    {
        // Add the CORRECTED_DATA column.
        TableDesc tdCorr;
        String col = MS::columnName(MS::CORRECTED_DATA);
        tdCorr.addColumn(ArrayColumnDesc<Complex>(col, "corrected data", 2));
        td.addColumn(ArrayColumnDesc<Complex>(col, "corrected data", 2));
        MeasurementSet::addColumnToDesc(tdCorr,
                MeasurementSet::CORRECTED_DATA, 2);
        TiledShapeStMan tsm("CorrectedTiled", dataTileShape);
        ms->addColumn(tdCorr, tsm);
    }
    ms->flush();
}

void oskar_MeasurementSet::copy_column(String source, String dest)
{
    if (!ms || !msmc) return;

    unsigned int n_rows = num_rows();
    ArrayColumn<Complex>* source_column;
    ArrayColumn<Complex>* dest_column;

    // Get the source column.
    if (source == "DATA")
        source_column = &msmc->data();
    else if (source == "MODEL_DATA")
        source_column = &msmc->modelData();
    else if (source == "CORRECTED_DATA")
        source_column = &msmc->correctedData();
    else
        return;

    // Get the destination column.
    if (dest == "DATA")
        dest_column = &msmc->data();
    else if (dest == "MODEL_DATA")
        dest_column = &msmc->modelData();
    else if (dest == "CORRECTED_DATA")
        dest_column = &msmc->correctedData();
    else
        return;

    // Copy the data.
    for (unsigned int i = 0; i < n_rows; ++i)
    {
        dest_column->put(i, *source_column);
    }
}

void oskar_MeasurementSet::close()
{
    set_time_range();
    if (msmc)
        delete msmc;
    if (msc)
        delete msc;
    if (ms)
        delete ms;
    ms = 0;
    msc = 0;
    msmc = 0;
    num_pols = 0;
    num_channels = 0;
    num_stations = 0;
    phase_centre_ra = 0.0;
    phase_centre_dec = 0.0;
}

bool oskar_MeasurementSet::create(const char* filename, const char* app_name,
        double ra_rad, double dec_rad, unsigned int num_pols,
        unsigned int num_channels, double ref_freq, double chan_width,
        unsigned int num_stations, int write_autocorr, int write_crosscorr)
{
    // Create the table descriptor and use it to set up a new main table.
    TableDesc desc = MS::requiredTableDesc();
    MS::addColumnToDesc(desc, MS::DATA, 2); // Visibilities (2D: pol, chan).
    desc.rwColumnDesc(MS::columnName(MS::DATA)).
            rwKeywordSet().define("UNIT", "Jy");
    IPosition dataShape(2, num_pols, num_channels);
    IPosition weightShape(1, num_pols);
    desc.rwColumnDesc(MS::columnName(MS::DATA)).setShape(dataShape);
    desc.rwColumnDesc(MS::columnName(MS::FLAG)).setShape(dataShape);
    desc.rwColumnDesc(MS::columnName(MS::WEIGHT)).setShape(weightShape);
    desc.rwColumnDesc(MS::columnName(MS::SIGMA)).setShape(weightShape);
    Vector<String> tsmNames(1);
    tsmNames[0] = MS::columnName(MS::DATA);
    desc.defineHypercolumn("TiledData", 3, tsmNames);
    tsmNames[0] = MS::columnName(MS::FLAG);
    desc.defineHypercolumn("TiledFlag", 3, tsmNames);
    tsmNames[0] = MS::columnName(MS::UVW);
    desc.defineHypercolumn("TiledUVW", 2, tsmNames);
    tsmNames[0] = MS::columnName(MS::WEIGHT);
    desc.defineHypercolumn("TiledWeight", 2, tsmNames);
    tsmNames[0] = MS::columnName(MS::SIGMA);
    desc.defineHypercolumn("TiledSigma", 2, tsmNames);
    try
    {
        unsigned int num_baselines = 0;

        if (write_autocorr && write_crosscorr)
            num_baselines = num_stations * (num_stations + 1) / 2;
        else if (!write_autocorr && write_crosscorr)
            num_baselines = num_stations * (num_stations - 1) / 2;
        else if (write_autocorr && !write_crosscorr)
            num_baselines = num_stations;
        else
            return false;

        SetupNewTable newTab(filename, desc, Table::New);

        // Create the default storage managers.
        IncrementalStMan incrStorageManager("ISMData");
        newTab.bindAll(incrStorageManager);
        StandardStMan stdStorageManager("SSMData", 32768, 32768);
        newTab.bindColumn(MS::columnName(MS::ANTENNA1), stdStorageManager);
        newTab.bindColumn(MS::columnName(MS::ANTENNA2), stdStorageManager);

        // Create tiled column storage manager for UVW column.
        IPosition uvwTileShape(2, 3, 2 * num_baselines);
        TiledColumnStMan uvwStorageManager("TiledUVW", uvwTileShape);
        newTab.bindColumn(MS::columnName(MS::UVW), uvwStorageManager);

        // Create tiled column storage managers for WEIGHT and SIGMA columns.
        IPosition weightTileShape(2, num_pols, 2 * num_baselines);
        TiledColumnStMan weightStorageManager("TiledWeight", weightTileShape);
        newTab.bindColumn(MS::columnName(MS::WEIGHT), weightStorageManager);
        IPosition sigmaTileShape(2, num_pols, 2 * num_baselines);
        TiledColumnStMan sigmaStorageManager("TiledSigma", sigmaTileShape);
        newTab.bindColumn(MS::columnName(MS::SIGMA), sigmaStorageManager);

        // Create tiled column storage managers for DATA and FLAG columns.
        IPosition dataTileShape(3, num_pols, num_channels, 2 * num_baselines);
        TiledColumnStMan dataStorageManager("TiledData", dataTileShape);
        newTab.bindColumn(MS::columnName(MS::DATA), dataStorageManager);
        IPosition flagTileShape(3, num_pols, num_channels, 16 * num_baselines);
        TiledColumnStMan flagStorageManager("TiledFlag", flagTileShape);
        newTab.bindColumn(MS::columnName(MS::FLAG), flagStorageManager);

        // Create the Measurement Set.
        ms = new MeasurementSet(newTab, TableLock(TableLock::PermanentLocking));

        // Create SOURCE sub-table.
        TableDesc descSource = MSSource::requiredTableDesc();
        MSSource::addColumnToDesc(descSource, MSSource::REST_FREQUENCY);
        MSSource::addColumnToDesc(descSource, MSSource::POSITION);
        SetupNewTable sourceSetup(ms->sourceTableName(), descSource, Table::New);
        ms->rwKeywordSet().defineTable(MS::keywordName(MS::SOURCE),
                Table(sourceSetup));

        // Create all required default subtables.
        ms->createDefaultSubtables(Table::New);

        // Create the MSMainColumns and MSColumns objects for accessing data
        // in the main table and subtables.
        msc = new MSColumns(*ms);
        msmc = new MSMainColumns(*ms);
        this->app_name = app_name;
    }
    catch (...)
    {
        if (msmc) delete msmc; msmc = 0;
        if (msc) delete msc; msc = 0;
        if (ms) delete ms; ms = 0;
        return false;
    }

    // Add a row to the OBSERVATION subtable.
    const char* username;
    username = getenv("USERNAME");
    if (!username)
        username = getenv("USER");
    ms->observation().addRow();
    Vector<String> corrSchedule(1);
    Vector<Double> timeRange(2, 0.0);
    msc->observation().schedule().put(0, corrSchedule);
    msc->observation().project().put(0, "");
    msc->observation().observer().put(0, username);
    msc->observation().telescopeName().put(0, app_name);
    msc->observation().timeRange().put(0, timeRange);
    set_time_range();

    // Add polarisation ID.
    add_pol(num_pols);

    // Add field data.
    add_field(ra_rad, dec_rad);

    // Set up the band.
    Vector<double> chan_widths(num_channels, chan_width);
    Vector<double> chan_freqs(num_channels);
    //double start = ref_freq - (num_channels - 1) * chan_width / 2.0;
    for (unsigned int c = 0; c < num_channels; ++c)
    {
        chan_freqs(c) = ref_freq + c * chan_width;
    }
    add_band(0, num_channels, ref_freq, chan_freqs, chan_widths);

    // Get a string containing the current system time.
    char time_str[80];
    time_t unix_time;
    unix_time = std::time(NULL);
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d, %H:%M:%S (%Z)",
            std::localtime(&unix_time));

    // Add a row to the HISTORY subtable.
    add_history(String("Measurement Set created at ") + String(time_str),
            app_name, 86400.0 * current_utc_to_mjd(), Vector<String>());

    // Set the private data.
    this->write_autocorr = (bool) write_autocorr;
    this->num_pols = num_pols;
    this->num_channels = num_channels;
    this->num_stations = num_stations;
    this->num_receptors = 2; // By default.
    this->ref_freq = ref_freq;
    this->chan_width = chan_width;

    // Size the ANTENNA and FEED tables.
    ms->antenna().addRow(num_stations);
    ms->feed().addRow(num_stations);
    set_antenna_feeds();

    return true;
}


template<typename T>
void oskar_MeasurementSet::copy_scalar(const String& column,
        unsigned int start_row, unsigned int num_rows,
        size_t data_size_bytes, void* data, size_t* required_size,
        int* status) const
{
    Slice slice(start_row, num_rows, 1);
    ROScalarColumn<T> ac(*ms, column);
    Array<T> a = ac.getColumnRange(slice);
    *required_size = a.size() * sizeof(T);
    if (data_size_bytes >= *required_size)
        memcpy(data, a.data(), *required_size);
    else
        *status = OSKAR_ERR_MS_OUT_OF_RANGE;
}

template<typename T>
void oskar_MeasurementSet::copy_array(const String& column,
        unsigned int start_row, unsigned int num_rows,
        size_t data_size_bytes, void* data, size_t* required_size,
        int* status) const
{
    Slice slice(start_row, num_rows, 1);
    ROArrayColumn<T> ac(*ms, column);
    Array<T> a = ac.getColumnRange(slice);
    *required_size = a.size() * sizeof(T);
    if (data_size_bytes >= *required_size)
        memcpy(data, a.data(), *required_size);
    else
        *status = OSKAR_ERR_MS_OUT_OF_RANGE;
}


void oskar_MeasurementSet::get_column(const String& column,
        unsigned int start_row, unsigned int num_rows,
        size_t data_size_bytes, void* data, size_t* required_size,
        int* status) const
{
    if (*status || !ms) return;

    // Check that the column exists.
    if (!ms->tableDesc().isColumn(column))
    {
        *status = OSKAR_ERR_MS_COLUMN_NOT_FOUND;
        return;
    }

    // Check that some data are selected.
    if (num_rows == 0) return;

    // Check that the row is within the table bounds.
    unsigned int total_rows = ms->nrow();
    if (start_row >= total_rows)
    {
        *status = OSKAR_ERR_MS_OUT_OF_RANGE;
        return;
    }
    if (start_row + num_rows > total_rows)
        num_rows = total_rows - start_row;

    // Get column description and data type.
    const ColumnDesc& cdesc = ms->tableDesc().columnDesc(column);
    DataType dtype = cdesc.dataType();

    if (cdesc.isScalar())
    {
        switch (dtype)
        {
        case TpBool:
            copy_scalar<Bool>(column, start_row, num_rows,
                    data_size_bytes, data, required_size, status); break;
        case TpUChar:
            copy_scalar<uChar>(column, start_row, num_rows,
                    data_size_bytes, data, required_size, status); break;
        case TpShort:
            copy_scalar<Short>(column, start_row, num_rows,
                    data_size_bytes, data, required_size, status); break;
        case TpUShort:
            copy_scalar<uShort>(column, start_row, num_rows,
                    data_size_bytes, data, required_size, status); break;
        case TpInt:
            copy_scalar<Int>(column, start_row, num_rows,
                    data_size_bytes, data, required_size, status); break;
        case TpUInt:
            copy_scalar<uInt>(column, start_row, num_rows,
                    data_size_bytes, data, required_size, status); break;
        case TpFloat:
            copy_scalar<Float>(column, start_row, num_rows,
                    data_size_bytes, data, required_size, status); break;
        case TpDouble:
            copy_scalar<Double>(column, start_row, num_rows,
                    data_size_bytes, data, required_size, status); break;
        case TpComplex:
            copy_scalar<Complex>(column, start_row, num_rows,
                    data_size_bytes, data, required_size, status); break;
        case TpDComplex:
            copy_scalar<DComplex>(column, start_row, num_rows,
                    data_size_bytes, data, required_size, status); break;
        default:
            *status = OSKAR_ERR_MS_UNKNOWN_DATA_TYPE; break;
        }
    }
    else
    {
        switch (dtype)
        {
        case TpBool:
            copy_array<Bool>(column, start_row, num_rows,
                    data_size_bytes, data, required_size, status); break;
        case TpUChar:
            copy_array<uChar>(column, start_row, num_rows,
                    data_size_bytes, data, required_size, status); break;
        case TpShort:
            copy_array<Short>(column, start_row, num_rows,
                    data_size_bytes, data, required_size, status); break;
        case TpUShort:
            copy_array<uShort>(column, start_row, num_rows,
                    data_size_bytes, data, required_size, status); break;
        case TpInt:
            copy_array<Int>(column, start_row, num_rows,
                    data_size_bytes, data, required_size, status); break;
        case TpUInt:
            copy_array<uInt>(column, start_row, num_rows,
                    data_size_bytes, data, required_size, status); break;
        case TpFloat:
            copy_array<Float>(column, start_row, num_rows,
                    data_size_bytes, data, required_size, status); break;
        case TpDouble:
            copy_array<Double>(column, start_row, num_rows,
                    data_size_bytes, data, required_size, status); break;
        case TpComplex:
            copy_array<Complex>(column, start_row, num_rows,
                    data_size_bytes, data, required_size, status); break;
        case TpDComplex:
            copy_array<DComplex>(column, start_row, num_rows,
                    data_size_bytes, data, required_size, status); break;
        default:
            *status = OSKAR_ERR_MS_UNKNOWN_DATA_TYPE; break;
        }
    }
}


// Method based on CASA VisModelData.cc.
bool oskar_MeasurementSet::is_otf_model_defined(const int field,
        const MeasurementSet& ms, String& key, int& source_row)
{
    source_row = -1;
    String mod_key = String("definedmodel_field_") + String::toString(field);
    key = "";
    if (Table::isReadable(ms.sourceTableName()) && ms.source().nrow() > 0)
    {
        if (ms.source().keywordSet().isDefined(mod_key))
        {
            key = ms.source().keywordSet().asString(mod_key);
            if (ms.source().keywordSet().isDefined(key))
                source_row = ms.source().keywordSet().asInt(key);
        }
    }
    else
    {
        if (ms.keywordSet().isDefined(mod_key))
            key = ms.keywordSet().asString(mod_key);
    }
    if (key != "")
        return is_otf_model_defined(key, ms);
    return false;
}

// Method based on CASA VisModelData.cc.
bool oskar_MeasurementSet::is_otf_model_defined(const String& key,
        const MeasurementSet& ms)
{
    // Try the Source table.
    if (Table::isReadable(ms.sourceTableName()) && ms.source().nrow() > 0 &&
            ms.source().keywordSet().isDefined(key))
        return true;

    // Try the Main table.
    if (ms.keywordSet().isDefined(key))
        return true;
    return false;
}

unsigned int oskar_MeasurementSet::num_rows() const
{
    if (!ms) return 0;
    return ms->nrow();
}

bool oskar_MeasurementSet::open(const char* filename)
{
    try
    {
        // Create the MeasurementSet. Storage managers are recreated as needed.
        ms = new MeasurementSet(filename,
                TableLock(TableLock::PermanentLocking), Table::Update);

        // Create the MSMainColumns and MSColumns objects for accessing data
        // in the main table and subtables.
        msc = new MSColumns(*ms);
        msmc = new MSMainColumns(*ms);
    }
    catch (...)
    {
        if (msmc) delete msmc; msmc = 0;
        if (msc) delete msc; msc = 0;
        if (ms) delete ms; ms = 0;
        return false;
    }

    // Get the data dimensions.
    num_pols = 0;
    num_channels = 0;
    if (ms->polarization().nrow() > 0)
        num_pols = msc->polarization().numCorr().get(0);
    if (ms->spectralWindow().nrow() > 0)
    {
        num_channels = msc->spectralWindow().numChan().get(0);
        ref_freq = msc->spectralWindow().refFrequency().get(0);
        chan_width = (msc->spectralWindow().chanWidth().get(0))(IPosition(1, 0));
    }
    num_stations = ms->antenna().nrow();
    if (ms->nrow() > 0)
        time_inc_sec = msc->interval().get(0);

    // Get the phase centre.
    phase_centre_ra = 0.0;
    phase_centre_dec = 0.0;
    if (ms->field().nrow() > 0)
    {
        Vector<MDirection> dir;
        msc->field().phaseDirMeasCol().get(0, dir, true);
        if (dir.size() > 0)
        {
            Vector<Double> v = dir(0).getAngle().getValue();
            phase_centre_ra = v(0);
            phase_centre_dec = v(1);
        }
    }

    // Get the time range.
    get_time_range();
    return true;
}

// Method based on CASA VisModelData.cc.
void oskar_MeasurementSet::remove_otf_model(MeasurementSet& ms)
{
    if (!ms.isWritable())
        return;
    Vector<String> parts(ms.getPartNames(True));
    if (parts.nelements() > 1)
    {
        for (unsigned int k = 0; k < parts.nelements(); ++k)
        {
            MeasurementSet subms(parts[k], ms.lockOptions(), Table::Update);
            remove_otf_model(subms);
        }
        return;
    }

    ROMSColumns msc(ms);
    Vector<Int> fields = msc.fieldId().getColumn();
    int num_fields = GenSort<Int>::sort(fields, Sort::Ascending,
            Sort::HeapSort | Sort::NoDuplicates);
    for (int k = 0; k < num_fields; ++k)
    {
        String key, mod_key;
        int srow;
        if (is_otf_model_defined(fields[k], ms, key, srow))
        {
            mod_key = String("definedmodel_field_") + String::toString(fields[k]);

            // Remove from Source table.
            remove_record_by_key(ms, key);
            if (srow > -1 && ms.source().keywordSet().isDefined(mod_key))
                ms.source().rwKeywordSet().removeField(mod_key);

            // Remove from Main table.
            if (ms.rwKeywordSet().isDefined(mod_key))
                ms.rwKeywordSet().removeField(mod_key);
        }
    }
}

// Method based on CASA VisModelData.cc.
void oskar_MeasurementSet::remove_record_by_key(MeasurementSet& ms,
        const String& key)
{
    if (Table::isReadable(ms.sourceTableName()) && ms.source().nrow() > 0 &&
            ms.source().keywordSet().isDefined(key))
    {
        // Replace the source model with an empty record.
        int row = ms.source().keywordSet().asInt(key);
        TableRecord record;
        MSSourceColumns srcCol(ms.source());
        srcCol.sourceModel().put(row, record);
        ms.source().rwKeywordSet().removeField(key);
    }

    // Remove from Main table.
    if (ms.rwKeywordSet().isDefined(key))
        ms.rwKeywordSet().removeField(key);
}

void oskar_MeasurementSet::set_antenna_feeds()
{
    if (!ms || !msc) return;

    // Determine constants for the FEED subtable.
    Matrix<Double> feedOffset(2, num_receptors, 0.0);
    Matrix<Complex> feedResponse(num_receptors, num_receptors, Complex(0.0, 0.0));
    Vector<String> feedType(num_receptors);
    feedType(0) = "X";
    if (num_receptors > 1) feedType(1) = "Y";
    Vector<Double> feedAngle(num_receptors, 0.0);

    // Fill the FEED subtable (required).
    for (unsigned int a = 0; a < num_stations; ++a)
    {
        msc->feed().antennaId().put(a, a);
        msc->feed().beamOffset().put(a, feedOffset);
        msc->feed().polarizationType().put(a, feedType);
        msc->feed().polResponse().put(a, feedResponse);
        msc->feed().receptorAngle().put(a, feedAngle);
        msc->feed().numReceptors().put(a, num_receptors);
    }
}

void oskar_MeasurementSet::set_num_rows(unsigned int num)
{
    if (!ms) return;

    unsigned int old_num_rows = num_rows();
    unsigned int rows_to_add = num - old_num_rows;
    if (rows_to_add == 0) return;
    ms->addRow(rows_to_add);
}

void oskar_MeasurementSet::get_time_range()
{
    if (!msc) return;
    Vector<Double> range(2, 0.0);
    if (msc->observation().nrow() > 0)
        msc->observation().timeRange().get(0, range);
    start_time = range[0];
    end_time = range[1];
}

void oskar_MeasurementSet::set_time_range()
{
    if (!msc) return;

    // Get the old time range.
    Vector<Double> old_range(2, 0.0);
    msc->observation().timeRange().get(0, old_range);

    // Compute the new time range.
    Vector<Double> range(2, 0.0);
    range[0] = (old_range[0] <= 0.0 || start_time < old_range[0]) ?
            start_time : old_range[0];
    range[1] = (end_time > old_range[1]) ? end_time : old_range[1];
    double release_date = range[1] + 365.25 * 86400.0;

    // Fill observation columns.
    msc->observation().timeRange().put(0, range);
    msc->observation().releaseDate().put(0, release_date);
}
