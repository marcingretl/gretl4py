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
    id = next_model_id++;
    estimated_models_add(id, this);
    std::cout << "GretlModel base: constructor | id = " << id << std::endl;
}

GretlModel::~GretlModel (void)
{
    if (GD) {
        GD->linked_models_del(id);
    }

    if (model != NULL) {
        gretl_model_free(model);
    }
    //FIXME: estimated_models_del(id) -> to GretlModel_destructor_wrapper()!!!
    std::cout << "~GretlModel base: destructor | id = " << id << std::endl;
}

int GretlModel::get_id (void) const
{
    return id;
}

gretlopt GretlModel::unpack_kwargs (const py::kwargs &kwargs, int ci)
{
    int opt, opts = OPT_NONE;
    OptStatus status;

    for (auto &item : kwargs) {
        if (!is_pystr(item.first)) {
            throw std::runtime_error("option key must be a string");
        }
        const std::string &key = item.first.cast<std::string>();

        if (key == "data") {
            // special case: we use already allocated object of GretlDataset class
            GD = item.second.cast<GretlDataset*>(); //FIXME: move it directly to the fit() method!!!
            continue;
        } else if (key == "quiet") {
            throw std::runtime_error("invalid option flag: " + key);
        } else if (key == "seasonal" && ci == ARMA) {
            // arima special: ignore here
            continue;
        } else if (key == "lagselect" && ci == ARMA) {
            throw std::runtime_error("'lagselect' option for (S)ARIMA is temporarily unavailable");
        } else {
            // should be a regular gretl option: check its validity in
            // the context of @ci
            if (std::strchr(key.c_str(), '_')) {
                std::string tmp(key);

                gretl_charsub(tmp.data(), '_', '-');
                opt = valid_long_opt(ci, tmp.c_str(), &status);
            } else {
                opt = valid_long_opt(ci, key.c_str(), &status);
            }
            if (opt == 0) {
                throw std::runtime_error("invalid option flag: " + key);
            }
        }
        if (status == OPT_NO_PARM) {
            // no parameter wanted, but we'll accept a boolean
            if (is_pyint(item.second)) {
                int ival = item.second.cast<int>();

                if (ival == 1) {
                    /* activate the option */
                    opts |= opt;
                } else if (ival != 0) {
                    throw std::runtime_error("invalid option parameter");
                }
            }
        } else {
            // an option parameter is accepted, perhaps required
            if (!is_pystr(item.second) && !is_pyint(item.second) && !is_pyfloat(item.second)) {
                throw std::runtime_error("unsupported option param type");
            } else {
                push_option_param(ci, static_cast<gretlopt>(opt),
                                  gretl_strdup(MapPyToCpp(item.second).to_string().c_str()));
            }
        }
    }

    return (gretlopt) opts;
}

int *GretlModel::list_from_formula (const char *formula)
{
    // convert from a simple R-style formula to a gretl list
    char *tmp = strdup(formula);
    int *list = NULL;
    int err = 0;

    gretl_charsub(tmp, '~', ' ');
    gretl_charsub(tmp, '+', ' ');
    list = generate_list(tmp, GD->dset, 0, &err);
    std::free(tmp);

    if (err) {
        std::cerr << "libgretl error occurred: " << err << std::endl;
        //FIXME: if (gretl4py_verbose) { std::perror(gretl_errmsg_get()); }
    }

    return list;
}

py::dict *GretlModel::save_model (PRN *prn, gretlopt opt)
{
    py::dict *res = nullptr;

    if (model->errcode) {
        const char *msg = errmsg_get_with_default(model->errcode);
        throw std::runtime_error(msg);
    } else {
        const char *summary;
        int err;

        printmodel(model, GD->dset, opt, prn);
        summary = gretl_print_get_buffer(prn);
        res = gretl_model_to_dict(model, GD->dset, summary, &err);
        if (err) {
            handle_gretl_error(err);
        }
    }

    return res;
}


//
//  Native Single Equation libgretl MODEL via dedicated Driver
//
template <auto F, GretlCmdIndex CI>
GretlModel_NSED<F, CI>::GretlModel_NSED(GretlDataset *GD) : GretlModel()  // explicitly call base class constructor!
{
    if (GD) {
        this->GD = GD;
        this->GD->linked_models_add(this->get_id()); // FIXME: move this at the end of estimation!!!
    }
    std::cout << "GretlModel_NSED, id = " << this->get_id();
    std::cout << ", GD  = " << GD << std::endl;
}

template <auto F, GretlCmdIndex CI>
GretlModel_NSED<F, CI>::~GretlModel_NSED (void)
{
    std::cout << "~GretlModel_SEND: destructor" << std::endl;
}

template <auto F, GretlCmdIndex CI>
void GretlModel_NSED<F, CI>::fit (void)
{
    std::cout << "GretlModel_SEND::fit()" << std::endl;
    std::cout << "Running fit() with F = " << F << " and CI = " << CI << std::endl;
    //some_function<F, CI>(formula, kwargs);
}

// Explicit instantiations for all required (F, CI) pairs
template class GretlModel_NSED<lsq, OLS>;
