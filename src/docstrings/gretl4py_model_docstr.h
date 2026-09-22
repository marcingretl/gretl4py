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
    Specify a linear model for estimation by ordinary least squares.

    Creates an OLS model specification with the specified dependent variable
    and regressors. The model can be specified using a gretl-style variable
    list, a formula, or a Python list of variables.

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

    Returns
    -------
    gretl.Model_ols
    An OLS model instance derived from the abstract ``gretl.Model``
    base class. Call ``fit()`` to estimate the model.

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
    ``gretl.ols("y 0 x1 x2").fit()``
    ``gretl.ols("1 0 2 3").fit()``
    ``gretl.ols("y ~ 0 + x1 + x2").fit()``
    ``gretl.ols([1, 0, 2, 3]).fit()``
    ``gretl.ols(["y", "const", "x1", "x2"]).fit()``
    )doc";

    inline constexpr char wls[] = R"doc(
    Specify a linear model for estimation by weighted least squares.

    Creates a WLS model specification using the first variable as the
    weighting variable, followed by the dependent variable and regressors.
    If ``w`` is the positive square root of the weighting variable, estimation
    is equivalent to OLS on the correspondingly weighted variables.

    Parameters
    ----------
    formula : str
        Model specification as a gretl-style variable list or formula.
        The first variable is the weighting variable, followed by the
        dependent variable and regressors.
    varlist : list
        Model specification as a list of variables, with the weighting
        variable first, the dependent variable second, followed by the
        regressors.

    Other Parameters
    ----------------
    vcv : bool
        Print the covariance matrix of the coefficient estimates.
    robust : bool
        Use heteroskedasticity-robust standard errors.
    quiet : bool
        Suppress printed output.
        allow_zeros : bool
        Allow zero weights together with positive weights.

    Returns
    -------
    gretl.Model_wls
    A WLS model instance derived from the abstract ``gretl.Model``
    base class. Call ``fit()`` to estimate the model.

    Notes
    -----
    The weighting variable should normally contain strictly positive values.
    If it is a 0/1 dummy variable, WLS is equivalent to OLS on observations
    for which the weight equals one. Other combinations of zero and positive
        weights require ``allow_zeros=True``.

    Examples
    --------
    ``gretl.wls("year employ const prdefl gnp").fit()``
    ``gretl.wls(["year", "employ", "const", "prdefl", "gnp"]).fit()``
    )doc";

    inline constexpr char logit[] = R"doc(
    Specify a logit model for estimation by maximum likelihood.

    Creates a logit model specification with the specified dependent variable
    and regressors. For a binary dependent variable, binary logit is used.
    For a non-binary discrete dependent variable, ordered logit is used by
    default; multinomial logit can be requested via ``multinomial=True``.

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
    robust : bool
        Use QML (Huber-White) robust standard errors.
    cluster : str
        Compute clustered standard errors using the specified variable to
        define clusters.
    multinomial : bool
    Estimate multinomial rather than ordered logit for a non-binary
    discrete dependent variable.
    vcv : bool
    Print the covariance matrix of the coefficient estimates.
    verbose : bool
    Print details of the numerical iterations.
    quiet : bool
    Suppress printed output.
    p_values : bool
    Report p-values instead of slopes for binary logit.
    estrella : bool
    Use Estrella's rather than McFadden's pseudo-R-squared.

    Returns
    -------
    gretl.Model_logit
    A logit model instance derived from the abstract ``gretl.Model``
    base class. Call ``fit()`` to estimate the model.

    Notes
    -----
    For binary logit, coefficients represent effects on the log odds. By
    default, slopes evaluated at the means of the regressors are reported
    in place of p-values; use ``p_values=True`` to report p-values instead.

    By default, standard errors are based on the negative inverse of the
    Hessian. ``robust=True`` requests QML standard errors, while ``cluster``
    requests cluster-robust standard errors.

    Examples
    --------
    ``gretl.logit("y 0 x1 x2").fit()``
    ``gretl.logit(["y", "const", "x1", "x2"]).fit()``
    )doc";

    inline constexpr char probit[] = R"doc(
    Specify a probit model for estimation by maximum likelihood.

    Creates a probit model specification with the specified dependent variable
    and regressors. For a binary dependent variable, binary probit is used.
    For a non-binary discrete dependent variable, ordered probit is used.
    For panel data, a random-effects probit model can also be estimated.

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
    robust : bool
        Use QML (Huber-White) robust standard errors.
    cluster : str
        Compute clustered standard errors using the specified variable to
        define clusters.
    vcv : bool
    Print the covariance matrix of the coefficient estimates.
    verbose : bool
    Print details of the numerical iterations.
    quiet : bool
    Suppress printed output.
    p_values : bool
    Report p-values instead of slopes for binary probit.
    estrella : bool
    Use Estrella's rather than McFadden's pseudo-R-squared.
    random_effects : bool
    Estimate a random-effects probit model for panel data.
    quadpoints : int
    Number of Gauss-Hermite quadrature points used for random-effects
    estimation. The default is 32.

    Returns
    -------
    gretl.Model_probit
    A probit model instance derived from the abstract ``gretl.Model``
    base class. Call ``fit()`` to estimate the model.

    Notes
    -----
    For binary probit, slopes evaluated at the means of the regressors are
    reported in place of p-values by default; use ``p_values=True`` to
    report p-values instead.

    For random-effects panel probit, the likelihood is evaluated using
    Gauss-Hermite quadrature. Increasing ``quadpoints`` may improve numerical
    accuracy at the cost of additional computation time.

    Examples
    --------
    ``gretl.probit("y 0 x1 x2").fit()``
    ``gretl.probit(["y", "const", "x1", "x2"]).fit()``
    )doc";

    inline constexpr char poisson[] = R"doc(
    Specify a Poisson regression model for estimation by maximum likelihood.

    Creates a Poisson model specification with the specified dependent
    variable and regressors. The dependent variable represents a count and
    must contain only non-negative integer values. The conditional mean is
    modeled using a log link.

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
    offset : str
        Name of a strictly positive scale variable. Its logarithm is added
        to the linear predictor with a coefficient fixed at 1.
    robust : bool
        Use QML (Huber-White) robust standard errors.
    cluster : str
    Compute clustered standard errors using the specified variable to
    define clusters.
    vcv : bool
    Print the covariance matrix of the coefficient estimates.
    verbose : bool
    Print details of the numerical iterations.
    quiet : bool
    Suppress printed output.

    Returns
    -------
    gretl.Model_poisson
    A Poisson model instance derived from the abstract ``gretl.Model``
    base class. Call ``fit()`` to estimate the model.

    Notes
    -----
    By default, standard errors are based on the negative inverse of the
    Hessian. ``robust=True`` requests QML standard errors, while ``cluster``
    requests cluster-robust standard errors.

    The ``offset`` option is useful when the expected count is proportional
    to a known exposure or scale variable. The offset variable must be
    strictly positive.

    Examples
    --------
    ``gretl.poisson("y 0 x1 x2").fit()``
    ``gretl.poisson(["y", "const", "x1", "x2"]).fit()``
    )doc";

    inline constexpr char negbin[] = R"doc(
    Specify a Negative Binomial regression model for estimation by maximum
    likelihood.

    Creates a Negative Binomial model specification with the specified
    dependent variable and regressors. The dependent variable represents a
    count and must contain only non-negative integer values. By default, the
    NegBin 2 specification is used.

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
    offset : str
        Name of a strictly positive scale variable. Its logarithm is added
        to the linear predictor with a coefficient fixed at 1.
    model1 : bool
        Use the NegBin 1 specification instead of NegBin 2.
        robust : bool
        Use QML robust standard errors.
        cluster : str
        Compute clustered standard errors using the specified variable to
        define clusters.
        opg : bool
        Compute the covariance matrix using the Outer Product of the
        Gradient (OPG).
        vcv : bool
        Print the covariance matrix of the coefficient estimates.
        verbose : bool
        Print details of the numerical iterations.
        quiet : bool
        Suppress printed output.

    Returns
    -------
    gretl.Model_negbin
    A Negative Binomial model instance derived from the abstract
    ``gretl.Model`` base class. Call ``fit()`` to estimate the model.

    Notes
    -----
    In the default NegBin 2 model, the conditional variance is
    ``mu * (1 + alpha * mu)``. With ``model1=True``, the NegBin 1
    specification uses ``mu * (1 + alpha)``. The Poisson model is the
    restricted case in which ``alpha = 0``.

    By default, standard errors are based on a numerical approximation to
    the Hessian. ``opg=True`` uses the OPG covariance matrix, while
    ``robust=True`` computes a QML sandwich covariance matrix.

    Examples
    --------
    ``gretl.negbin("y 0 x1 x2").fit()``
    ``gretl.negbin(["y", "const", "x1", "x2"]).fit()``
    )doc";

    inline constexpr char biprobit[] = R"doc(
    Specify a bivariate probit model for estimation by maximum likelihood.

    Creates a bivariate probit model specification for two binary dependent
    variables. Each equation may have its own set of regressors. The two
    disturbances are assumed to follow a bivariate normal distribution with
    correlation parameter ``rho``.

    Parameters
    ----------
    formula : str
        Model specification as two gretl-style variable lists separated by
        a semicolon. The first variable in each list is the dependent
        variable, followed by the regressors.
    varlist : list
        Model specification as a list containing two variable lists, one
        for each equation. The first variable in each inner list is the
        dependent variable, followed by the regressors.

    Other Parameters
    ----------------
    vcv : bool
        Print the covariance matrix of the coefficient estimates.
    robust : bool
        Use QML robust standard errors.
    cluster : str
        Compute clustered standard errors using the specified variable to
        define clusters.
        opg : bool
        Compute the covariance matrix using the Outer Product of the
        Gradient (OPG).
        save_xbeta : bool
        Store the index functions for the two equations as fitted values.
        verbose : bool
        Print additional information during estimation.

    Returns
    -------
    gretl.Model_biprobit
    A bivariate probit model instance derived from the abstract
    ``gretl.Model`` base class. Call ``fit()`` to estimate the model.

    Notes
    -----
    By default, standard errors are based on the analytical Hessian.
    ``opg=True`` uses the OPG covariance matrix, while ``robust=True``
    computes a QML sandwich covariance matrix.

    The estimated correlation ``rho`` between the disturbances of the two
    equations is included as the last element of the coefficient vector.

    By default, fitted values contain probabilities for the four joint
    outcomes ``(1,1)``, ``(1,0)``, ``(0,1)`` and ``(0,0)``. With
    ``save_xbeta=True``, they contain the two index functions instead.

    Examples
    --------
    ``gretl.biprobit("y1 y2 const x1 x2 ; const x3 x4").fit()``

    ``gretl.biprobit([["y1", "y2", "const", "x1", "x2"], ["const", "x3", "x4"]]).fit()``
    )doc";

    inline constexpr char duration[] = R"doc(
    Specify a parametric duration model for estimation by maximum likelihood.

    Creates a duration model specification with the specified dependent
    variable and regressors. The dependent variable represents duration and
    must be strictly positive. The Weibull distribution is used by default;
    exponential, log-logistic and log-normal specifications are also
    available.

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
    censvar : str
        Right-censoring variable. Non-zero values identify censored
        observations.
    exponential : bool
        Use the exponential distribution.
        loglogistic : bool
        Use the log-logistic distribution.
        lognormal : bool
        Use the log-normal distribution.
        medians : bool
        Return conditional medians rather than means as fitted values.
        robust : bool
        Use QML robust standard errors.
        cluster : str
        Compute clustered standard errors using the specified variable to
        define clusters.
        vcv : bool
        Print the covariance matrix of the coefficient estimates.
        verbose : bool
        Print details of the numerical iterations.
        quiet : bool
        Suppress printed output.

    Returns
    -------
    gretl.Model_duration
    A duration model instance derived from the abstract ``gretl.Model``
    base class. Call ``fit()`` to estimate the model.

    Notes
    -----
    Right-censored observations can be identified using ``censvar``. By
    default, fitted values are conditional means of duration; with
    ``medians=True`` they are conditional medians instead.

    Examples
    --------
    ``gretl.duration("y const x1 x2").fit()``
    ``gretl.duration(["y", "const", "x1", "x2"]).fit()``
    )doc";

    inline constexpr char heckit[] = R"doc(
    Specify a Heckman sample-selection model for estimation.

    Creates a sample-selection model specification consisting of an outcome
    equation and a selection equation. The dependent variable in the
    selection equation must be binary. Parameters are estimated by maximum
    likelihood by default.

    Parameters
    ----------
    formula : str
        Model specification as two gretl-style variable lists separated by
        a semicolon. The first list specifies the outcome equation and the
        second specifies the selection equation.
    varlist : list
        Model specification as a list containing two variable lists. The
        first specifies the outcome equation and the second specifies the
        selection equation. In each list, the dependent variable comes
        first, followed by the regressors.

    Other Parameters
    ----------------
    quiet : bool
        Suppress printed output.
    two_step : bool
        Use Heckman's two-step estimator instead of maximum likelihood.
    vcv : bool
    Print the covariance matrix of the coefficient estimates.
    opg : bool
    Compute the covariance matrix using the Outer Product of the
    Gradient (OPG).
    robust : bool
    Use QML robust standard errors.
    cluster : str
    Compute clustered standard errors using the specified variable to
    define clusters.
    verbose : bool
    Print additional information during estimation.

    Returns
    -------
    gretl.Model_heckit
    A Heckman sample-selection model instance derived from the abstract
    ``gretl.Model`` base class. Call ``fit()`` to estimate the model.

    Notes
    -----
    By default, estimation is by maximum likelihood and the covariance
    matrix is based on the negative inverse of the Hessian. ``opg=True``
    uses the OPG covariance matrix, while ``robust=True`` computes a QML
    sandwich covariance matrix.

    With ``two_step=True``, Heckman's two-step estimator is used and the
    covariance matrix for the outcome equation is adjusted for the
    two-step estimation procedure.

    Examples
    --------
    ``gretl.heckit("y const x1 x2 ; s const x3 x4").fit()``

    ``gretl.heckit([["y", "const", "x1", "x2"], ["s", "const", "x3", "x4"]]).fit()``
    )doc";

    inline constexpr char logistic[] = R"doc(
    Specify a linear model using a logistic transformation of the dependent
    variable.

    Creates a model specification in which the dependent variable is
    transformed using the logistic transformation and the resulting model
    is estimated by OLS. The dependent variable must be strictly positive.
    Fitted values and residuals are transformed back to the original scale.

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
    ymax : float
        Specify the asymptotic maximum of the dependent variable. It must
        exceed all observed values.
    robust : bool
        Use heteroskedasticity-robust standard errors.
        cluster : str
        Compute clustered standard errors using the specified variable to
        define clusters.
        vcv : bool
        Print the covariance matrix of the coefficient estimates.
        fixed_effects : bool
        Estimate a fixed-effects logistic model for panel data.
        quiet : bool
        Suppress printed output.

    Returns
    -------
    gretl.Model_logistic
    A logistic model instance derived from the abstract ``gretl.Model``
    base class. Call ``fit()`` to estimate the model.

    Notes
    -----
    If the dependent variable lies between 0 and 1, ``ymax`` defaults to 1;
    if it lies between 0 and 100, it defaults to 100. A different maximum
        can be specified explicitly using ``ymax``.

    For panel data, ``fixed_effects=True`` applies the within transformation
    to the logistic transform of the dependent variable. For a binary
    dependent variable, use ``logit`` instead.

    Examples
    --------
    ``gretl.logistic("y const x1 x2").fit()``
    ``gretl.logistic(["y", "const", "x1", "x2"]).fit()``
    )doc";

} // namespace doc::Model

