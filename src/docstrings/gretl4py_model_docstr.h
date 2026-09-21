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

#pragma once

namespace doc::Model {

    inline constexpr char ols[] = R"doc(
    Estimate a linear model by ordinary least squares.

    Estimates an OLS regression with the specified dependent variable and
    regressors. The model can be specified using a gretl-style variable list,
    a formula, or a Python list of variables.

    Parameters
    ----------
    formula : str
        Model specification as a gretl-style variable list or formula.
        Variables may be specified by name or index; the value 0 denotes
        the constant term.
    varlist : list
        Model specification as a list of variables, with the dependent
        variable first. Variables may be specified by name or index; the
        value 0 denotes the constant term.

    Other Parameters
    ----------------
    vcv : bool
        Print the covariance matrix of the coefficient estimates.
    robust : bool
        Use heteroskedasticity-robust standard errors.
    cluster : str
        Compute clustered standard errors using the specified variable to
        define clusters.
        jackknife : bool
        Use the jackknife variant of robust standard errors (HC3a).
        simple_print : bool
        Suppress printing of auxiliary model statistics.
        quiet : bool
        Suppress printed output.
        anova : bool
        Print an analysis of variance (ANOVA) table.
        no_df_corr : bool
        Suppress the degrees-of-freedom correction when estimating the
        error variance.
        print_final : bool
        Within a loop, print estimation results only on the final iteration.

    Notes
    -----
    The specific HC estimator used with ``robust=True`` is determined by
    the current robust covariance settings. ``jackknife=True`` selects
    HC3a, while ``cluster`` overrides the HC variant and computes standard
    errors clustered by the distinct values of the specified variable.

    By default, the estimated error variance uses a degrees-of-freedom
    correction. This correction is disabled with ``no_df_corr=True``.

    Examples
    --------
    ``gretl.ols("y 0 x1 x2")``
    ``gretl.ols("1 0 2 3")``
    ``gretl.ols("y ~ 0 + x1 + x2")``
    )doc";

} // namespace doc::Model

