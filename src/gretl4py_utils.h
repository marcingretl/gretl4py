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

#ifndef GRETL4PY_UTILS_H
#define GRETL4PY_UTILS_H

#include "gretl4py_types.h"

namespace py = pybind11;

//////////////////////////////////////////////////
// dataset registry
//////////////////////////////////////////////////

GretlDatasetPtr get_dataset_by_id (int id);

std::set<int> get_datasets_ids (void);

void register_dataset (const GretlDatasetPtr GD);

#endif // GRETL4PY_UTILS_H
