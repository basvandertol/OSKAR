/*
 * Copyright (c) 2012-2019, The University of Oxford
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

#include "apps/oskar_app_settings.h"
#include "apps/oskar_settings_log.h"
#include "apps/oskar_settings_to_interferometer.h"
#include "apps/oskar_settings_to_sky.h"
#include "apps/oskar_settings_to_telescope.h"
#include "log/oskar_log.h"
#include "settings/oskar_option_parser.h"
#include "interferometer/oskar_interferometer.h"
#include "utility/oskar_get_error_string.h"
#include "utility/oskar_version_string.h"

#include <cstdio>
#include <cstdlib>

using namespace oskar;

static const char app[] = "oskar_sim_interferometer";

int main(int argc, char** argv)
{
    OptionParser opt(app, oskar_version_string(), oskar_app_settings(app));
    opt.add_settings_options();
    opt.add_flag("-q", "Suppress printing.", false, "--quiet");
    if (!opt.check_options(argc, argv)) return EXIT_FAILURE;
    const char* settings = opt.get_arg(0);
    int status = 0;

    // Load the settings file.
    SettingsTree* s = oskar_app_settings_tree(app, settings);
    if (!s)
    {
        oskar_log_error(0, "Failed to read settings file '%s'", settings);
        return EXIT_FAILURE;
    }

    // Get/set setting if necessary.
    if (opt.is_set("--get"))
    {
        printf("%s\n", s->to_string(opt.get_arg(1), &status));
        SettingsTree::free(s);
        return !status ? 0 : EXIT_FAILURE;
    }
    else if (opt.is_set("--set"))
    {
        const char* key = opt.get_arg(1);
        const char* val = opt.get_arg(2);
        bool ok = val ? s->set_value(key, val) : s->set_default(key);
        if (!ok) oskar_log_error(0, "Failed to set '%s'='%s'", key, val);
        SettingsTree::free(s);
        return ok ? 0 : EXIT_FAILURE;
    }

    // Set up the interferometer simulator.
    oskar_Interferometer* sim =
            oskar_settings_to_interferometer(s, NULL, &status);
    oskar_Log* log = oskar_interferometer_log(sim);
    int priority = opt.is_set("-q") ? OSKAR_LOG_WARNING : OSKAR_LOG_STATUS;
    oskar_log_set_term_priority(log, priority);

    // Write settings to log.
    oskar_settings_log(s, log);

    // Set up the sky model and telescope model.
    oskar_Telescope* tel = 0;
    oskar_Sky* sky = oskar_settings_to_sky(s, log, &status);
    if (!sky || status)
        oskar_log_error(log, "Failed to set up sky model: %s.",
                oskar_get_error_string(status));
    else
    {
        tel = oskar_settings_to_telescope(s, log, &status);
        if (!tel || status)
            oskar_log_error(log, "Failed to set up telescope model: %s.",
                    oskar_get_error_string(status));
    }

    // Set sky and telescope models.
    if (sky && tel)
    {
        oskar_interferometer_set_sky_model(sim, sky, &status);
        oskar_interferometer_set_telescope_model(sim, tel, &status);
    }
    oskar_sky_free(sky, &status);
    oskar_telescope_free(tel, &status);

    // Run simulation.
    oskar_interferometer_run(sim, &status);

    // Free memory.
    oskar_interferometer_free(sim, &status);
    SettingsTree::free(s);
    return status;
}
