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
#include <pybind11/numpy.h>
#include "gretl4py_utils.h"
#include "gretl4py_types.h"
#include "gretl4py_data.h"

static std::map<int, std::weak_ptr<GretlDataset>> GDs;

namespace py = pybind11;

//////////////////////////////////////////////////
// dataset registry
//////////////////////////////////////////////////

std::set<int> get_datasets_ids (void)
{
    //
    // here we return a set of alive dataset's ids
    // (i.e. we erase already deleted)
    //
    std::set<int> res;

    for (auto it = GDs.begin(); it != GDs.end();) {
        // at first we always erase already deleted datasets
        if (it->second.expired()) {
            it = GDs.erase(it);
        } else {
            res.insert(it->first);
            ++it;
        }
    }

    return res;
}

GretlDatasetPtr get_dataset_by_id (int id)
{
    //
    // we return pointer to already registered dataset
    //
    auto dset_reg = get_datasets_ids();

    if (!dset_reg.count(id)) throw std::invalid_argument("there is no dataset with id = " + std::to_string(id));
    auto it = GDs.find(id);

    return it->second.lock();
}

void register_dataset (const GretlDatasetPtr GD)
{
    //
    // here we register newly created GretlDataset instance
    //
    if (!GD) throw std::runtime_error("Attempt to register null dataset");

    auto [it, ok] = GDs.insert({GD->get_id(), GD});

    if (!ok) throw std::logic_error("Duplicate dataset id: " + std::to_string(GD->get_id()));
}
