/*
 *  gretl -- Gnu Regression, Econometrics and Time-series Library
 *  Copyright (C) 2001 Allin Cottrell and Riccardo "Jack" Lucchetti
 *
 *  gretl4py -- Python bindings for gretl
 *  Copyright (C) 2024 Marcin Błażejowski
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
#include "gretl4py_dataset.h"
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
// Class to handle different gretl model types
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
    gretlopt opt_ = OPT_NONE;           // instance-wide options (including tests and forecasts)
    mutable int err_ = 0;               // holds libgretl errors (instance-wide)
    GretlDatasetPtr dataset_;           // dataset used
    gretl_list_ptr var_list_{nullptr};  // libgretl list of variables
    bool var_list_from_list_ = false;   // whether varlist was passed directly as Python's list (i.e. not as formula)
    GretlBundlePtr gb_model_ = nullptr; // bundle with results
    GretlPrint printer_ = GretlPrint(); // printer (instance wide)
    std::string summary_;               // model summary to print on python side

    template <typename T>
    void save_model_ (T *model);

    template <typename T>
    GretlBundlePtr do_restrict_ (T *model, const py::list &impose, const py::kwargs &kwargs);

    template <typename T>
    void gretl_do_forecast_ (void *model, const py::args &args, const py::kwargs &kwargs);

    void check_if_estimated_ (void) const;

    template <typename T>
    std::string do_save_uhat_ (T *model, int eqn, const std::string &name) const;

    std::string extract_additional_vars_ (py::handle src) const;

    void prepare_plot_ (GretlPlot &plotter, gretlopt opt, bool multiple = false);
public:
    explicit GretlModel (GretlObjType model_obj_type, GretlCmdIndex ci, const py::kwargs &kwargs);
    virtual ~GretlModel (void);

    virtual void fit (void) = 0;

    virtual void forecast (const py::args &args, const py::kwargs &kwargs) = 0;

    virtual std::string get_formula (void) const = 0;

    virtual py::object get_accessor (const std::string &name) const = 0;

    int get_id (void) const;

    std::string repr_impl (void) const noexcept;                // for __repr__ binding

    GretlBundlePtr get_bundle (void) const noexcept;            // return pointer to '$model' bundle

    const GretlDatasetPtr get_dataset (void) const noexcept;    // return pointer to dataset

    [[nodiscard]] bool estimated (void) const noexcept;
};

//
// Native Single Equation libgretl MODEL (NSE)
// extends: GretlModel
//
class GretlModel_NSE : public GretlModel
{
protected:
    MODEL *model_ = nullptr;    // libgretl MODEL struct

    template <GretlCmdIndex CI>
    void regexp_extract_ (const std::string &formula, std::string &formula_left, std::string &formula_right);    // for extracting formula from regular expression

    DATASET *get_model_dataset_ (gretlopt opt = OPT_NONE);
public:
    GretlModel_NSE (GretlCmdIndex ci, const py::kwargs &kwargs);
    virtual ~GretlModel_NSE (void);

    void fit (void) override = 0;

    std::string get_formula (void) const override = 0;

    void forecast (const py::args &args, const py::kwargs &kwargs) override;

    py::object get_accessor (const std::string &name) const override;

    GretlBundlePtr restrict (const py::list &impose, const py::kwargs &kwargs);

    template <ModelDataIndex IDX>
    char *gretl_get_fit_or_resid (const std::string &name = "") const;

    GretlBundlePtr test (const std::string &type, const py::kwargs &kwargs);

    GretlBundlePtr test (const std::string &type, const py::object &varlist, const py::kwargs &kwargs);

    template <GretlCmdIndex CI>
    std::pair<GretlBundlePtr, GretlModelPtr> add_omit_auto (const py::kwargs &kwargs);

    double adj_rsq (void) const;

    py::object plot (const std::string &type, const std::string &xvar = "");
};

//
// NSE via arima plugin
// extends: GretlModel_NSE
//
class GretlModel_NSE_arima : public GretlModel_NSE
{
private:
    const std::string y_;       // name of y process
    int y_number_;              // number of y process in the context of GD
    int seasonal_ = 0;          // number of seasonal elements
    int exogenous_ = 0;         // number of exogenous variables
    gretl_list_ptr indepvars_;  // list of exogenous variables
    int p_, d_, q_;
    int P_, D_, Q_;
public:
    GretlModel_NSE_arima (const std::string &depvar, const py::tuple &tup, const py::kwargs &kwargs);
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
    std::string formula_right_;         // right-hand site of formula use in MIDASREG

    GretlModel_NSE_Driver (void) : GretlModel_NSE(CI, py::dict()), formula_(std::string("add/omit model")) {}    // special case for models generated by add/omit tests
public:
    GretlModel_NSE_Driver (const std::string &formula, const py::kwargs &kwargs);
    GretlModel_NSE_Driver (const py::list &varlist, const py::kwargs &kwargs);

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
    GretlModel_NSE_Driver_quantreg (const std::string &formula, const double &tau, const py::kwargs &kwargs);
    GretlModel_NSE_Driver_quantreg (const std::string &formula, const py::array_t<double> &tau, const py::kwargs &kwargs);
    GretlModel_NSE_Driver_quantreg (const py::list &varlist, const double &tau, const py::kwargs &kwargs);
    GretlModel_NSE_Driver_quantreg (const py::list &varlist, const py::array_t<double> &tau, const py::kwargs &kwargs);

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
    GretlModel_NSE_Driver_dpanel (const std::string &formula, const int &p, const py::kwargs &kwargs);
    GretlModel_NSE_Driver_dpanel (const std::string &formula, const py::array_t<int> &p, const py::kwargs &kwargs);

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
    GretlModel_NSE_Driver_tobit (const std::string &formula, const py::kwargs &kwargs);
    GretlModel_NSE_Driver_tobit (const py::list &varlist, const py::kwargs &kwargs);

    void fit (void) override;
};

//
// OLS for add/omit tests
// extends: GretlModel_NSE_Driver<lsq, OLS>
//
template <GretlCmdIndex CI>
class GretlModel_NSE_OLS_add_omit : public GretlModel_NSE_Driver<lsq, OLS>
{
private:
    MODEL *src_model_;                  // source model
    gretlopt test_opt_;
    const int *addvars_;                // list of vars to be added
public:
    GretlModel_NSE_OLS_add_omit (MODEL *src, GretlDatasetPtr src_dataset, gretlopt opt, const int *addvars = nullptr);
    ~GretlModel_NSE_OLS_add_omit (void);

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
    void add_line_to_parse (const std::string &keyword, const std::string &val);

    void add_init_value (const std::string &varname, double value);
};

//
// VAR libgretl MODEL
// extends: GretlModel
//
class GretlModel_VAR : public GretlModel
{
protected:
    std::string vars_;              // list of endogenous (+ exogenous) variables
    const int order_;               // VAR order
    GRETL_VAR *model_ = nullptr;    // libgretl GRETL_VAR struct

    int find_variable_nr_ (const std::string &variable_name);

    int find_variable_nr_ (int id);
public:
    GretlModel_VAR (const std::string &ylist, const int order, GretlCmdIndex ci, const py::kwargs &kwargs);
    GretlModel_VAR (const py::list &varlist, const int order, GretlCmdIndex ci, const py::kwargs &kwargs);
    virtual ~GretlModel_VAR (void);

    void fit (void) override;

    void forecast (const py::args &args, const py::kwargs &kwargs) override;

    std::string get_formula (void) const override;

    py::object get_accessor (const std::string &name) const override;

    std::string save_uhat (int eqn, const std::string &name = "") const;

    GretlBundlePtr test (const std::string &type, const py::kwargs &kwargs);

    GretlMatrixPtr irf (int target, int shock, double alpha = 0);

    GretlMatrixPtr irf (const std::string &target, const std::string &shock, double alpha = 0);

    template <typename T>
    void set_cholesky_order (const T &varlist);

    py::object plot (const std::string &type, std::optional<int> equation = std::nullopt,
                     std::optional<std::string> mode = std::nullopt, std::optional<int> periods = std::nullopt,
                     std::optional<double> alpha = std::nullopt, std::optional<bool> error_bars = std::nullopt,
                     std::variant<int, std::string> targ = -1, std::variant<int, std::string> shock = -1,
                     std::optional<bool> histogram = true);
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
    GretlModel_VAR_VECM (const std::string &ylist, const int order, const int rank, const py::kwargs &kwargs);
    GretlModel_VAR_VECM (const py::list &varlist, const int order, const int rank, const py::kwargs &kwargs);

    void fit (void) override;

    GretlBundlePtr restrict (const py::list &impose, const py::kwargs &kwargs);

    std::string save_ec_term (const int nr, const std::string &name = "") const;

    py::object plot_ec (void);
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
    equation_system *model_ = nullptr;      // libgretl equation_system struct
public:
    GretlModel_NME (const std::string &method, const py::kwargs &kwargs);
    virtual ~GretlModel_NME (void);

    void fit (void) override;

    void forecast (const py::args &args, const py::kwargs &kwargs) override;

    std::string get_formula (void) const override;

    py::object get_accessor (const std::string &name) const override;

    GretlBundlePtr restrict (const py::list &impose, const py::kwargs &kwargs);

    std::string save_uhat (int eqn, const std::string &name = "") const;
};

#endif // GRETL4PY_MODEL_H
