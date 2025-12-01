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

extern GretlDatasetPtr DSET;

//
// Class to handle printing
//
class GretlPrint
{
private:
    PRN *prn_ = NULL;           // libgretl PRN struct
    PrnType prn_type_;          // libgretl printer type
public:
    int err = 0;

    GretlPrint (PrnType prn_type = GRETL_PRINT_BUFFER);  // by default we use buffered printer
    ~GretlPrint (void);

    PRN *prn_ptr (void);

    void print (void);

    const char *get_buffer (void);
};

void do_set (const std::string &key,
             const std::optional<py::object> &pyval = std::nullopt);

void do_summary (const std::string &varlist, const py::kwargs &kwargs);

double do_gretl_get_pvalue (const std::string &dist, double value,
                            const py::array_t<double> &params = py::array());

py::object do_include (const std::string &name);

GretlPackagePtr load_package (const std::string &pkgname,
                              const std::string &suffix = "");

                              template<int MODE>
double gretl_get_pvalue_critval_pdf_cdf (const std::string &dist, double value,
                                         const py::object &param = py::none());

// these two functions are temporal and will be moved to class
void gretl_user_var_delete_by_name (const std::string &name);

void gretl_print_user_var_by_name (const std::string &name,
                                   GretlDatasetPtr data = nullptr);

void uservars_list (void);

void genr (const std::string &formula, GretlDatasetPtr data);

template <typename T>
void genr (const std::string &name, const T &value);

void handle_gretl_error (int err);

void gretl_valid_long_opt (gretlopt &opt, GretlCmdIndex ci,
                           const std::string &key, const py::handle &val,
                           const char *ext_val = nullptr);

void unpack_kwargs_test (gretlopt &opt, const std::string &type,
                         const py::kwargs &kwargs, int &order);

gretl_list_ptr list_from_formula (const std::string &formula,
                                  DATASET *dset, int &err);

GretlTypesPtr gretl_generate_gretl_object (const char *s, int &err,
                                           GretlObjType obj_type = GRETL_OBJ_INFO,
                                           DATASET *dset = NULL, void *model = NULL);

template <GretlType GR_TYPE>
void gretl_generate (const std::string &formula, GretlDatasetPtr GD = DSET,
                     const std::string &name = {});

#endif // GRETL_CALLS_H
