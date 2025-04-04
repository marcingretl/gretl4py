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
    py::dict *RES = nullptr;        // estimation results as dictionary (returned by ::fit(): FIXME: not pointer, pybind11 Object*!!!)
    GretlDataset *GD = nullptr;
    GretlBundle *GB = nullptr;
    MODEL *model = NULL;
    mutable int err = 0;
    int id;                         // model ID

    gretlopt unpack_kwargs (const py::kwargs &kwargs, GretlCmdIndex ci);
    int *list_from_formula (const char *formula);
    void save_model (PRN *prn, gretlopt opt = OPT_NONE);   //FIXME: PRN *prn as class field!!!
    void gretl_model_to_dict (const char *summary);
public:
    explicit GretlModel (GretlDataset *GD);
    virtual ~GretlModel (void);

    virtual py::dict *fit (const std::string &formula, const py::kwargs &kwargs) = 0;

    virtual void forecast (void) { std::cout << "this is model: " << id << ", in future I'll get you forecasts\n"; };

    int get_id (void) const;
};

//
// Native Single Equation libgretl MODEL via dedicated Driver
//
template <auto F, GretlCmdIndex CI>
class GretlModel_NSED : public GretlModel
{
public:
    GretlModel_NSED (GretlDataset *GD);

    py::dict *fit (const std::string &formula, const py::kwargs &kwargs) override;
};

//
// Native SARIMA libgretl MODEL
//
class GretlModel_arima : public GretlModel
{
private:
    const std::string y;
    const py::tuple tup;
    int p, d, q;
    int P, D, Q;

    void get_arima_order (const py::tuple &tup, int &p, int &d, int &q);
public:
    GretlModel_arima (GretlDataset *GD, const std::string y, const py::tuple &tup);

    py::dict *fit (const std::string &formula, const py::kwargs &kwargs) override;
};

// functions implemented in gretl_calls.cpp
void estimated_models_add (const int id, GretlModel *GM);
void estimated_models_del (const int id);

#endif // GRETL4PY_MODEL_H
