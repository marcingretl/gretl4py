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
#include <gretl/gretl_errors.h>
#include <gretl/libset.h>
extern "C" {
    #include <gretl/gretl_string_table.h>
}
#include <iostream>
#include <cstdlib>
#include <unordered_set>

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

// global variables (some used outside this file)
std::shared_ptr<GretlDataset> DSET = nullptr;   // default dataset
std::unordered_set<std::string> UserVars;       // registry of user variables
GretlPrint GP = GretlPrint();                   // printer
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
    // check libgretl version
    if (libgretl_version() < LIBGRETL_MIN_VER) {
        throw std::runtime_error("gretl4py requires libgretl-1.0 version "
                                 + std::to_string(LIBGRETL_MIN_VER) + " or higher");
    }

    // initialize libgretl
    libgretl_init();

    // we set user workdir, gretldir and dotdir
    py::module_ pathlib = py::module_::import("pathlib");
    py::object start = pathlib.attr("Path").attr("cwd")();
    std::string start_dir = py::str(start);

    if (int err = gretl_set_path_by_name("workdir", start_dir.c_str())) {
        handle_gretl_error(err);
    }
    if (const char* env = std::getenv("GRETL_HOME"); env) {
        if (int err = gretl_set_path_by_name("gretldir", env)) {
            handle_gretl_error(err);
        }
#if defined(_WIN32)
        std::string dotdir_path = std::string(env).append("\\tmp");
#else
        std::string dotdir_path = std::string(env).append("/tmp");
#endif // defined
        if (int err = gretl_set_path_by_name("dotdir", dotdir_path.c_str())) {
            handle_gretl_error(err);
        }
    }

    // read and set GRETL_PLUGIN_PATH if available
    if (const char* env = std::getenv("GRETL_PLUGIN_PATH"); env) {
        set_gretl_plugin_path(env);
    }

    // read and set GRETL4PY_VERBOSE if available
    if (const char* env = std::getenv("GRETL4PY_VERBOSE"); env) {
        gretl4py_verbose = std::atoi(env);
    }

    // verbose output
    if (gretl4py_verbose) {
        std::printf("libgretl initialized\n");
    }
}

void module_cleanup (void)
{
    int err = 0;

    for (const auto &name : UserVars) {
        err = user_var_delete_by_name(name.c_str(), NULL);
        if (err) {
            std::cerr << "Failed to delete '" + name + "': ";
            std::cerr << errmsg_get_with_default(err) << std::endl;
        } else {
            std::cout << "Variable '" + name + "' deleted" << std::endl;
        }
    }

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

    if (do_print || gretl4py_verbose) {
        std::cout << mod_logo << std::endl;
        std::cout << "gretl4py v" << mod_ver;
        std::cout << " [" << COMPILER_IDENT << "]" << std::endl;
        std::cout << "\t+ pybind11     v" << py11_ver << std::endl;
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
        py::print("\ngretl_plugin_path = \"" + std::string(gretl_plugin_path()) + "\"");
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

void gretl_user_var_delete_by_name (const std::string &name)
{
    if (UserVars.find(name) != UserVars.end()) {
        int err = user_var_delete_by_name(name.c_str(), GP.prn_ptr());
        if (err) {
            handle_gretl_error(err);
        }
        UserVars.erase(name);
    } else {
        throw std::invalid_argument("no such variable: '" + name + "'");
    }
}

void gretl_print_user_var_by_name (const std::string &name,
                                   std::shared_ptr<GretlDataset> data)
{
    if (UserVars.find(name) == UserVars.end()) {
        throw std::invalid_argument("unknown variable: '" + name + "'");
    }

    int err = 0;
    GretlType uv_type = user_var_get_type_by_name(name.c_str());

    if (uv_type == GRETL_TYPE_BUNDLE) {
        user_var *uv = get_user_var_by_name(name.c_str());

        if (!uv) {
            handle_gretl_error(E_UNKVAR);
        } else {
            gretl_bundle *b = static_cast<gretl_bundle*>(user_var_get_value(uv));

            err = gretl_bundle_print(b, GP.prn_ptr());
        }
    } else if (uv_type == GRETL_TYPE_DOUBLE || uv_type == GRETL_TYPE_INT) {
        print_scalar_by_name(name.c_str(), GP.prn_ptr());
    } else {
        err = print_user_var_by_name(name.c_str(),
                                     data ? data->dset : DSET->dset,
                                     OPT_N, GP.prn_ptr());
    }

    if (err) {
        handle_gretl_error(err);
    }
    GP.print();
}

void uservars_list (void)
{
    std::string var_type_name;

    py::print("Registered variables:");
    for (const auto &name : UserVars) {
        var_type_name = gretl_type_to_string(user_var_get_type_by_name(name.c_str()));

        py::print(name, "(" + var_type_name + ")");
    }
    py::print("\n");
}

// interfaces to gretl's 'genr' command
void genr (const std::string &formula, std::shared_ptr<GretlDataset> data)
{
    // formula-based (mimics gretl's 'genr' command)
    // GretlType genr_get_last_output_type (void);
    gretl_generate<GRETL_TYPE_ANY>(formula, data ? data : DSET);
}

void genr (const std::string &name, const std::string &value,
           std::shared_ptr<GretlDataset> data)
{
    // name- and value- based
    gretl_generate<GRETL_TYPE_ANY>(value, data ? data : DSET, name);
}

void genr (const std::string &name, const py::array &nparray)
{
    // matrix special case: we pass NumPy directly
    gretl_matrix *gretl_mat = NULL;
    NumPyArrayView view;

    NumPyArray_extract(nparray, view);
    gretl_mat = gretl_matrix_alloc(view.rows, view.cols);
    if (gretl_mat == NULL) {
        handle_gretl_error(E_ALLOC);
    }
    NumPyArray_to_gretl_matrix<gretl_matrix>(view, gretl_mat);

    int err = user_var_add(name.c_str(), GRETL_TYPE_MATRIX, gretl_mat);
    if (err) {
        handle_gretl_error(err);
    }

    UserVars.insert(name);
}
