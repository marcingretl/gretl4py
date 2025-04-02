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
public:
    GretlModel (void);
    virtual ~GretlModel (void);

    virtual void fit (void) = 0;

    int get_id (void) const;
};

//
// Native Single Equation libgretl MODEL via dedicated Driver
//
class GretlModel_NSED : public GretlModel
{
public:
    GretlModel_NSED (GretlDataset *GD);
    ~GretlModel_NSED (void) override;

    void fit (void) override;
};


// signatures implemented in gretl_calls.cpp
void estimated_models_add (int id, GretlModel *GM);
void estimated_models_del (int id);

#endif // GRETL4PY_MODEL_H
