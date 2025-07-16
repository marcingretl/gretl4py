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
extern "C" {
    #include <gretl/gretl_midas.h>
}
#include <gretl/objstack.h>
#include <iostream>

namespace py = pybind11;

using GretlPackageRegistry = std::map<std::string, GretlPackagePtr>;

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
    std::string summary = "no results"; // model summary to print on python side
    mutable int err = 0;

    template<typename T>
    void save_model (T *model);

    template<typename T>
    void do_restrict (T *model, const py::list &impose, const py::kwargs &kwargs);

    template<typename T, GretlCmdIndex CI>
    void do_forecast (T *model, const py::kwargs &kwargs, int variable_nr = 0);

    void unpack_kwargs (GretlCmdIndex ci);      // model kwargs unpacking

    void unpack_forecast_kwargs (const py::kwargs &kwargs, int &pconf);

    void fr_to_dict (FITRESID *fr, py::dict &fcdict);

    virtual void *model_ptr (void) const = 0;   // non-typed pointer to model
public:
    explicit GretlModel (GretlObjType model_obj_type, std::shared_ptr<GretlDataset> GD,
                         const py::kwargs &kwargs);
    virtual ~GretlModel (void);

    int get_id (void) const;

    std::string repr (void) const;

    void unlink_from_dataset (void) const;

    virtual GretlTypes *get_accessor (const std::string &accessor) const;

    virtual void fit (void) = 0;

    virtual void forecast (const py::kwargs &kwargs) = 0;

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

    void restrict (const py::list &impose, const py::kwargs &kwargs);

    void fit (void) override = 0;

    void forecast (const py::kwargs &kwargs) override;

    void test (const std::string &type, const py::kwargs &kwargs);

    void test (const std::string &type, const std::string &vars, const py::kwargs &kwargs);

    std::string get_formula (void) const override = 0;
};

//
// NSE via dedicated Driver
// extends: GretlModel_NSE
//
template<auto F, GretlCmdIndex CI>
class GretlModel_NSE_Driver : public GretlModel_NSE
{
protected:
    const std::string formula;          // formula defining model
    std::vector<double> optional_par;   // optional general purpose parameter (tau for QUANTREG, (l/r)limits for TOBIT, tests...)
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
template<auto F, GretlCmdIndex CI>
class GretlModel_NSE_Driver_quantreg : public GretlModel_NSE_Driver<F, CI>
{
public:
    GretlModel_NSE_Driver_quantreg (std::shared_ptr<GretlDataset> GD, const std::string &formula,
                                    const double &tau, const py::kwargs &kwargs);
    GretlModel_NSE_Driver_quantreg (std::shared_ptr<GretlDataset> GD, const std::string &formula,
                                    const py::array_t<double> &tau, const py::kwargs &kwargs);
};

//
// NSE_Driver_dpanel via dpd_model
// extends: GretlModel_NSE_Driver
//
template<auto F, GretlCmdIndex CI>
class GretlModel_NSE_Driver_dpanel : public GretlModel_NSE_Driver<F, CI>
{
public:
    GretlModel_NSE_Driver_dpanel (std::shared_ptr<GretlDataset> GD, const std::string &formula,
                                  const int &p, const py::kwargs &kwargs);
    GretlModel_NSE_Driver_dpanel (std::shared_ptr<GretlDataset> GD, const std::string &formula,
                                  const py::array_t<int> &p, const py::kwargs &kwargs);
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

    template<GretlCmdIndex CI>
    void do_var_vecm_fit (const int coint_rank = 0);

    template<GretlCmdIndex CI>
    void do_var_vecm_forecast (const py::kwargs &kwargs);

    int find_variable_nr (const std::string &variable_name);
public:
    GretlModel_VAR (std::shared_ptr<GretlDataset> GD, const std::string &ylist,
                    const int order, const py::kwargs &kwargs);
    virtual ~GretlModel_VAR (void);

    void fit (void) override;

    void forecast (const py::kwargs &kwargs) override;

    void test (const std::string &type, const py::kwargs &kwargs);

    void irf (int targ, int shock, double alpha = 0);

    void irf (const std::string &targ, const std::string &shock, double alpha = 0);

    std::string get_formula (void) const override;
};

//
// VECM libgretl MODEL
// extends: GretlModel_VAR
//
class GretlModel_VAR_VECM : public GretlModel_VAR
{
private:
    const int coint_rank;               // co-integration rank
public:
    GretlModel_VAR_VECM (std::shared_ptr<GretlDataset> GD, const std::string &ylist,
                         const int order, const int rank, const py::kwargs &kwargs);

    void restrict (const py::list &impose, const py::kwargs &kwargs);

    void fit (void) override;

    void forecast (const py::kwargs &kwargs) override;
};


//
// libgretl contributed packages
// extends: GretlModel
//
class GretlModel_package : public GretlModel
{
private:
    const GretlPackagePtr GPKG;
    GretlType res_type;                         // type returned by 'func_name'
    void *res_ptr = NULL;                       // pointer to results from 'func_name'
    std::string func_name;                      // main function to call
    ufunc *func_name_ptr = NULL;                // main function to call: pointer
    fncall *func_name_call = NULL;              // pointer to main function call

    void unpack_kwargs (const std::string &func_name,
                        const py::kwargs &kwargs);// hides base-class method

    void *model_ptr (void) const override;

    void free_results (void *res_to_free,
                       GretlType res_to_free_type);
public:
    GretlModel_package (std::shared_ptr<GretlDataset> GD,
                        const GretlPackagePtr PKG, const py::kwargs &kwargs);
    ~GretlModel_package (void);

    void fit (void) override;

    void forecast (const py::kwargs &kwargs) override;

    std::string get_formula (void) const override;

    void functions (void) const;
};

//////////////////////////////////////////////////
// aliases
//////////////////////////////////////////////////
using GretlModel_NSE_dpanel = GretlModel_NSE_Driver_dpanel<dpd_model, DPANEL>;
using GretlModel_NSE_quantreg = GretlModel_NSE_Driver_quantreg<quantreg_driver, QUANTREG>;

#endif // GRETL4PY_MODEL_H
