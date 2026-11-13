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

#ifndef GRETL4PY_MODEL_INSTANTIATION_H
#define GRETL4PY_MODEL_INSTANTIATION_H

// explicit instantiation declarations for required model template specializations
extern template class GretlModel_NSE_Driver<lsq, OLS>;
extern template class GretlModel_NSE_OLS_add_omit<ADD>;
extern template class GretlModel_NSE_OLS_add_omit<OMIT>;
extern template class GretlModel_NSE_Driver<lsq, WLS>;
extern template class GretlModel_NSE_Driver<logit_probit, LOGIT>;
extern template class GretlModel_NSE_Driver<logit_probit, PROBIT>;
extern template class GretlModel_NSE_Driver<count_model, POISSON>;
extern template class GretlModel_NSE_Driver<count_model, NEGBIN>;
extern template class GretlModel_NSE_Driver<biprobit_model, BIPROBIT>;
extern template class GretlModel_NSE_Driver<duration_model, DURATION>;
extern template class GretlModel_NSE_Driver<heckit_model, HECKIT>;
extern template class GretlModel_NSE_Driver<logistic_driver, LOGISTIC>;
extern template class GretlModel_NSE_Driver<ar_model, AR>;
extern template class GretlModel_NSE_Driver<ar1_model, AR1>;
extern template class GretlModel_NSE_Driver<lad_model, LAD>;
extern template class GretlModel_NSE_Driver<garch, GARCH>;
extern template class GretlModel_NSE_Driver<mp_ols, MPOLS>;
extern template class GretlModel_NSE_Driver<panel_model, PANEL>;
extern template class GretlModel_NSE_Driver<ivreg, IVREG>;
extern template class GretlModel_NSE_Driver<tobit_model, TOBIT>;
extern template class GretlModel_NSE_Driver<hsk_model, HSK>;
extern template class GretlModel_NSE_Driver<interval_model, INTREG>;
extern template class GretlModel_NSE_Driver<midas_model, MIDASREG>;
extern template class GretlModel_NSE_nl<NLS>;
extern template class GretlModel_NSE_nl<MLE>;
extern template class GretlModel_NSE_nl<GMM>;

#endif // GRETL4PY_MODEL_INSTANTIATION_H
