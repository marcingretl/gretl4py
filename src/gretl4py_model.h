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
//#include <gretl/matrix_extra.h>
#include <iostream>

namespace py = pybind11;

// workaround to manually resolve overloads ambiguity
static MODEL (&garch_to_call)(const int*, DATASET*, gretlopt, PRN*) = garch;

//
// Class to handle gretl 'MODEL' type
//
class GretlModel
{
private:
    static inline int next_model_id = 1;
protected:
    int id;                         // model ID
    py::dict *RES = nullptr;        // estimation results as dictionary //FIXME: not pointer, pybind11 Object*!!!
    GretlDataset *GD = nullptr;     // dataset used
    GretlPrint GP = GretlPrint();   // printer (instance wide)
    int *var_list = NULL;           // libgretl list of variables
    gretlopt opt = OPT_NONE;        // model options
    mutable int err = 0;

    gretlopt unpack_kwargs (const py::kwargs &kwargs, GretlCmdIndex ci);
public:
    explicit GretlModel (GretlDataset *GD);
    virtual ~GretlModel (void);

    int get_id (void) const;

    void unlink_from_dataset (void) const;

    virtual py::dict *fit (const std::string &formula, const py::kwargs &kwargs) = 0;

    virtual void forecast (void) = 0;
};

//
// Native Single Equation libgretl MODEL (NSE)
// extends: GretlModel
//
class GretlModel_NSE : public GretlModel
{
protected:
    MODEL *model = NULL;                            // libgretl MODLEL struct

    void save_model (gretlopt opt = OPT_NONE);      //FIXME: do we need opt here? We already have: gretlopt opt = OPT_NONE

    //int out_of_sample_info (int add_ok, int *t2);   //FIXME: change to void!!!
public:
    GretlModel_NSE (GretlDataset *GD);
    virtual ~GretlModel_NSE (void);

    py::dict *fit (const std::string &formula, const py::kwargs &kwargs) override = 0;

    void forecast (void) override;
};

//
// NSE via dedicated Driver
// extends: GretlModel_NSE
//
template <auto F, GretlCmdIndex CI>
class GretlModel_NSE_Driver : public GretlModel_NSE
{
private:
    void list_from_formula (const std::string &formula);
public:
    GretlModel_NSE_Driver (GretlDataset *GD);

    py::dict *fit (const std::string &formula, const py::kwargs &kwargs) override;
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
