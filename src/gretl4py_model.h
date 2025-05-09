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

//
// Class to handle gretl 'MODEL' type
//
class GretlModel
{
private:
    static inline int next_model_id = 1;
protected:
    const int id;                       // model ID
    const GretlObjType model_obj_type;  // libgretl's object type (in model context)
    const py::kwargs kwargs;            // kwargs passed to the model
    py::dict RES;                       // estimation results as dictionary
    std::shared_ptr<GretlDataset> GD;   // dataset used
    GretlPrint GP = GretlPrint();       // printer (instance wide)
    gretl_list_ptr var_list{nullptr};   // libgretl list of variables
    gretlopt opt = OPT_NONE;            // instance wide options (including tests and forecasts)
    mutable int err = 0;

    template<typename T>
    void save_model (T *model);

    void unpack_kwargs (GretlCmdIndex ci);      // model kwargs unpacking

    void fr_to_dict (FITRESID *fr, py::dict &fcdict);

    virtual void *model_ptr (void) const = 0;   // non-typed pointer to model
public:
    py::list tests;                     // list with results of preformed tests

    explicit GretlModel (GretlObjType model_obj_type, std::shared_ptr<GretlDataset> GD,
                         const py::kwargs &kwargs);
    virtual ~GretlModel (void);

    int get_id (void) const;

    void unlink_from_dataset (void) const;

    virtual std::unique_ptr<GretlTypes> get_accessor (const std::string &accessor) const;

    virtual void fit (void) = 0;

    virtual py::dict forecast (const py::kwargs &kwargs) = 0;

    virtual void restrict (const py::list &impose, const py::kwargs &kwargs) = 0;

    virtual void test (const std::string &type, const py::kwargs &kwargs) = 0;

    virtual void test (const std::string &type, const std::string &vars, const py::kwargs &kwargs) = 0;

    virtual std::string get_formula (void) const = 0;
};

//
// Native Single Equation libgretl MODEL (NSE)
// extends: GretlModel
//
class GretlModel_NSE : public GretlModel
{
protected:
    MODEL *model = NULL;    // libgretl MODEL struct

    void *model_ptr (void) const override;
public:
    GretlModel_NSE (std::shared_ptr<GretlDataset> GD, const py::kwargs &kwargs);
    virtual ~GretlModel_NSE (void);

    void fit (void) override = 0;

    py::dict forecast (const py::kwargs &kwargs) override;

    void restrict (const py::list &impose, const py::kwargs &kwargs) override;

    void test (const std::string &type, const py::kwargs &kwargs) override;

    void test (const std::string &type, const std::string &vars, const py::kwargs &kwargs) override;

    std::string get_formula (void) const override = 0;
};

//
// NSE via dedicated Driver
// extends: GretlModel_NSE
//
template <auto F, GretlCmdIndex CI>
class GretlModel_NSE_Driver : public GretlModel_NSE
{
protected:
    const std::string formula;                  // formula defining model
    std::vector<double> optional_par;           // optional general purpose parameter (tau for QUANTREG, (l/r)limits for TOBIT, tests...)
    gretl_matrix *optional_par_as_mat = NULL;
public:
    GretlModel_NSE_Driver (std::shared_ptr<GretlDataset> GD,
                           const std::string &formula, const py::kwargs &kwargs);

    void fit (void) override;

    std::string get_formula (void) const override;
};

//
// NSE_Driver_quantreg via quantreg_driver
// extends: GretlModel_NSE_Driver
//
template <auto F, GretlCmdIndex CI>
class GretlModel_NSE_Driver_quantreg : public GretlModel_NSE_Driver<F, CI>
{
public:
    GretlModel_NSE_Driver_quantreg (std::shared_ptr<GretlDataset> GD, const std::string &formula,
                                    const double &tau, const py::kwargs &kwargs);
    GretlModel_NSE_Driver_quantreg (std::shared_ptr<GretlDataset> GD, const std::string &formula,
                                    const py::array_t<double> &tau, const py::kwargs &kwargs);

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
    GretlModel_NSE_arima (std::shared_ptr<GretlDataset> GD, const std::string &y,
                          const py::tuple &tup, const py::kwargs &kwargs);
    ~GretlModel_NSE_arima (void);

    void fit (void) override;

    std::string get_formula (void) const override;
};


//
// VAR libgretl MODEL
// extends: GretlModel
//
class GretlModel_VAR : public GretlModel
{
protected:
    std::string vars;           // list of endogenous (+ exogenous) variables
    const int order;            // VAR order
    GRETL_VAR *model = NULL;    // libgretl GRETL_VAR struct

    void *model_ptr (void) const override;
public:
    GretlModel_VAR (std::shared_ptr<GretlDataset> GD, const std::string &ylist,
                    const int order, const py::kwargs &kwargs);
    virtual ~GretlModel_VAR (void);

    void fit (void) override;

    py::dict forecast (const py::kwargs &kwargs) override;

    void restrict (const py::list &impose, const py::kwargs &kwargs) override;

    void test (const std::string &type, const py::kwargs &kwargs) override;

    void test (const std::string &type, const std::string &vars, const py::kwargs &kwargs) override;

    void irf (int targ, int shock, double alpha = 0);

    void irf (const std::string &targ, const std::string &shock, double alpha = 0);

    std::string get_formula (void) const override;
};


//////////////////////////////////////////////////
// aliases
//////////////////////////////////////////////////
using GretlModel_NSE_quantreg = GretlModel_NSE_Driver_quantreg<quantreg_driver, QUANTREG>;

#endif // GRETL4PY_MODEL_H
