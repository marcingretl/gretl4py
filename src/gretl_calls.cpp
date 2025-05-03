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

#if defined(_WIN32)
# include <gretl/gretl_win32.h>
#endif

#if (defined __GNUC__) && !(defined __clang__)
# define COMPILER_IDENT "GCC " __VERSION__
#else
# define COMPILER_IDENT __VERSION__
#endif

#define LIBGRETL_MIN_VER 51

#define GRETL_PYBIND11_DEBUG 0

std::shared_ptr<GretlDataset> DSET = nullptr;                           // default dataset
std::unordered_map<int, std::unique_ptr<GretlModel>> estimated_models;  // list of already estimated models
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
        res[py::cast("sysinfo")] = b.to_dict();
    } else {
        res[py::cast("sysinfo")] = py::none();
    }
    res[py::cast("built_in_strings")] = built_in_strings;
    res[py::cast("sj_ver")] = sj_ver;
    res[py::cast("sj_name")] = sj_name;
    res[py::cast("sj_desc")] = sj_desc;

    if (do_print || gretl4py_verbose) {
        std::cout << mod_logo << std::endl;
        std::cout << "gretl4py v" << mod_ver;
        std::cout << " [" << COMPILER_IDENT << "]" << std::endl;
        std::cout << "\t+ pybind11     v" << py11_ver << std::endl;
        std::cout << "\t+ simdjson     v" << sj_ver << std::endl;
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
        err = execute_set(key.c_str(), NULL, NULL, OPT_NONE, GP.prn_ptr());
    } else {
        err = execute_set(key.c_str(), MapPyToCpp(pyval).to_string().c_str(),
                          NULL, OPT_NONE, GP.prn_ptr());
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
void set_default_dset (std::shared_ptr<GretlDataset> GD)
{
    if (DSET) {
        DSET->is_default = false;
    }
    DSET = GD;
}

//////////////////////////////////////////////////
// model management
//////////////////////////////////////////////////

static void check_if_model_exists (const int model_id)
{
    if (estimated_models.find(model_id) == estimated_models.end()) {
        throw std::runtime_error("model (id=" + std::to_string(model_id) + ") does not exist");
    }
}

// wrapper for "destructing" GretlModel models
void GretlModel_destructor_wrapper (const int model_id, const bool unlink)
{
    auto model_it = estimated_models.find(model_id);

    if (model_it != estimated_models.end()) {
        if (unlink) {
            model_it->second->unlink_from_dataset();
        }
        estimated_models.erase(model_it);
    }
}

GretlTypes *GretlModel_get_accessor (const int model_id, const std::string &accessor)
{
    check_if_model_exists(model_id);

    return estimated_models.at(model_id).get()->get_accessor(accessor).release();
}

py::dict GretlModel_test (const int model_id, const std::string &type,
                          const py::kwargs &kwargs)
{
    check_if_model_exists(model_id);

    return estimated_models.at(model_id).get()->test(type, kwargs);
}

py::dict GretlModel_test (const int model_id, const std::string &type,
                          const std::string &vars, const py::kwargs &kwargs)
{
    check_if_model_exists(model_id);

    return estimated_models.at(model_id).get()->test(type, vars, kwargs);
}

py::dict GretlModel_restrict (const int model_id, const py::list &impose,
                              const py::kwargs &kwargs)
{
    check_if_model_exists(model_id);

    return estimated_models.at(model_id).get()->restrict(impose, kwargs);
}

py::dict GretlModel_fcast (const int model_id, const py::kwargs &kwargs)
{
    check_if_model_exists(model_id);

    return estimated_models.at(model_id).get()->forecast(kwargs);
}

//////////////////////////////////////////////////
// model estimation
//////////////////////////////////////////////////
/* preparation for ModelClass binding:
auto model = std::make_unique<GretlModel_NSE_Driver<F, CI>>(DSET);
return std::unique_ptr<GretlModel>(std::move(model));
*/
// wrapper for calling GretlModel_NSE_Driver models
template <auto F, GretlCmdIndex CI>
py::dict GretlModel_NSE_Driver_wrapper (const std::string formula, const py::kwargs &kwargs)
{
    auto model = std::make_unique<GretlModel_NSE_Driver<F, CI>>(DSET, formula, kwargs);

    py::dict RES = model->fit(formula, kwargs);
    estimated_models.try_emplace(model->get_id(), std::move(model));

    return RES;
}
// explicit instantiations for all required (F, CI) pairs
template py::dict GretlModel_NSE_Driver_wrapper<lsq, OLS>(const std::string, const py::kwargs&);
template py::dict GretlModel_NSE_Driver_wrapper<lsq, WLS>(const std::string, const py::kwargs&);
template py::dict GretlModel_NSE_Driver_wrapper<logit_probit, LOGIT>(const std::string, const py::kwargs&);
template py::dict GretlModel_NSE_Driver_wrapper<logit_probit, PROBIT>(const std::string, const py::kwargs&);
template py::dict GretlModel_NSE_Driver_wrapper<count_model, POISSON>(const std::string, const py::kwargs&);
template py::dict GretlModel_NSE_Driver_wrapper<count_model, NEGBIN>(const std::string, const py::kwargs&);
template py::dict GretlModel_NSE_Driver_wrapper<biprobit_model, BIPROBIT>(const std::string, const py::kwargs&);
template py::dict GretlModel_NSE_Driver_wrapper<duration_model, DURATION>(const std::string, const py::kwargs&);
template py::dict GretlModel_NSE_Driver_wrapper<heckit_model, HECKIT>(const std::string, const py::kwargs&);
template py::dict GretlModel_NSE_Driver_wrapper<logistic_driver, LOGISTIC>(const std::string, const py::kwargs&);
template py::dict GretlModel_NSE_Driver_wrapper<ar_model, AR>(const std::string, const py::kwargs&);
template py::dict GretlModel_NSE_Driver_wrapper<ar1_model, AR1>(const std::string, const py::kwargs&);
template py::dict GretlModel_NSE_Driver_wrapper<lad_model, LAD>(const std::string, const py::kwargs&);
template py::dict GretlModel_NSE_Driver_wrapper<garch, GARCH>(const std::string, const py::kwargs&);
template py::dict GretlModel_NSE_Driver_wrapper<mp_ols, MPOLS>(const std::string, const py::kwargs&);
template py::dict GretlModel_NSE_Driver_wrapper<panel_model, PANEL>(const std::string, const py::kwargs&);
template py::dict GretlModel_NSE_Driver_wrapper<ivreg, IVREG>(const std::string, const py::kwargs&);
template py::dict GretlModel_NSE_Driver_wrapper<tobit_model, TOBIT>(const std::string, const py::kwargs&);
