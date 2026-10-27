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

#ifndef GRETL_CALLS_H
#define GRETL_CALLS_H

#include "gretl4py_types.h"

namespace py = pybind11;

//////////////////////////////////////////////////
// classes
//////////////////////////////////////////////////
//
// Class to handle printing
//
class GretlPrint
{
private:
    PRN *prn_ = nullptr;        // libgretl PRN struct
    PrnType prn_type_;          // libgretl printer type
    int err_ = 0;
public:
    GretlPrint (PrnType prn_type = GRETL_PRINT_BUFFER); // by default we use buffered printer
    GretlPrint (const GretlPrint&) = delete;            // we do not allow for copy-constructor
    ~GretlPrint (void);

    GretlPrint &operator= (const GretlPrint&) = delete;

    PRN *prn_ptr (void);

    void print (void);

    const char *get_buffer (void);
};

//////////////////////////////////////////////////
// functions
//////////////////////////////////////////////////
void do_set (const std::string &key, const std::string &value, const py::kwargs &kwargs);

template <typename T>
py::object do_gretl_summary (const T &varlist, const py::kwargs &kwargs);

template <typename T>
void do_gretl_print (const std::string &varlist, const py::kwargs &kwargs);

double do_gretl_get_pvalue (const std::string &dist, double value, const py::array_t<double> &params = py::array());

void do_gretl_varlist (const py::kwargs &kwargs);

py::object do_gretl_include (const std::string &filename);

void do_gretl_pkg (const std::string &action, const std::string &pkgname, const py::kwargs &kwargs);

template <bool RUN>
void gretl_hansl_execute (const std::string &script);

template <GretlFunIndex FI>
double gretl_get_pvalue_critval_pdf_cdf (const std::string &dist, double value, const py::object &param = py::none());

template <GretlFunIndex FI, typename T>
GretlMatrixPtr matrix_functions_wrapper (const std::string &dist, const T &src, const py::object &param = py::none());

template <GretlFunIndex FI>
GretlMatrixPtr matrix_cov_corr (const py::array &X, int dfcorr = 1);

template <GretlFunIndex FI>
GretlMatrixPtr matrix_cov_corr (const GretlMatrixPtr &X, int dfcorr = 1);

void genr (const std::string &formula, GretlDatasetPtr data);

template <typename T>
void genr (const std::string &name, const T &value);

[[noreturn]] void handle_gretl_error (int err, const std::string &ext_msg = "", const std::string &ext_msg_desc = "");

void gretl_valid_long_opt (gretlopt &opt, GretlCmdIndex ci, const std::string &key, const py::handle &val, const char *ext_val = nullptr);

template <GretlCmdIndex CI, typename T>
gretlopt unpack_kwargs_command (T &dataset, const py::kwargs &kwargs, GretlPlot *plotter = nullptr);

bool unpack_kwargs_test (gretlopt &opt, const std::string &type, const py::kwargs &kwargs, int &order, GretlPlot *plotter = nullptr);

GretlTypesPtr gretl_generate_gretl_object (const char *s, int &err, GretlObjType obj_type = GRETL_OBJ_INFO, DATASET *dset = nullptr, void *model = nullptr);

template <GretlType GR_TYPE>
void gretl_generate (const std::string &formula, GretlDatasetPtr dataset = nullptr, const std::string &name = {});

void do_gretl_delete_variables (const std::string &varname, const py::kwargs &kwargs, bool print = true);

py::object get_uservar (const std::string &varname);

template <GretlObjType OBJ_TYPE>
py::object do_get_accessor (const std::string &name, GretlDatasetPtr dataset = nullptr, void *model = nullptr);

GretlBundlePtr kmeans (const GretlMatrix &X, int centers, const GretlBundlePtr opts = nullptr);

GretlBundlePtr kmeans (const GretlMatrix &X, const GretlMatrix &centers, const GretlBundlePtr opts = nullptr);

GretlMatrixPtr do_get_tdisagg_matrix (py::object Y, int s, const py::kwargs &kwargs, GretlDatasetPtr dataset = nullptr);

GretlMatrixPtr do_kpsscrit (int T, bool trend = false);

template <GretlCmdIndex CI, typename T>
py::object do_execute_plot_call (const T &varlist, const GretlDatasetPtr dataset, const std::string &literal, const py::kwargs &kwargs);

#endif // GRETL_CALLS_H
