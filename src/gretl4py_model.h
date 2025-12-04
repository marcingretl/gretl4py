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
#include "gretl_calls.h"
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
    static inline int next_model_id_ = 1;
protected:
    const int id_;                      // model ID
    const GretlObjType model_obj_type_; // libgretl's object type (in model context)
    const GretlCmdIndex ci_;            // command index used
    bool estimated_ = false;            // if model already estimated
    gretlopt opt = OPT_NONE;            // instance-wide options (including tests and forecasts)
    mutable int err = 0;                // holds libgretl errors (instance-wide)
    GretlDatasetPtr GD_;                // dataset used
    gretl_list_ptr var_list_{nullptr};  // libgretl list of variables
    GretlBundlePtr GB_model_ = nullptr; // bundle with results
    GretlPrint GP_ = GretlPrint();      // printer (instance wide)
    std::string summary_;               // model summary to print on python side

    virtual void *model_ptr_ (void) const = 0;   // non-typed pointer to model

    template <typename T>
    void save_model_ (T *model);

    template <typename T>
    void do_restrict_ (T *model, const py::list &impose, const py::kwargs &kwargs);

    template <typename T, GretlCmdIndex CI>
    void do_forecast_ (T *model, const py::kwargs &kwargs, int variable_nr = 0);

    void unpack_forecast_kwargs_ (const py::kwargs &kwargs, int &pconf);

    void fr_to_dict_ (FITRESID *fr, py::dict &fcdict);

    void check_if_estimated_ (void) const;
public:
    explicit GretlModel (GretlObjType model_obj_type, GretlCmdIndex ci,
                         const py::kwargs &kwargs);
    virtual ~GretlModel (void);

    virtual void fit (void) = 0;

    virtual void forecast (const py::kwargs &kwargs) = 0;

    virtual std::string get_formula (void) const = 0;

    virtual py::object get_accessor (const std::string &accessor) const;

    int get_id (void) const;

    std::string repr_impl (void) const;         // for __repr__ binding

    GretlBundlePtr get_bundle (void) const;     // return pointer to GB_model

    void unlink_from_dataset (void) const;
};

//
// Native Single Equation libgretl MODEL (NSE)
// extends: GretlModel
//
class GretlModel_NSE : public GretlModel
{
protected:
    MODEL *model_ = NULL;    // libgretl MODEL struct

    void *model_ptr_ (void) const override;

    template <GretlCmdIndex CI>
    void regexp_extract_ (const std::string &formula,
                          std::string &formula_left,
                          std::string &formula_right);    // for extracting formula from regular expression
public:
    GretlModel_NSE (GretlCmdIndex ci, const py::kwargs &kwargs);
    virtual ~GretlModel_NSE (void);

    void fit (void) override = 0;

    std::string get_formula (void) const override = 0;

    void forecast (const py::kwargs &kwargs) override;

    void restrict (const py::list &impose, const py::kwargs &kwargs);

    template <ModelDataIndex IDX>
    char *gretl_get_fit_or_resid (const std::string &name = "") const;

    void test (const std::string &type, const py::kwargs &kwargs);

    void test (const std::string &type, const std::string &vars, const py::kwargs &kwargs);

    double adj_rsq (void) const;
};

//
// NSE via arima plugin
// extends: GretlModel_NSE
//
class GretlModel_NSE_arima : public GretlModel_NSE
{
private:
    const std::string y_;   // name of y process
    const py::tuple tup_;   // basic (P,D,Q) definition
    int y_number_;          // number of y process in the context of GD
    int seasonal_ = 0;      // number of seasonal elements
    int exogenous_ = 0;     // number of exogenous variables
    gretl_list_ptr xlist_;  // list of exogenous variables
    int p_, d_, q_;
    int P_, D_, Q_;
public:
    GretlModel_NSE_arima (const std::string &y, const py::tuple &tup,
                          const py::kwargs &kwargs);
    ~GretlModel_NSE_arima (void);

    void fit (void) override;

    std::string get_formula (void) const override;
};

//
// NSE via dedicated Driver
// extends: GretlModel_NSE
//
template <auto F, GretlCmdIndex CI>
class GretlModel_NSE_Driver : public GretlModel_NSE
{
protected:
    const std::string formula_;         // formula defining model
public:
    GretlModel_NSE_Driver (const std::string &formula,
                           const py::kwargs &kwargs);

    void fit (void) override;

    std::string get_formula (void) const override;
};

//
// NSE_Driver_quantreg via quantreg_driver
// extends: GretlModel_NSE_Driver
//
class GretlModel_NSE_Driver_quantreg : public GretlModel_NSE_Driver<quantreg_driver, QUANTREG>
{
private:
    std::string tau_;
public:
    GretlModel_NSE_Driver_quantreg (const std::string &formula,
                                    const double &tau,
                                    const py::kwargs &kwargs);
    GretlModel_NSE_Driver_quantreg (const std::string &formula,
                                    const py::array_t<double> &tau,
                                    const py::kwargs &kwargs);

    void fit (void) override;
};

//
// NSE_Driver_dpanel via dpd_model
// extends: GretlModel_NSE_Driver
//
class GretlModel_NSE_Driver_dpanel : public GretlModel_NSE_Driver<dpd_model, DPANEL>
{
private:
    std::vector<double> lags_;
public:
    GretlModel_NSE_Driver_dpanel (const std::string &formula,
                                  const int &p,
                                  const py::kwargs &kwargs);
    GretlModel_NSE_Driver_dpanel (const std::string &formula,
                                  const py::array_t<int> &p,
                                  const py::kwargs &kwargs);

    void fit (void) override;
};

//
// NSE_Driver_tobit via tobit_model
// extends: GretlModel_NSE_Driver
//
class GretlModel_NSE_Driver_tobit : public GretlModel_NSE_Driver<tobit_model, TOBIT>
{
private:
    double llimit_ = 0;
    double rlimit_ = NADBL;
public:
    GretlModel_NSE_Driver_tobit (const std::string &formula,
                                 const py::kwargs &kwargs);

    void fit (void) override;
};

//
// NLS,MLE,GMM via plugin
// extends: GretlModel_NSE
//
template <GretlCmdIndex CI>
class GretlModel_NSE_nl : public GretlModel_NSE
{
private:
    const std::string command_;         // original gretl command (nls, mle, gmm)
    const std::string formula_;         // formula defining model
    std::string weights_;
    std::string params_;
    std::string param_names_;
    std::vector<std::string> deriv_;
    std::vector<std::string> orthog_;
    std::vector<std::string> statements_;

    template <typename T>
    void do_nl_parse_line_ (const std::string &key, T *keyword = nullptr);
public:
    GretlModel_NSE_nl (const std::string formula, const py::kwargs &kwargs);
    ~GretlModel_NSE_nl (void);

    void fit (void) override;

    std::string get_formula (void) const override;

    template <bool ForString>
    void add_line_to_parse (const std::string &keyword,
                            const std::string &val);

    void add_init_value (const std::string &varname,
                         double value);
};

//
// VAR libgretl MODEL
// extends: GretlModel
//
class GretlModel_VAR : public GretlModel
{
protected:
    std::string vars_;          // list of endogenous (+ exogenous) variables
    const int order_;           // VAR order
    GRETL_VAR *model_ = NULL;   // libgretl GRETL_VAR struct

    void *model_ptr_ (void) const override;

    template <GretlCmdIndex CI>
    void do_var_vecm_fit_ (const int coint_rank = 0);

    template <GretlCmdIndex CI>
    void do_var_vecm_forecast_ (const py::kwargs &kwargs);

    int find_variable_nr_ (const std::string &variable_name);
public:
    GretlModel_VAR (const std::string &ylist,
                    const int order, GretlCmdIndex ci,
                    const py::kwargs &kwargs);
    virtual ~GretlModel_VAR (void);

    void fit (void) override;

    void forecast (const py::kwargs &kwargs) override;

    std::string get_formula (void) const override;

    char *save_uhat (int eqn, const std::string &name = "") const;

    void test (const std::string &type, const py::kwargs &kwargs);

    void irf (int targ, int shock, double alpha = 0);

    void irf (const std::string &targ, const std::string &shock, double alpha = 0);
};

//
// VECM libgretl MODEL
// extends: GretlModel_VAR
//
class GretlModel_VAR_VECM : public GretlModel_VAR
{
private:
    const int coint_rank_;          // co-integration rank
public:
    GretlModel_VAR_VECM (const std::string &ylist, const int order,
                         const int rank, const py::kwargs &kwargs);

    void fit (void) override;

    void forecast (const py::kwargs &kwargs) override;

    void restrict (const py::list &impose, const py::kwargs &kwargs);

    char *save_ec_term (const int nr, const std::string &name = "") const;
};

//
// Native Multiple Equation libgretl SYSTEM (NME)
// extends: GretlModel
//
class GretlModel_NME : public GretlModel
{
private:
    std::string method_;                    // estimation method
    std::vector<std::string> equations_;    // vector holding equations
    std::vector<std::string> instruments_;  // vector holding instruments
    std::vector<std::string> identities_;   // vector holding identities
    std::string endogs_;                    // vector holding endogs
    equation_system *model_ = NULL;         // libgretl equation_system struct

    void *model_ptr_ (void) const override;
public:
    GretlModel_NME (const std::string &method, const py::kwargs &kwargs);
    virtual ~GretlModel_NME (void);

    void fit (void) override;

    void forecast (const py::kwargs &kwargs) override;

    std::string get_formula (void) const override;

    void restrict (const py::list &impose, const py::kwargs &kwargs);

    //void test (const std::string &type, const py::kwargs &kwargs);

    //char *save_uhat (int eqn, const std::string &name = "") const;
};

//
// libgretl contributed packages
// extends: GretlModel
//
class GretlModel_package : public GretlModel
{
private:
    const py::kwargs kwargs_;
    GretlPackagePtr GPKG_;          // pointer to package to use
    std::string main_func_name_;    // main function to call
    std::string print_func_name_;   // function with role: UFUN_BUNDLE_PRINT
    std::string fcast_func_name_;   // function with role: UFUN_BUNDLE_FCAST
    GretlPackageRes res_;           // function results

    void *model_ptr_ (void) const override;
public:
    GretlModel_package (const std::string &name, const py::kwargs &kwargs);

    void fit (void) override;

    void forecast (const py::kwargs &kwargs) override;

    std::string get_formula (void) const override;

    void functions (void) const;
};

#endif // GRETL4PY_MODEL_H
