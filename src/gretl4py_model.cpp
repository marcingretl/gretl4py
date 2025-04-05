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

#define use_ldco_sig(c) (c == OLS || c == WLS) //const int *list, DATASET *dset, GretlCmdIndex ci, gretlopt opt
#define use_ldcop_sig(c) (c == LOGIT || c == PROBIT) //int *list, DATASET *dset, int ci, gretlopt opt, PRN *prn
#define use_ldo_sig(c) (c == LOGISTIC || c == LAD \
                        || c == MPOLS || c == IVREG) //const int *list, DATASET *dset, gretlopt opt
#define use_ldop_sig(c) (c == BIPROBIT || c == DURATION \
                         || c == HECKIT || c == AR || c == AR1 \
                         || c == GARCH || c == PANEL) //const int *list, DATASET *dset, gretlopt opt, PRN *prn
#define use_lcdop_sig(c) (c == POISSON || c == NEGBIN) //const int *list, int ci, DATASET *dset, gretlopt opt, PRN *prn

//
// GretlModel class methods
//
GretlModel::GretlModel (GretlDataset *GD)
{
    this->id = next_model_id++;
    estimated_models_add(this->id, this);

    if (GD) {
        this->GD = GD;
    }
}

GretlModel::~GretlModel (void)
{
    if (this->GD) {
        this->GD->linked_models_del(this->id);
    }

    if (this->model != NULL) {
        gretl_model_free(this->model);
    }
}

int GretlModel::get_id (void) const
{
    return id;
}

gretlopt GretlModel::unpack_kwargs (const py::kwargs &kwargs, GretlCmdIndex ci)
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

void GretlModel::save_model (PRN *prn, gretlopt opt)
{
    if (model->errcode) {
        const char *msg = errmsg_get_with_default(model->errcode);
        throw std::runtime_error(msg);
    } else {
        const char *summary;

        printmodel(model, GD->dset, opt, prn);
        summary = gretl_print_get_buffer(prn);
        gretl_model_to_dict(summary);
        if (err) {
            handle_gretl_error(err);
        } else {
            // we link model with dataset used
            GD->linked_models_add(id);
        }
    }
}

void GretlModel::gretl_model_to_dict (const char *summary)
{
    //
    // Here we construct a python dictionary to represent a gretl
    // MODEL, using as intermediate step the gretl_bundle produced by
    // libgretl's bundle_from_model(). We also add the @summary text
    // produced by libgretl's printmodel().
    //

    GB = new GretlBundle(model, GD->dset);
    err = GB->err;

    if (!err) {
        RES = GB->to_dict();
        if (!RES) {
            err = E_DATA;
            throw std::runtime_error("Model bundle to dict conversion failed.");
        } else {
            py::dict _ids;
            _ids[py::cast("_id")] = model->ID;
            _ids[py::cast("_model_id")] = id;
            (*RES)[py::cast("_ids")] = _ids;
            (*RES)[py::cast("summary")] = summary;
        }
    }
    delete GB;
}


//
//  Native Single Equation libgretl MODEL via dedicated Driver
//
template <auto F, GretlCmdIndex CI>
GretlModel_NSED<F, CI>::GretlModel_NSED(GretlDataset *GD)
                                        : GretlModel(GD)  // explicitly call base class constructor!
{
    std::cout << "GretlModel_NSED, id = " << this->get_id();
    std::cout << ", GD  = " << GD << std::endl;
}

template <auto F, GretlCmdIndex CI>
int *GretlModel_NSED<F, CI>::list_from_formula (void)
{
    // convert from a simple R-style formula to a gretl list
    char *tmp = strdup(formula.c_str());
    int *list = NULL;

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

// Method template to support several estimation methods that
// have the same "basic" signature, plus a few special cases
// F: function to call
// CI: GretlCmdIndex
template <auto F, GretlCmdIndex CI>
py::dict *GretlModel_NSED<F, CI>::fit (const std::string &formula,
                                       const py::kwargs &kwargs)
{
    GretlPrint GP = GretlPrint();
    int *list = NULL;

    if (py::len(kwargs) > 0) {
        opt = unpack_kwargs(kwargs, CI);
    }

    if (!GD) {
        throw std::runtime_error("No dataset is available");
    }

    this->formula = formula;
    list = list_from_formula();
    model = gretl_model_new();

    if (list == NULL) {
        throw std::runtime_error("Formula rendering failed");
    } else if (model == NULL || GP.prn == NULL) {
        throw std::runtime_error("Failed to allocate memory");
    }

    if constexpr (use_ldco_sig(CI)) {
        *model = F(list, GD->dset, CI, opt);
    } else if constexpr (use_ldcop_sig(CI)) {
        *model = F(list, GD->dset, CI, opt, GP.prn);
    } else if constexpr (use_ldo_sig(CI)) {
        *model = F(list, GD->dset, opt);
    } else if constexpr (use_ldop_sig(CI)) {
        *model = F(list, GD->dset, opt, GP.prn);
    } else if constexpr (use_lcdop_sig(CI)) {
        *model = F(list, CI, GD->dset, opt, GP.prn);
    }
    save_model(GP.prn, opt);

#if defined(_WIN32)
    gretl_list_free(list);
#else
    std::free(list);
#endif

    return RES;
}
// Explicit instantiations for all required (F, CI) pairs
template class GretlModel_NSED<lsq, OLS>;
template class GretlModel_NSED<lsq, WLS>;
template class GretlModel_NSED<logit_probit, LOGIT>;
template class GretlModel_NSED<logit_probit, PROBIT>;
template class GretlModel_NSED<count_model, POISSON>;
template class GretlModel_NSED<count_model, NEGBIN>;
template class GretlModel_NSED<biprobit_model, BIPROBIT>;
template class GretlModel_NSED<duration_model, DURATION>;
template class GretlModel_NSED<heckit_model, HECKIT>;
template class GretlModel_NSED<logistic_driver, LOGISTIC>;
template class GretlModel_NSED<ar_model, AR>;
template class GretlModel_NSED<ar1_model, AR1>;
template class GretlModel_NSED<lad_model, LAD>;
template class GretlModel_NSED<garch_to_call, GARCH>;
template class GretlModel_NSED<mp_ols, MPOLS>;
template class GretlModel_NSED<panel_model, PANEL>;
template class GretlModel_NSED<ivreg, IVREG>;


//
// Native SARIMA libgretl MODEL
//
GretlModel_arima::GretlModel_arima(GretlDataset *GD,
                                   const std::string y,
                                   const py::tuple &tup)
                                   : GretlModel(GD), y(y), tup(tup)  // explicitly call base class constructor!
{
    std::cout << "GretlModel_arima, id = " << this->get_id();
    std::cout << ", GD  = " << GD << std::endl;
}

void GretlModel_arima::get_arima_order (const py::tuple &tup,
                                        int &p, int &d, int &q)
{
    int n = py::len(tup);

    if (n == 2) {
        p = tup[0].cast<int>();
        q = tup[1].cast<int>();
    } else if (n == 3) {
        p = tup[0].cast<int>();
        d = tup[1].cast<int>();
        q = tup[2].cast<int>();
    } else {
        throw std::runtime_error("order must be of length 2 or 3");
    }
}

py::dict *GretlModel_arima::fit (const std::string &formula,
                                 const py::kwargs &kwargs)
{
    GretlPrint GP = GretlPrint(); // ask Allin if PRN is re-usable
    int *list = NULL;
    int seasonal = 0;
    int y_number;
    int i;

    if (py::len(kwargs) > 0) {
        opt = unpack_kwargs(kwargs, ARMA);
    }

    if (!GD) {
        estimated_models_del(id);   //FIXME: Fatal Python error: PyThreadState_Get:!!!!
        throw std::runtime_error("No dataset is available");
    }

    y_number = current_series_index(GD->dset, y.c_str());
    if (y_number == -1) {
        throw std::runtime_error("There is no variable: " + y);
    }

    p = d = q = 0;
    P = D = Q = 0;
    get_arima_order(tup, p, d, q);

    if (kwargs.contains("seasonal") && !kwargs["seasonal"].is_none()) {
        if (is_pytuple(kwargs["seasonal"])) {
            const py::tuple seas = kwargs["seasonal"].cast<py::tuple>();

            get_arima_order(seas, P, D, Q);
            if (P > 0 || D > 0 || Q > 0) {
                seasonal = 1;
            }
        }
    }

    model = gretl_model_new();
    list = gretl_list_new(seasonal ? 9 : 5);
    if (model == NULL || list == NULL || GP.prn == NULL) {
        throw std::runtime_error("Failed to allocate memory");
    }

    i = 1;
    list[i++] = p;
    list[i++] = d;
    list[i++] = q;
    if (seasonal) {
        list[i++] = LISTSEP;
        list[i++] = P;
        list[i++] = D;
        list[i++] = Q;
    }
    list[i++] = LISTSEP;
    list[i++] = y_number;

    *model = arma(list, NULL, GD->dset, opt, GP.prn);
    save_model(GP.prn, opt);

#if defined(_WIN32)
    gretl_list_free(list);
#else
    std::free(list);
#endif

    return RES;
}
