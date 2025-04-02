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

#include <pybind11/pybind11.h>
#include "gretl4py_data.h"
#include "gretl4py_model.h"
#include <gretl/matrix_extra.h>
#include <gretl/gretl_model.h>
#include <string>

//
// GretlModel class methods
//
GretlModel::GretlModel (void)
{
    id = GretlModel::next_model_id++;
    estimated_models_add(id, this);
    std::cout << "GretlModel base: constructor | id = " << id << std::endl;
}

GretlModel::~GretlModel (void)
{
    std::cout << "~GretlModel base: destructor | id = " << id << std::endl;
}

int GretlModel::get_id (void) const
{
    return id;
}

//
//  Native Single Equation libgretl MODEL via dedicated Driver
//
GretlModel_NSED::GretlModel_NSED (GretlDataset *GD)
{
    //FIXME: we need look for data in kwargs!!!
    if (GD) {
        this->GD = GD;
        this->GD->linked_models_add(id);
    }
    std::cout << "GretlModel_SEND, id = " << id;
    std::cout << ", GD  = " << GD << std::endl;
}

GretlModel_NSED::~GretlModel_NSED (void)
{
    std::cout << "~GretlModel_SEND: destructor" << std::endl;
}

void GretlModel_NSED::fit (void)
{
    std::cout << "GretlModel_SEND::fit()" << std::endl;
}
