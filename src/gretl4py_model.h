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
//#include <gretl/matrix_extra.h>
#include <iostream>

namespace py = pybind11;

//
// Class to handle gretl 'MODEL' type
// NOTE: instances MUST NOT be allocated at stack to avoid double free!!!
//
class GretlModel
{
private:
    static inline int next_model_id = 1;
protected:
    GretlDataset *GD = nullptr;
    GretlBundle *GB = nullptr;
    MODEL *model = NULL;
    mutable int err = 0;
    int id;

    gretlopt unpack_kwargs (const py::kwargs &kwargs, int ci);  //FIXME ci as GretlCommand + gretlopt as instance field!!!
    int *list_from_formula (const char *formula);
    py::dict *save_model (PRN *prn, gretlopt opt = OPT_NONE);   //FIXME: PRN *prn as class field!!!
public:
    explicit GretlModel (void);
    virtual ~GretlModel (void);

    virtual py::dict *fit (const std::string &formula, const py::kwargs &kwargs) = 0;

    //virtual void forecast (void) = 0;

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
// signatures implemented in gretl_calls.cpp
void estimated_models_add (int id, GretlModel *GM);
void estimated_models_del (int id);


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

#endif // GRETL4PY_MODEL_H
