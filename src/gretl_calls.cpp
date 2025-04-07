/*
 *  gretl -- Gnu Regression, Econometrics and Time-series Library
 *  Copyright (C) 2001 Allin Cottrell and Riccardo "Jack" Lucchetti
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "gretl4py.h" /* MUST be at the first position!!! */
#include "gretl4py_data.h"
#include "gretl4py_model.h"
#include <gretl/gretl_drivers.h>
#include <gretl/gretl_errors.h>
#include <gretl/libset.h>
extern "C" {
    #include <gretl/gretl_string_table.h>
}
#include <iostream>
#include <cstdlib>
#include <unordered_map>

#if USE_MAPBOX_VARIANT
# include "mapbox/variant.hpp"
#endif // USE_MAPBOX_VARIANT

#if defined(_WIN32)
# include <gretl/gretl_win32.h>
#endif

#if (defined __GNUC__) && !(defined __clang__)
# define COMPILER_IDENT "GCC " __VERSION__
#else
# define COMPILER_IDENT __VERSION__
#endif

#if defined(_WIN32)
# define LIBGRETL_MIN_VER 50 // it is related with special function for memory free (msvcrt compatibility)
#else
# define LIBGRETL_MIN_VER 49
#endif
#define GRETL_PYBIND11_DEBUG 0

static GretlDataset *DSET = nullptr;                                            // default dataset
static std::unordered_map<int, std::shared_ptr<GretlModel>> estimated_models;   // list of already estimated models
static int gretl4py_verbose = 0;

//////////////////////////////////////////////////
// module related tools
//////////////////////////////////////////////////

static std::string gretl4py_logo (void)
{
    std::string logo = "";

    logo +="                _   _   ___\n";
    logo +="               | | | | /   |\n";
    logo +="  __ _ _ __ ___| |_| |/ /| |_ __  _   _\n";
    logo +=" / _` | '__/ _ \\ __| / /_| | '_ \\| | | |\n";
    logo +="| (_| | | |  __/ |_| \\___  | |_) | |_| |\n";
    logo +=" \\__, |_|  \\___|\\__|_|   |_/ .__/ \\__, |\n";
    logo +="  __/ |                    | |     __/ |\n";
    logo +=" |___/                     |_|    |___/\n";

    return logo;
}

void module_init (void)
{
    if (libgretl_version() < LIBGRETL_MIN_VER) {
        throw std::runtime_error("gretl4py requires libgretl-1.0 version "
                                 + std::to_string(LIBGRETL_MIN_VER) + " or higher");
    }
    char *str = std::getenv("GRETL4PY_VERBOSE");
    if (str) {
        gretl4py_verbose = std::atoi(str);
    }
    libgretl_init();
#if defined(_WIN32)
    win32_cli_read_rc();
#else
    cli_read_rc();
#endif
    if (gretl4py_verbose) {
        std::cout << gretl4py_logo() << std::endl;
        std::printf("libgretl initialized\n");
    }
}

void module_cleanup (void)
{
    libgretl_cleanup();
    if (gretl4py_verbose) {
        std::printf("libgretl cleaned up\n");
    }
}

py::dict about (bool do_print)
{
    GretlBundle b = GretlBundle();
    const std::string lgretl_strings[9] =
        {"dotdir", "gnuplot", "gretldir", "seats", "tramo",
         "tramodir", "workdir", "x12a", "x12adir"};
    std::string mod_ver, mod_logo, lg_ver, py11_ver, sj_ver, sj_name, sj_desc;
    py::dict res, built_in_strings;

    gretl_push_c_numeric_locale();

    mod_ver = std::to_string(GRETL4PY_VERSION).substr(0,4);
    mod_logo = gretl4py_logo();
    py11_ver = std::to_string(PYBIND11_VERSION_MAJOR);
    py11_ver += "." + std::to_string(PYBIND11_VERSION_MINOR);
    py11_ver += "." + std::to_string(PYBIND11_VERSION_PATCH);
    lg_ver = std::to_string(libgretl_version());
    sj_ver = SIMDJSON_VERSION;
    sj_name = simdjson::get_active_implementation()->name();
    sj_desc = simdjson::get_active_implementation()->description();
#if USE_MAPBOX_VARIANT
    std::string mbv_ver;
    mbv_ver = std::to_string(VARIANT_MAJOR_VERSION);
    mbv_ver += "." + std::to_string(VARIANT_MINOR_VERSION);
    mbv_ver += "." + std::to_string(VARIANT_PATCH_VERSION);
#endif // USE_MAPBOX_VARIANT

    // built-in strings
    for (int i=0; i<9; i++ ) {
        built_in_strings[py::cast(lgretl_strings[i])] =
            get_built_in_string_by_name(lgretl_strings[i].c_str());
    }

    res[py::cast("mod_ver")] = mod_ver;
    res[py::cast("mod_logo")] = mod_logo;
    res[py::cast("py11_ver")] = py11_ver;
    res[py::cast("lg_ver")] = lg_ver;
    if (!b.err) {
        py::dict *sysinfo = b.to_dict();
        res[py::cast("sysinfo")] = *sysinfo;
        delete sysinfo;
    } else {
        res[py::cast("sysinfo")] = py::none();
    }
    res[py::cast("built_in_strings")] = built_in_strings;
    res[py::cast("sj_ver")] = sj_ver;
    res[py::cast("sj_name")] = sj_name;
    res[py::cast("sj_desc")] = sj_desc;
#if USE_MAPBOX_VARIANT
    res[py::cast("mbv_ver")] = mbv_ver;
#endif // USE_MAPBOX_VARIANT

    if (do_print || gretl4py_verbose) {
        std::cout << mod_logo << std::endl;
        std::cout << "gretl4py v" << mod_ver;
        std::cout << " [" << COMPILER_IDENT << "]" << std::endl;
        std::cout << "\t+ pybind11     v" << py11_ver << std::endl;
        std::cout << "\t+ simdjson     v" << sj_ver << std::endl;
#if USE_MAPBOX_VARIANT
        std::cout << "\t+ mapbox       v" << mbv_ver << std::endl;
#endif // USE_MAPBOX_VARIANT
        std::cout << "\t+ libgretl-1.0 v" << lg_ver << std::endl;
        if (!b.err) {
            std::cout << std::endl << "libgretl $sysinfo:" << std::endl;
            b.print();
        }
        std::cout << "libgretl built-in strings:" << std::endl;
        for (auto item : built_in_strings) {
            std::cout << "  " << item.first << " = ";
            if (!MapPyToCpp(item.second).to_string().empty()) {
                std::cout << "\"" << item.second << "\"";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl << "simdjson architecture implementation (runtime):" << std::endl;
        std::cout << "  name = " << "\"" << sj_name << "\"" << std::endl;
        std::cout << "  description = " << "\"" << sj_desc << "\"" << std::endl;
    }

    gretl_pop_c_numeric_locale();

    if (b.err) {
        std::cerr << errmsg_get_with_default(b.err) << std::endl;
    }

    return res;
}

// set a string or int valued "setvar"

void set (const std::string key,
          const std::optional<py::object> &pyval)
{
    GretlPrint GP = GretlPrint();
    int err = 0;

    if (pyval == std::nullopt) {
        err = execute_set(key.c_str(), NULL, NULL, OPT_NONE, GP.prn);
    } else {
        err = execute_set(key.c_str(), MapPyToCpp(pyval).to_string().c_str(),
                          NULL, OPT_NONE, GP.prn);
    }

    GP.print();

    if (err) {
        handle_gretl_error(err);
    }
}

//////////////////////////////////////////////////
// data management
//////////////////////////////////////////////////

// signature in gretl4py_data.h
void set_default_dset (GretlDataset *GD)
{
    if (DSET) {
        DSET->is_default = false;
    }
    DSET = GD;
}

//////////////////////////////////////////////////
// models management
//////////////////////////////////////////////////

// wrapper for calling GretlModel_NSE_Driver models
template <auto F, GretlCmdIndex CI>
static py::dict *GretlModel_NSE_Driver_wrapper (const std::string formula, const py::kwargs &kwargs)
{
    std::shared_ptr<GretlModel> model = std::make_shared<GretlModel_NSE_Driver<F, CI>>(DSET);
    estimated_models.try_emplace(model->get_id(), model);

    return model->fit(formula, kwargs);
}

// wrapper for "destructing" GretlModel models
void GretlModel_destructor_wrapper (const int model_id)
{
    if (estimated_models.find(model_id) != estimated_models.end()) {
        estimated_models.at(model_id)->unlink_from_dataset();
        estimated_models.erase(model_id);
    }
# if GRETL_PYBIND11_DEBUG
    std::cout << "GretlModel_destructor_wrapper() called for model id=" << model_id << std::endl;
# endif // GRETL_PYBIND11_DEBUG
}

//////////////////////////////////////////////////
// forecasting
//////////////////////////////////////////////////

void GretlModel_fcast (const int model_id)
{
    if (estimated_models.find(model_id) == estimated_models.end()) {
        std::cerr << "model (id=" << model_id << ") does not exist" << std::endl;
    } else {
        estimated_models.at(model_id)->forecast();
    }
}

//////////////////////////////////////////////////
// model estimation
//////////////////////////////////////////////////
py::dict *arima (const std::string y, const py::tuple &tup,
                 const py::kwargs &kwargs)
{
    // note: custom deleter is needed because of (y, tup) initialisation by constructor
    std::shared_ptr<GretlModel> model =
    std::shared_ptr<GretlModel>(new GretlModel_NSE_arima(DSET, y, tup), [](GretlModel *ptr)
                                {
                                    if (Py_IsInitialized()) {
                                        delete ptr;  // delete manually when Python is alive
                                    }
                                });
    estimated_models.try_emplace(model->get_id(), model);

    return model->fit("", kwargs);
}

py::dict *ols (const std::string formula, const py::kwargs &kwargs)
{
    return GretlModel_NSE_Driver_wrapper<lsq, OLS>(formula, kwargs);
}

py::dict *wls (const std::string formula, const py::kwargs &kwargs)
{
    return GretlModel_NSE_Driver_wrapper<lsq, WLS>(formula, kwargs);
}

py::dict *logit (const std::string formula, const py::kwargs &kwargs)
{
    return GretlModel_NSE_Driver_wrapper<logit_probit, LOGIT>(formula, kwargs);
}

py::dict *probit (const std::string formula, const py::kwargs &kwargs)
{
    return GretlModel_NSE_Driver_wrapper<logit_probit, PROBIT>(formula, kwargs);
}

py::dict *poisson (const std::string formula, const py::kwargs &kwargs)
{
    return GretlModel_NSE_Driver_wrapper<count_model, POISSON>(formula, kwargs);
}

py::dict *negbin (const std::string formula, const py::kwargs &kwargs)
{
    return GretlModel_NSE_Driver_wrapper<count_model, NEGBIN>(formula, kwargs);
}

py::dict *biprobit (const std::string formula, const py::kwargs &kwargs)
{
    return GretlModel_NSE_Driver_wrapper<biprobit_model, BIPROBIT>(formula, kwargs);
}

py::dict *duration (const std::string formula, const py::kwargs &kwargs)
{
    return GretlModel_NSE_Driver_wrapper<duration_model, DURATION>(formula, kwargs);
}

py::dict *heckit (const std::string formula, const py::kwargs &kwargs)
{
    return GretlModel_NSE_Driver_wrapper<heckit_model, HECKIT>(formula, kwargs);
}

py::dict *logistic (const std::string formula, const py::kwargs &kwargs)
{
    return GretlModel_NSE_Driver_wrapper<logistic_driver, LOGISTIC>(formula, kwargs);
}

py::dict *ar (const std::string formula, const py::kwargs &kwargs)
{
    return GretlModel_NSE_Driver_wrapper<ar_model, AR>(formula, kwargs);
}

py::dict *ar1 (const std::string formula, const py::kwargs &kwargs)
{
    return GretlModel_NSE_Driver_wrapper<ar1_model, AR1>(formula, kwargs);
}

py::dict *lad (const std::string formula, const py::kwargs &kwargs)
{
    return GretlModel_NSE_Driver_wrapper<lad_model, LAD>(formula, kwargs);
}

py::dict *garch (const std::string formula, const py::kwargs &kwargs)
{
    return GretlModel_NSE_Driver_wrapper<garch_to_call, GARCH>(formula, kwargs);
}

py::dict *mpols (const std::string formula, const py::kwargs &kwargs)
{
    return GretlModel_NSE_Driver_wrapper<mp_ols, MPOLS>(formula, kwargs);
}

py::dict *panel (const std::string formula, const py::kwargs &kwargs)
{
    return GretlModel_NSE_Driver_wrapper<panel_model, PANEL>(formula, kwargs);
}

py::dict *tsls (const std::string formula, const py::kwargs &kwargs)
{
    return GretlModel_NSE_Driver_wrapper<ivreg, IVREG>(formula, kwargs);
}
