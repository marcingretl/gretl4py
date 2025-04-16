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

#ifndef GRETL4PY_MODEL_H
#define GRETL4PY_MODEL_H

#include <pybind11/pybind11.h>
#include "gretl4py_data.h"
#include <gretl/libgretl.h>
#include <gretl/gretl_drivers.h>
#include <gretl/objstack.h>
#include <iostream>

namespace py = pybind11;

// workaround to manually resolve overloads ambiguity
static MODEL (&garch_to_call)(const int*, DATASET*, gretlopt, PRN*) = garch;
static MODEL (&quantreg_to_call)(const gretl_matrix*, const int*, DATASET*, gretlopt, PRN*) = quantreg;

// custom libgretl's list deleter
static inline void gretl_list_deleter (int *ptr) {
#if defined(_WIN32)
    gretl_list_free(ptr);
#else
    std::free(ptr);
#endif
}

//
// Class to handle gretl 'MODEL' type
//
class GretlModel
{
private:
    static inline int next_model_id = 1;
protected:
    int id;                                     // model ID
    py::dict *RES = nullptr;                    // estimation results as dictionary     //FIXME: not pointer, pybind11 Object*!!!
    py::dict *RES_fcast = nullptr;              // forecasting results as dictionary    //FIXME: not pointer, pybind11 Object*!!!
    mutable GretlTypes *RES_other = nullptr;    // other results (accessor, tests, ets)
    GretlDataset *GD = nullptr;                 // dataset used
    GretlPrint GP = GretlPrint();               // printer (instance wide)
    gretl_list_ptr var_list{nullptr, gretl_list_deleter}; // libgretl list of variables
    gretlopt opt = OPT_NONE;                    // model options (including tests)
    std::vector<double> optional_par;           // optional general purose parameter (tau for QUANTREG, (l/r)limits for TOBIT, tests...)
    bool model_free = true;                     // do we free model in deconstructor
    mutable int err = 0;

    void unpack_kwargs (const py::kwargs &kwargs, GretlCmdIndex ci);

    gretlopt unpack_kwargs_forecast (const py::kwargs &kwargs, GretlCmdIndex ci);
public:
    explicit GretlModel (GretlDataset *GD);
    virtual ~GretlModel (void);

    int get_id (void) const;

    void unlink_from_dataset (void) const;

    virtual GretlTypes *get_accessor (const std::string &accessor) const = 0;

    virtual py::dict *fit (const std::string &formula, const py::kwargs &kwargs) = 0;

    virtual py::dict *forecast (const py::kwargs &kwargs) = 0;

    virtual py::dict test (const std::string &type, const py::kwargs &kwargs) = 0;

    virtual py::dict test (const std::string &type, const std::string &vars, const py::kwargs &kwargs) = 0;
};

//
// Native Single Equation libgretl MODEL (NSE)
// extends: GretlModel
//
class GretlModel_NSE : public GretlModel
{
protected:
    MODEL *model = NULL;                            // libgretl MODLEL struct

    gretl_list_ptr list_from_formula (const std::string &formula);

    void save_model (gretlopt opt = OPT_NONE);      //FIXME: do we need opt here? We already have: gretlopt opt = OPT_NONE
public:
    GretlModel_NSE (GretlDataset *GD);
    virtual ~GretlModel_NSE (void);

    GretlTypes *get_accessor (const std::string &accessor) const override;

    py::dict *fit (const std::string &formula, const py::kwargs &kwargs) override = 0;

    py::dict *forecast (const py::kwargs &kwargs) override;

    py::dict test (const std::string &type, const py::kwargs &kwargs) override;

    py::dict test (const std::string &type, const std::string &vars, const py::kwargs &kwargs) override;
};

//
// NSE via dedicated Driver
// extends: GretlModel_NSE
//
template <auto F, GretlCmdIndex CI>
class GretlModel_NSE_Driver : public GretlModel_NSE
{
protected:
    gretl_matrix *optional_par_as_mat = NULL;
public:
    GretlModel_NSE_Driver (GretlDataset *GD);

    py::dict *fit (const std::string &formula, const py::kwargs &kwargs) override;
};

//
// NSE_Driver_quantreg via quantreg_driver
// extends: GretlModel_NSE_Driver
//
template <auto F, GretlCmdIndex CI>
class GretlModel_NSE_Driver_quantreg : public GretlModel_NSE_Driver<F, CI>
{
public:
    GretlModel_NSE_Driver_quantreg (GretlDataset *GD, const double &tau);
    GretlModel_NSE_Driver_quantreg (GretlDataset *GD, const py::list &tau);

    virtual ~GretlModel_NSE_Driver_quantreg (void);
};

//
// NSE via arima plugin
// extends: GretlModel_NSE
//
class GretlModel_NSE_arima : public GretlModel_NSE
{
private:
    const std::string y;
    const py::tuple tup;
    int p, d, q;
    int P, D, Q;

    void get_arima_order (const py::tuple &tup, int &p, int &d, int &q);
public:
    GretlModel_NSE_arima (GretlDataset *GD, const std::string &y, const py::tuple &tup);

    py::dict *fit (const std::string &formula, const py::kwargs &kwargs) override;
};

#endif // GRETL4PY_MODEL_H
