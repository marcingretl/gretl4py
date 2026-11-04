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

    inline constexpr char dataset[] = R"doc(
    Dataset used by the model.

    Returns
    -------
    gretl.Dataset
        Dataset associated with this Model instance.
    )doc";

    inline constexpr char estimated[] = R"doc(
    Whether the model has been estimated.

    Returns
    -------
    bool
        ``True`` if the model has been estimated, ``False`` otherwise.
    )doc";

    inline constexpr char adj_rsq[] = R"doc(
    Adjusted R-squared of the estimated model.

    Returns
    -------
    float
        Adjusted R-squared.
    )doc";

    inline constexpr char get_accessor[] = R"doc(
    Retrieve a gretl dollar accessor.

    Retrieves accessors related either to the Model instance or to general
    gretl accessors.

    Parameters
    ----------
    name : str
        Name of the accessor, including the ``$`` prefix.

    Returns
    -------
    object
        Value of the requested accessor.

    Examples
    --------
    ``model.get_accessor("$coeff")``
    ``model.get_accessor("$version")``
    )doc";

    inline constexpr char get_formula[] = R"doc(
    Return the model specification.

    Returns
    -------
    str
        Model specification.
    )doc";

    inline constexpr char get_bundle[] = R"doc(
    Return the model results as a gretl Bundle.

    The returned Bundle corresponds to gretl's ``$model`` accessor and
    contains model-related results and accessors.

    Returns
    -------
    gretl.Bundle
        Bundle containing the model results.
    )doc";

    inline constexpr char fcast[] = R"doc(
    Generate forecasts based on the estimated model.

    Parameters
    ----------
    *args
        Forecast range, optional variable name, and, for recursive
        forecasts, the number of steps ahead.

    Other Parameters
    ----------------
    dynamic : bool
        Generate dynamic forecasts.
    static : bool
        Generate static forecasts.
    out_of_sample : bool
        Generate post-sample forecasts.
    no_stats : bool
        Suppress forecast statistics.
    stats_only : bool
        Print forecast statistics only.
    quiet : bool
        Suppress all output.
    recursive : bool
        Generate recursive forecasts.
    exponentiate : bool or int
        Exponentiate forecasts of a logged dependent variable. An integer
        value specifies the bias adjustment method.
    integrate : bool
        Integrate forecasts of a differenced dependent variable.
    all_probs : bool
        Generate probabilities for all possible outcomes.
    plot : str
        Filename for saving the forecast plot.

    Examples
    --------
    ``model.fcast("1997:1", "2001:4", "f1")``
    ``model.fcast("fit2")``
    ``model.fcast("2004:1", "2008:3", 4, "rfcast", recursive=True)``
    )doc";

    inline constexpr char fit[] = R"doc(
    Estimate the specified model.

    Executes the underlying implementation of the estimator specified
    by this Model instance.

    Returns
    -------
    self
        The estimated Model instance.
    )doc";

    inline constexpr char restrict[] = R"doc(
    Impose restrictions on the estimated model.

    Parameters
    ----------
    impose : list
        List of strings defining the restrictions.

    Other Parameters
    ----------------
    quiet : bool
        Suppress restricted estimates.
    silent : bool
        Suppress all output.
    wald : bool
        Use a Wald test for system estimators.
    bootstrap : bool
        Bootstrap the test if possible.
    full : bool
        Set restricted estimates as the last model, where applicable.

    Returns
    -------
    gretl.Bundle
    Bundle containing the test statistic (``test``), p-value
    (``pvalue``), and printed output (``summary``).

    Examples
    --------
    ``model.restrict(["b[1] = 0", "b[2] - b[3] = 0"])``
    )doc";

    inline constexpr char save_yhat[] = R"doc(
    Add fitted values to the dataset.

    Parameters
    ----------
    name : str, optional
        Name of the generated series. If omitted, a name is generated
        according to the gretl naming convention.

    Returns
    -------
    str
        Name of the series added to the dataset.
    )doc";

    inline constexpr char save_uhat[] = R"doc(
    Add residuals to the dataset.

    The interpretation of the residuals depends on the estimator. For
    example, for an ARMA model they are one-step-ahead forecast errors,
    while for a probit model they are generalized residuals.

    Parameters
    ----------
    name : str, optional
        Name of the generated series. If omitted, a name is generated
        according to the gretl naming convention.

    Returns
    -------
    str
        Name of the series added to the dataset.
    )doc";

    inline constexpr char save_uhat_sq[] = R"doc(
    Add squared residuals to the dataset.

    Parameters
    ----------
    name : str, optional
        Name of the generated series. If omitted, a name is generated
        according to the gretl naming convention.

    Returns
    -------
    str
        Name of the series added to the dataset.
    )doc";

    inline constexpr char save_ahat[] = R"doc(
    Add estimates of the individual effects to the dataset.

    This method is applicable to fixed-effects and random-effects panel
    data models.

    Parameters
    ----------
    name : str, optional
        Name of the generated series. If omitted, a name is generated
        according to the gretl naming convention.

    Returns
    -------
    str
        Name of the series added to the dataset.
    )doc";

    inline constexpr char save_h[] = R"doc(
    Add the estimated conditional variance to the dataset.

    This method is applicable to GARCH models.

    Parameters
    ----------
    name : str, optional
        Name of the generated series. If omitted, a name is generated
        according to the gretl naming convention.

    Returns
    -------
    str
        Name of the series added to the dataset.
    )doc";

    inline constexpr char nls_add_params[] = R"doc(
    Specify the parameters to be estimated.

    Use this method when analytical derivatives are not supplied. The
    Jacobian is then computed numerically.

    Parameters
    ----------
    line : str
        Space- or comma-separated list of parameter names.

    Returns
    -------
    self
        The Model instance.

    Examples
    --------
    ``model.add_params("a b g")``
    )doc";

    inline constexpr char mle_add_params[] = R"doc(
    Specify the parameters to be estimated.

    Use this method when analytical derivatives are not supplied. The
    derivatives are then computed numerically.

    Parameters
    ----------
    line : str
        Space-separated list of parameters. Parameters may be scalars,
        vectors, or a combination of the two.

    Returns
    -------
    self
        The Model instance.

    Examples
    --------
    ``model.add_params("p")``
    )doc";

    inline constexpr char gmm_add_params[] = R"doc(
    Specify the parameters to be estimated.

    Parameters
    ----------
    line : str
        Space-separated list of parameters.

    Returns
    -------
    self
        The Model instance.

    Examples
    --------
    ``model.add_params("b")``
    )doc";

    inline constexpr char add_param_names[] = R"doc(
    Specify names for the model parameters.

    These names are used when displaying estimation results and may be
    more informative than the parameter names used in the model
    specification.

    Parameters
    ----------
    line : str
        Space-separated list of parameter names.

    Returns
    -------
    self
        The Model instance.

    Examples
    --------
    ``model.add_param_names("alpha beta gamma")``
    )doc";

    inline constexpr char nls_add_deriv[] = R"doc(
    Add an analytical derivative of the regression function.

    Parameters
    ----------
    line : str
        Derivative specification in the form ``parameter = expression``.

    Returns
    -------
    self
        The Model instance.

    Examples
    --------
    ``model.add_deriv("a = 1")``
    ``model.add_deriv("b = Y^g")``
    ``model.add_deriv("g = b * Y^g * log(Y)")``
    )doc";

    inline constexpr char mle_add_deriv[] = R"doc(
    Add an analytical derivative of the log-likelihood function.

    Parameters
    ----------
    line : str
        Derivative specification in the form ``parameter = expression``.

    Returns
    -------
    self
        The Model instance.

    Examples
    --------
    ``model.add_deriv("p = X/p - (1-X)/(1-p)")``
    )doc";

    inline constexpr char nls_add_initval[] = R"doc(
    Set an initial value for a model parameter.

    Parameters
    ----------
    name : str
        Name of the parameter.
    value : float
        Initial value of the parameter.

    Returns
    -------
    self
        The Model instance.

    Examples
    --------
    ``model.add_initval("a", 1.0)``
    )doc";

    inline constexpr char mle_add_initval[] = R"doc(
    Set an initial value for a model parameter.

    Parameters
    ----------
    name : str
        Name of the parameter.
    value : float
        Initial value of the parameter.

    Returns
    -------
    self
        The Model instance.

    Examples
    --------
    ``model.add_initval("p", 0.5)``
    )doc";

    inline constexpr char gmm_add_initval[] = R"doc(
    Set an initial value for a model parameter.

    Parameters
    ----------
    name : str
        Name of the parameter.
    value : float
        Initial value of the parameter.

    Returns
    -------
    self
        The Model instance.

    Examples
    --------
    ``model.add_initval("b", 1.0)``
    )doc";

    inline constexpr char mle_add_statements[] = R"doc(
    Add a statement to the maximum likelihood specification.

    Statements may be used to compute auxiliary quantities required
    for evaluating the log-likelihood function.

    Parameters
    ----------
    line : str
        Hansl statement to be evaluated as part of the model specification.

    Returns
    -------
    self
        The Model instance.

    Examples
    --------
    ``model.add_statements("...")``
    )doc";

    inline constexpr char gmm_add_statements[] = R"doc(
    Add a statement to the GMM specification.

    Statements may be used to update quantities required for evaluating
    the orthogonality conditions.

    Parameters
    ----------
    line : str
        Hansl statement to be evaluated as part of the model specification.

    Returns
    -------
    self
        The Model instance.

    Examples
    --------
    ``model.add_statements("e = y - X*b")``
    )doc";

    inline constexpr char gmm_add_weights[] = R"doc(
    Specify the initial weighting matrix for GMM estimation.

    Parameters
    ----------
    line : str
        Name of the matrix containing the initial weights.

    Returns
    -------
    self
        The Model instance.

    Examples
    --------
    ``model.add_weights("V")``
    )doc";

    inline constexpr char gmm_add_orthog[] = R"doc(
    Add a set of orthogonality conditions for GMM estimation.

    Parameters
    ----------
    line : str
        Orthogonality specification, giving the residual or error term
        followed by a semicolon and the associated instruments.

    Returns
    -------
    self
        The Model instance.

    Examples
    --------
    ``model.add_orthog("e ; W")``
    )doc";

    inline constexpr char ols[] = R"doc(
    Specify a linear model for estimation by ordinary least squares.

    Creates an OLS model specification with the specified dependent variable
    and regressors. The model can be specified using a gretl-style variable
    list, a formula, or a Python list of variables.

    Parameters
    ----------
    formula : str or list
        Model specification as a gretl-style variable list or formula,
        or as a list of variable names or IDs. The dependent variable
        comes first; the value 0 denotes the constant term.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
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
    formula : str or list
        Model specification as a gretl-style variable list or formula,
        or as a list of variable names or IDs. The weighting variable
        comes first, followed by the dependent variable and regressors.
        The value 0 denotes the constant term.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
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
    formula : str or list
        Model specification as a gretl-style variable list or formula,
        or as a list of variable names or IDs. The dependent variable
        comes first; the value 0 denotes the constant term.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
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
    formula : str or list
        Model specification as a gretl-style variable list or formula,
        or as a list of variable names or IDs. The dependent variable
        comes first; the value 0 denotes the constant term.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
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

    By default, standard errors are based on the negative inverse of the
    Hessian. ``robust=True`` requests QML standard errors, while ``cluster``
    requests cluster-robust standard errors.

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
    formula : str or list
        Model specification as a gretl-style variable list or formula,
        or as a list of variable names or IDs. The dependent variable
        comes first; the value 0 denotes the constant term.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
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
    ``gretl.poisson("y 0 x1 x2 ; exposure").fit()``
    ``gretl.poisson([["y", "const", "x1", "x2"], ["exposure"]]).fit()``
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
    formula : str or list
        Model specification as a gretl-style variable list or as a list
        of variable names or IDs. The dependent variable comes first;
        the value 0 denotes the constant term. An optional offset variable
        may be specified after a semicolon, or using a nested Python list.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
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

    An optional offset variable accounts for exposure or scale effects.
    Its logarithm enters the linear predictor with a coefficient fixed
    at 1. The offset variable must be strictly positive.

    By default, standard errors are based on a numerical approximation to
    the Hessian. ``opg=True`` uses the OPG covariance matrix, while
    ``robust=True`` computes a QML sandwich covariance matrix.

    Examples
    --------
    ``gretl.negbin("y 0 x1 x2").fit()``
    ``gretl.negbin("y 0 x1 x2 ; exposure").fit()``
    ``gretl.negbin([["y", "const", "x1", "x2"], ["exposure"]]).fit()``
    )doc";

    inline constexpr char biprobit[] = R"doc(
    Specify a bivariate probit model for estimation by maximum likelihood.

    Creates a bivariate probit model specification for two binary dependent
    variables. The equations may share a common set of regressors or have
    separate sets of regressors. The disturbances follow a bivariate normal
    distribution with correlation parameter ``rho``.

    Parameters
    ----------
    formula : str or list
        Model specification as a gretl-style variable list or as a list
        of variable names or IDs. The first two variables are the dependent
        variables, followed by the regressors. The value 0 denotes the
        constant term. An optional second set of regressors may be specified
        after a semicolon or using a nested Python list.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
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
    If only one set of regressors is specified, it is used in both
    equations. Otherwise, the first set applies to the first equation
    and the second set to the second equation.

    By default, standard errors are based on the analytical Hessian.
    ``opg=True`` uses the OPG covariance matrix, while ``robust=True``
    computes a QML sandwich covariance matrix.

    The estimated correlation ``rho`` between the disturbances is included
    as the last element of the coefficient vector.

    By default, fitted values contain probabilities for the four joint
    outcomes ``(1,1)``, ``(1,0)``, ``(0,1)`` and ``(0,0)``. With
    ``save_xbeta=True``, they contain the two index functions instead.

    Examples
    --------
    ``gretl.biprobit("y1 y2 0 x1 x2").fit()``
    ``gretl.biprobit("y1 y2 0 x1 x2 ; 0 x3 x4").fit()``
    ``gretl.biprobit([["y1", "y2", 0, "x1", "x2"], [0, "x3", "x4"]]).fit()``
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
    formula : str or list
        Model specification as a gretl-style variable list or as a list
        of variable names or IDs. The dependent variable comes first;
        the value 0 denotes the constant term. An optional right-censoring
        variable may be specified after a semicolon or using a nested
        Python list.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
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
    Right-censored observations can be identified using an optional
    censoring variable. Non-zero values indicate right-censored cases.

    By default, fitted values are conditional means of duration; with
    ``medians=True`` they are conditional medians instead.

    Examples
    --------
    ``gretl.duration("y 0 x1 x2").fit()``
    ``gretl.duration("y 0 x1 x2 ; cens").fit()``
    ``gretl.duration([["y", 0, "x1", "x2"], ["cens"]]).fit()``
    )doc";

    inline constexpr char heckit[] = R"doc(
    Specify a Heckman sample-selection model for estimation.

    Creates a sample-selection model specification consisting of an outcome
    equation and a selection equation. The dependent variable in the
    selection equation must be binary. Parameters are estimated by maximum
    likelihood by default.

    Parameters
    ----------
    formula : str or list
        Model specification as two gretl-style variable lists separated by
        a semicolon, or as a nested Python list. The first list specifies
        the outcome equation and the second specifies the selection
        equation. In each list, the dependent variable comes first,
        followed by the regressors; the value 0 denotes the constant term.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
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
    ``gretl.heckit("y 0 x1 x2 ; ys 0 x3 x4").fit()``
    ``gretl.heckit([["y", 0, "x1", "x2"], ["ys", 0, "x3", "x4"]]).fit()``
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
    formula : str or list
        Model specification as a gretl-style variable list or formula,
        or as a list of variable names or IDs. The dependent variable
        comes first; the value 0 denotes the constant term.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
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
    If all values of the dependent variable lie between 0 and 1, ``ymax``
    defaults to 1; otherwise, if they lie between 0 and 100, it defaults
    to 100. A different maximum can be specified using ``ymax``.

    For panel data, ``fixed_effects=True`` applies the within transformation
    to the logistic transform of the dependent variable. For a binary
    dependent variable, use ``logit`` instead.

    Examples
    --------
    ``gretl.logistic("y 0 x1 x2").fit()``
    ``gretl.logistic(["y", "const", "x1", "x2"]).fit()``
    ``gretl.logistic("y 0 x1 x2", ymax=50).fit()``
    )doc";

    inline constexpr char ar[] = R"doc(
    Specify a linear regression model with autoregressive errors.

    Creates a model specification for estimation using the generalized
    Cochrane-Orcutt iterative procedure. The autoregressive structure of
    the error term is specified by one or more residual lags.

    Parameters
    ----------
    formula : str or list
        Model specification consisting of residual lags followed by a
        semicolon and a gretl-style variable list, or as a nested Python
        list. The first list specifies the residual lags; the second
        contains the dependent variable followed by the regressors.
        The value 0 denotes the constant term.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
    vcv : bool
        Print the covariance matrix of the coefficient estimates.
    quiet : bool
        Suppress printing of parameter estimates.

    Returns
    -------
    gretl.Model_ar
        An autoregressive-error model instance derived from the abstract
        ``gretl.Model`` base class. Call ``fit()`` to estimate the model.

    Notes
    -----
    Estimation uses the generalized Cochrane-Orcutt iterative procedure.
    Iteration terminates when successive error sums of squares differ by
    no more than 0.005 percent or after 20 iterations.

    The residual lags need not be consecutive, allowing higher-order
    autoregressive error structures to be specified directly.

    Examples
    --------
    ``gretl.ar("1 3 4 ; y 0 x1 x2 x3").fit()``
    ``gretl.ar([[1, 3, 4], ["y", 0, "x1", "x2", "x3"]]).fit()``
    )doc";

    inline constexpr char ar1[] = R"doc(
    Specify a linear regression model with first-order autoregressive errors.

    Creates a model specification for feasible GLS estimation under the
    assumption that the error term follows an AR(1) process. The default
    estimator is the iterative Cochrane-Orcutt procedure.

    Parameters
    ----------
    formula : str or list
        Model specification as a gretl-style variable list or formula,
        or as a list of variable names or IDs. The dependent variable
        comes first; the value 0 denotes the constant term.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
    hilu : bool
        Use the Hildreth-Lu search procedure.
    pwe : bool
        Use the Prais-Winsten estimator.
    vcv : bool
        Print the covariance matrix of the coefficient estimates.
    no_corc : bool
        Do not fine-tune Hildreth-Lu estimates using the Cochrane-Orcutt
        procedure.
    loose : bool
        Use a looser convergence criterion for iterative estimation.
        quiet : bool
        Suppress printed output.

    Returns
    -------
    gretl.Model_ar1
    An AR(1)-error model instance derived from the abstract
    ``gretl.Model`` base class. Call ``fit()`` to estimate the model.

    Notes
    -----
    By default, estimation uses the iterative Cochrane-Orcutt procedure.
    Convergence is reached when successive estimates of the autocorrelation
    coefficient differ by no more than ``1e-6``. With ``loose=True``, the
    tolerance is relaxed to ``0.001``. An error is reported if convergence
    is not achieved within 100 iterations.

    With ``pwe=True``, the Prais-Winsten estimator is used, retaining the
    first observation rather than discarding it as in Cochrane-Orcutt.

    With ``hilu=True``, the Hildreth-Lu search procedure is used and its
    results are fine-tuned by Cochrane-Orcutt unless ``no_corc=True``.
    ``no_corc`` has no effect with other estimators.

    Examples
    --------
    ``gretl.ar1("y 0 x1 x2").fit()``
    ``gretl.ar1(["y", "const", "x1", "x2"]).fit()``
    ``gretl.ar1("y 0 x1 x2", pwe=True).fit()``
    )doc";

    inline constexpr char lad[] = R"doc(
    Specify a least absolute deviations regression model.

    Creates a regression model specification in which the coefficients are
    chosen to minimize the sum of absolute deviations between observed
    and fitted values. Estimation uses the Barrodale-Roberts simplex
    algorithm.

    Parameters
    ----------
    formula : str or list
        Model specification as a gretl-style variable list or formula,
        or as a list of variable names or IDs. The dependent variable
        comes first; the value 0 denotes the constant term.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
    vcv : bool
        Print the covariance matrix of the coefficient estimates.
    no_vcv : bool
        Do not compute the covariance matrix or standard errors.
    quiet : bool
        Suppress printed output.

    Returns
    -------
    gretl.Model_lad
    A least absolute deviations model instance derived from the
    abstract ``gretl.Model`` base class. Call ``fit()`` to estimate
    the model.

    Notes
    -----
    Standard errors are computed by bootstrap using 500 replications.
    The covariance matrix printed with ``vcv=True`` is based on the
    same bootstrap. With ``no_vcv=True``, covariance matrix computation
    is skipped and standard errors are not available.

    A warning is issued if the solution is not unique.

    LAD estimation can be relatively slow for large samples or models
    with many regressors. In such cases, median regression via
    ``quantreg`` may be faster; ``quantreg`` uses the Frisch-Newton
    algorithm and analytical rather than bootstrap standard errors.

    Examples
    --------
    ``gretl.lad("y 0 x1 x2").fit()``
    ``gretl.lad(["y", "const", "x1", "x2"]).fit()``
    )doc";

    inline constexpr char garch[] = R"doc(
    Specify a GARCH model for estimation by maximum likelihood.

    Creates a GARCH model specification with conditional variance orders
    ``p`` and ``q`` and an optional set of regressors in the mean equation.
    The order ``p`` gives the GARCH (AR) component, while ``q`` gives the
    ARCH (MA) component.

    Parameters
    ----------
    formula : str or list
        Model specification consisting of the GARCH orders ``p`` and ``q``
        followed by a semicolon and a gretl-style variable list, or as a
        nested Python list. The first list specifies the orders ``[p, q]``;
        the second contains the dependent variable and optional regressors.
        A constant is included automatically unless ``nc=True``.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
    robust : bool
        Use QML (White) robust standard errors.
    verbose : bool
        Print details of the numerical iterations.
    quiet : bool
        Suppress printed output.
    vcv : bool
        Print the covariance matrix of the coefficient estimates.
    nc : bool
        Do not include a constant in the mean equation.
    stdresid : bool
        Store standardized rather than raw residuals.
    fcp : bool
        Use the Fiorentini-Calzolari-Panattoni estimation algorithm.
    arma_init : bool
        Initialize the variance parameters using an ARMA model.

    Returns
    -------
    gretl.Model_garch
    A GARCH model instance derived from the abstract ``gretl.Model``
    base class. Call ``fit()`` to estimate the model.

    Notes
    -----
    If ``p`` is positive, ``q`` must also be positive. A standard ARCH
    model can be specified with ``p = 0`` and ``q > 0``. The sum of
    ``p`` and ``q`` must not exceed 5.

    By default, estimation uses gretl's native BFGS-based algorithm and
    the covariance matrix is based on the Hessian. ``robust=True`` requests
    the QML (White) covariance matrix. With ``fcp=True``, the
    Fiorentini-Calzolari-Panattoni algorithm is used instead.

    By default, variance parameters are initialized using an initial OLS
    regression. ``arma_init=True`` obtains starting values from an ARMA
    model, which may improve convergence.

    The residuals and estimated conditional variance are available via
    ``uhat`` and ``h``, respectively. With ``stdresid=True``, residuals
    are standardized by the conditional standard deviation.

    Examples
    --------
    ``gretl.garch("1 1 ; y").fit()``
    ``gretl.garch("1 1 ; y x1 x2", robust=True).fit()``
    ``gretl.garch([[1, 1], ["y", "x1", "x2"]]).fit()``
    )doc";

    inline constexpr char mpols[] = R"doc(
    Specify a linear model for high-precision OLS estimation.

    Creates an OLS model specification for estimation using multiple-precision
    floating-point arithmetic provided by the GNU Multiple Precision (GMP)
    library. By default, calculations use 256 bits of precision.

    Parameters
    ----------
    formula : str or list
        Model specification as a gretl-style variable list or formula,
        or as a list of variable names or IDs. The dependent variable
        comes first; the value 0 denotes the constant term. Optional
        powers of the last regressor may be specified after a semicolon
        or using a nested Python list.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
    vcv : bool
        Print the covariance matrix of the coefficient estimates.
    simple_print : bool
        Suppress printing of auxiliary model statistics.
    quiet : bool
        Suppress printed output.

    Returns
    -------
    gretl.Model_mpols
    A multiple-precision OLS model instance derived from the abstract
    ``gretl.Model`` base class. Call ``fit()`` to estimate the model.

    Notes
    -----
    Calculations use 256 bits of precision by default. The precision can
    be increased by setting the ``GRETL_MP_BITS`` environment variable
    before starting Python.

    An optional list of powers may follow the regressors, separated by
    a semicolon. These powers apply to the last regressor and the
    resulting terms are computed using multiple-precision arithmetic.

    Examples
    --------
    ``gretl.mpols("y 0 x1 x2").fit()``
    ``gretl.mpols(["y", "const", "x1", "x2"]).fit()``
    ``gretl.mpols("y 0 x ; 2 3 4").fit()``
    ``gretl.mpols([["y", 0, "x"], [2, 3, 4]]).fit()``
    )doc";

    inline constexpr char panel[] = R"doc(
    Specify a linear panel-data model for estimation.

    Creates a panel model specification with the specified dependent variable
    and regressors. By default, the fixed-effects estimator is used. Random
    effects, pooled OLS, between-groups and unit-weighted estimators are also
    available.

    Parameters
    ----------
    formula : str or list
        Model specification as a gretl-style variable list or formula,
        or as a list of variable names or IDs. The dependent variable
        comes first; the value 0 denotes the constant term.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
    vcv : bool
        Print the covariance matrix of the coefficient estimates.
    fixed_effects : bool
        Estimate the model using group fixed effects.
    random_effects : bool
        Estimate a random-effects model.
    nerlove : bool
        Use the Nerlove method for random-effects estimation.
    pooled : bool
        Estimate the model using pooled OLS.
    between : bool
        Estimate the between-groups model using group means.
    robust : bool
        Use robust standard errors.
    cluster : str
        Compute clustered standard errors using the specified variable
        to define clusters.
    time_dummies : bool
        Include time dummy variables in the model.
    unit_weights : bool
        Estimate the model by weighted least squares using unit-specific
        residual variances.
    iterate : bool
        Use iterative estimation with unit weights.
    matrix_diff : bool
        Use the matrix-difference form of the Hausman test for a
        random-effects model.
    unbalanced : str
        Select the ANOVA method for an unbalanced random-effects model.
        Supported values are ``"bc"`` and ``"stata"``.
    quiet : bool
        Reduce printed output.
    verbose : bool
        Print additional estimation details.

    Returns
    -------
    gretl.Model_panel
    A panel-data model instance derived from the abstract ``gretl.Model``
    base class. Call ``fit()`` to estimate the model.

    Notes
    -----
    The fixed-effects estimator is used by default and is implemented using
    the within transformation. With ``random_effects=True``, estimation
    uses the Swamy-Arora method by default; ``nerlove=True`` selects the
    Nerlove method instead.

    With ``robust=True``, the default panel-robust covariance estimator is
    the Arellano HAC estimator, clustered by panel unit. Alternative
    estimators include PCSE and Driscoll-Kraay standard errors, selected
    through gretl's ``panel_robust`` setting.

    ``iterate=True`` applies only with ``unit_weights=True``. If iteration
    converges, the resulting estimates are maximum likelihood.
    ``matrix_diff`` and ``unbalanced`` apply only to random-effects models.

    Examples
    --------
    ``gretl.panel("y 0 x1 x2").fit()``
    ``gretl.panel(["y", "const", "x1", "x2"]).fit()``
    ``gretl.panel("y 0 x1 x2", random_effects=True).fit()``
    )doc";

    inline constexpr char tsls[] = R"doc(
    Specify an instrumental-variables model for estimation.

    Creates an IV model specification consisting of a regression equation
    and a set of instruments. By default, estimation uses two-stage least
    squares (TSLS). Limited Information Maximum Likelihood (LIML) and
    Generalized Method of Moments (GMM) estimation are also available.

    Parameters
    ----------
    formula : str or list
        Model specification as two gretl-style variable lists separated
        by a semicolon, or as a nested Python list. The first specifies
        the dependent variable and regressors, while the second specifies
        the instruments. The value 0 denotes the constant term.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
    no_tests : bool
        Suppress diagnostic tests.
    vcv : bool
        Print the covariance matrix of the coefficient estimates.
    quiet : bool
        Suppress printed output.
    no_df_corr : bool
        Suppress the degrees-of-freedom correction.
    robust : bool
        Use heteroskedasticity-robust standard errors.
    cluster : str
        Compute clustered standard errors using the specified variable.
    matrix_diff : bool
        Use the matrix-difference method for the Hausman test.
    liml : bool
        Estimate the model by Limited Information Maximum Likelihood.
    gmm : bool
        Estimate the model by the Generalized Method of Moments.
    two_step : bool
        Use two-step rather than one-step GMM.
    iterate : bool
        Iterate GMM estimation to convergence.
    weights : str
        Name of the weighting matrix to use for GMM estimation.

    Returns
    -------
    gretl.Model_tsls
    An instrumental-variables model instance derived from the
    abstract ``gretl.Model`` base class. Call ``fit()`` to
    estimate the model.

    Notes
    -----
    The instrument list must contain at least as many variables as
    the regressor list for the model to be identified. Exogenous
    regressors should also appear in the instrument list.

    TSLS diagnostic output includes a Hausman test and, for
    overidentified models, the Sargan test of overidentifying
    restrictions. Under i.i.d. errors, a weak-instruments test
    is also reported for TSLS and LIML estimation.

    The Hausman and Sargan statistics are available through the
    ``hausman`` and ``sargan`` accessors, respectively, when applicable.

    The ``two_step``, ``iterate`` and ``weights`` options apply only
    to GMM estimation. The default GMM weighting matrix is the
    identity matrix.

    For just-identified models, TSLS, LIML and GMM yield equivalent
    estimates; for overidentified models they generally differ.

    Examples
    --------
    ``gretl.tsls("y 0 x1 x2 ; 0 x1 z1 z2").fit()``
    ``gretl.tsls([["y", 0, "x1", "x2"], [0, "x1", "z1", "z2"]]).fit()``
    ``gretl.tsls("y 0 x1 x2 ; 0 x1 z1 z2", liml=True).fit()``
    )doc";

    inline constexpr char hsk[] = R"doc(
    Specify a heteroskedasticity-corrected linear regression model.

    Creates a model specification for feasible weighted least squares when
    the error variance is an unknown function of the regressors. The
    procedure estimates the conditional error variance from an auxiliary
    regression and uses its reciprocal as the weight in the final regression.

    Parameters
    ----------
    formula : str or list
        Model specification as a gretl-style variable list or formula,
        or as a list of variable names or IDs. The dependent variable
        comes first; the value 0 denotes the constant term.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
    no_squares : bool
        Exclude squared regressors from the auxiliary variance regression.
    vcv : bool
        Print the covariance matrix of the coefficient estimates.
    quiet : bool
        Suppress printed output.

    Returns
    -------
    gretl.Model_hsk
    A heteroskedasticity-corrected model instance derived from the
    abstract ``gretl.Model`` base class. Call ``fit()`` to estimate
    the model.

    Notes
    -----
    Estimation proceeds in three stages. First, the model is estimated
    by OLS. Second, the logarithm of the squared OLS residuals is
    regressed on the original regressors and, by default, their squares.
    Finally, the model is estimated by weighted least squares using
    the reciprocal of the estimated error variance as the weight.

    The weights are calculated as ``1 / exp(u_star)``, where ``u_star``
    denotes the fitted values from the auxiliary regression. This
    transformation ensures positive estimated variances.

    With ``no_squares=True``, only the original regressors are included
    in the auxiliary variance regression.

    Examples
    --------
    ``gretl.hsk("y 0 x1 x2").fit()``
    ``gretl.hsk(["y", "const", "x1", "x2"]).fit()``
    ``gretl.hsk("y 0 x1 x2", no_squares=True).fit()``
    )doc";

    inline constexpr char intreg[] = R"doc(
    Specify an interval regression model for estimation by maximum likelihood.

    Creates an interval regression model specification for a dependent
    variable observed within an interval rather than necessarily as an
    exact value. The lower and upper bounds are specified by separate
    variables.

    Parameters
    ----------
    formula : str or list
        Model specification as a gretl-style variable list, or as a list
        of variable names or IDs. The first two variables specify the
        lower and upper bounds of the dependent variable, followed by
        the regressors. The value 0 denotes the constant term.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
    quiet : bool
        Suppress printed output.
    verbose : bool
        Print details of the numerical iterations.
    robust : bool
        Use QML (Huber-White) robust standard errors.
    opg : bool
        Compute the covariance matrix using the Outer Product of the
        Gradient (OPG).
    cluster : str
        Compute clustered standard errors using the specified variable.

    Returns
    -------
    gretl.Model_intreg
    An interval regression model instance derived from the abstract
    ``gretl.Model`` base class. Call ``fit()`` to estimate the model.

    Notes
    -----
    The model assumes a normally distributed disturbance term and is
    estimated by maximum likelihood. Exact observations are represented
    by equal lower and upper bounds. Missing lower or upper bounds
    indicate left- or right-unbounded intervals, respectively.

    By default, standard errors are based on the negative inverse of the
    Hessian. ``robust=True`` computes a QML sandwich covariance matrix,
    while ``opg=True`` uses the Outer Product of the Gradient alone.

    Examples
    --------
    ``gretl.intreg("ymin ymax 0 x1 x2").fit()``
    ``gretl.intreg(["ymin", "ymax", 0, "x1", "x2"]).fit()``
    ``gretl.intreg("ymin ymax 0 x1 x2", robust=True).fit()``
    )doc";

    inline constexpr char midasreg[] = R"doc(
    Specify a MIDAS regression model for least-squares estimation.

    Creates a Mixed Data Sampling (MIDAS) model combining regressors
    observed at the frequency of the dependent variable with one or
    more higher-frequency regressors. Depending on the parameterization,
    estimation uses OLS or nonlinear least squares (NLS).

    Parameters
    ----------
    formula : str
        Model specification consisting of the dependent variable and
        same-frequency regressors, followed by a semicolon and one or
        more high-frequency terms specified using ``mds()`` or ``mdsl()``.
        The value 0 denotes the constant term.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
    vcv : bool
        Print the covariance matrix of the coefficient estimates.
    robust : bool
        Use robust standard errors.
    quiet : bool
        Suppress printed output.
    levenberg : bool
        Force use of the Levenberg-Marquardt algorithm for NLS estimation.

    Returns
    -------
    gretl.Model_midasreg
    A MIDAS regression model instance derived from the abstract
    ``gretl.Model`` base class. Call ``fit()`` to estimate the model.

    Notes
    -----
    Same-frequency regressors appear before the semicolon, while
    higher-frequency terms are specified using ``mds()`` or ``mdsl()``.
    The former specifies a MIDAS list and a range of lags; the latter
    uses a list containing the required lags.

    Supported parameterizations are ``umidas`` (0), ``nealmon`` (1),
    ``beta0`` (2), ``betan`` (3), ``almonp`` (4), and ``beta1`` (5).
    U-MIDAS is estimated by OLS; other specifications use NLS.

    Nonlinear specifications require initial parameter values, which
    may be supplied as a vector, requested automatically using ``null``
    where supported, or specified by the number of parameters.

    For normalized exponential Almon and beta specifications, the
    default estimator combines constrained BFGS and OLS.
    ``levenberg=True`` selects the Levenberg-Marquardt algorithm.

    Examples
    --------
    ``gretl.midasreg("y 0 y(-1) ; mds(X, 1, 9, 1, theta)").fit()``
    ``gretl.midasreg("y 0 y(-1) ; mds(X, 1, 9, 0)").fit()``
    ``gretl.midasreg("y 0 y(-1) ; mdsl(XL, 2, theta)").fit()``
    )doc";

    inline constexpr char arima[] = R"doc(
    Specify an ARIMA model for estimation by maximum likelihood.

    Creates an Autoregressive Integrated Moving Average model specification.
    The non-seasonal AR, differencing and MA orders are given by ``order``;
    optional seasonal orders are given by ``seasonal``. Exogenous regressors
    may be included to specify an ARIMAX model.

    Parameters
    ----------
    y : str
        Name of the dependent variable.
    order : tuple
        Non-seasonal ARIMA order ``(p, d, q)``, where ``p`` is the
        autoregressive order, ``d`` is the differencing order, and ``q`` is
        the moving-average order.
    seasonal : tuple, optional
        Seasonal ARIMA order ``(P, D, Q)``, where ``P`` is the seasonal
        autoregressive order, ``D`` is the seasonal differencing order, and
        ``Q`` is the seasonal moving-average order.
    x : str or list, optional
        Exogenous regressors for an ARIMAX model.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
    verbose : bool
        Print details of the numerical iterations.
    quiet : bool
        Suppress printed output.
    vcv : bool
        Print the covariance matrix of the coefficient estimates.
    hessian : bool
        Require use of the numerical Hessian for the covariance matrix,
        disabling fallback to OPG.
    opg : bool
        Use the Outer Product of the Gradient (OPG) for the covariance
        matrix.
    nc : bool
        Do not include a constant.
    conditional : bool
        Use conditional rather than exact maximum likelihood.
        x_12_arima : bool
        Use X-12-ARIMA or X-13ARIMA-SEATS for estimation.
    lbfgs : bool
        Use the L-BFGS-B maximizer instead of BFGS.
    y_diff_only : bool
        For ARIMAX models, difference only the dependent variable, leaving
        the exogenous regressors in levels.
    lagselect : bool
        Perform lag-order selection instead of estimating a single model.

    Returns
    -------
    gretl.Model_arima
        An ARIMA model instance derived from the abstract ``gretl.Model``
        base class. Call ``fit()`` to estimate the model.

    Notes
    -----
    By default, gretl's native exact maximum-likelihood estimator is used.
    ``conditional=True`` selects conditional maximum likelihood.

    With native exact ML, the covariance matrix is based on a numerical
    approximation to the negative inverse Hessian, with fallback to OPG if
    the Hessian cannot be computed. ``opg=True`` forces use of OPG, while
    ``hessian=True`` disables the fallback.

    In an ARIMAX model with non-zero differencing, both the dependent
    variable and exogenous regressors are differenced by default.
    ``y_diff_only=True`` leaves the regressors in levels.

    With ``lagselect=True``, the AR and MA orders are interpreted as maximum
    orders and a table of information criteria and log-likelihood values is
    produced instead of estimates for a single specification.

    Examples
    --------
    ``gretl.arima(y="y", order=(1, 1, 1)).fit()``
    ``gretl.arima(y="y", order=(1, 1, 1), seasonal=(1, 1, 1)).fit()``
    )doc";

    inline constexpr char quantreg[] = R"doc(
    Specify a quantile regression model.

    Estimates one or more conditional quantiles of the dependent variable.
    Call ``fit()`` to estimate the model.

    Parameters
    ----------
    tau : float or numpy.ndarray
        Conditional quantile or sequence of quantiles to estimate.
        Each value must lie in the range 0.01 to 0.99.
    formula : str or list
        Model specification as a gretl-style variable list or formula,
        or a list of variable names or IDs. The value 0 denotes the
        constant term.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
    robust : bool
        Use heteroskedasticity-robust standard errors or confidence intervals.
    intervals : bool or float
        Compute confidence intervals instead of standard errors.
        ``True`` selects the default 90% level; a float specifies
        another confidence level, e.g. ``0.95``.
    vcv : bool
        Print the covariance matrix.
    quiet : bool
        Suppress printed output.

    Returns
    -------
    gretl.Model_quantreg
    Quantile regression model instance. Call ``fit()`` to estimate.

    Notes
    -----
    Standard errors are based on Koenker and Bassett (1978), or on
    Koenker and Zhao (1994) when ``robust=True``.

    Confidence intervals are computed by rank inversion and may be
    asymmetric. With ``robust=True``, the method of Koenker and
    Machado (1999) is used.

    For multiple quantiles, ``coeff`` and ``stderr`` are matrices
    with one column per quantile. The ``rq_sequence`` entry in the
    model results provides the combined coefficient and standard
    error output.

    Examples
    --------
    >>> m = gretl.quantreg(0.5, "y const x1 x2").fit()
    >>> m = gretl.quantreg(0.25, ["y", "const", "x1", "x2"],
    ...                   robust=True).fit()
    >>> m = gretl.quantreg([0.25, 0.5, 0.75],
    ...                   "y const x1 x2", intervals=0.95).fit()
    )doc";

    inline constexpr char dpanel[] = R"doc(
    Specify a dynamic panel-data model for GMM estimation.

    Creates a dynamic panel model containing one or more lags of the
    dependent variable. Estimation uses either difference GMM (GMM-DIF)
    or, with ``system=True``, system GMM (GMM-SYS).

    Parameters
    ----------
    p : int or list
        Autoregressive lag order or a list of dependent-variable lags to
        include. A list may be used to specify non-contiguous lags.
    formula : str
        Model specification as a gretl-style variable list. The dependent
        variable and regressors are specified in levels. An optional
        instrument specification follows a semicolon and may include
        ``GMM()`` and ``GMMlevel()`` terms.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
    quiet : bool
        Suppress printed output.
    vcv : bool
        Print the covariance matrix of the coefficient estimates.
    two_step : bool
        Perform two-step rather than one-step GMM estimation.
    system : bool
        Use system GMM by adding equations in levels.
    collapse : bool
        Collapse GMM-style instruments to limit instrument proliferation.
    time_dummies : bool
        Add time dummy variables to the model.
    dpdstyle : bool
        Use the DPD convention for handling time dummy variables.
    asymptotic : bool
        Use uncorrected asymptotic standard errors for two-step estimation
        instead of the Windmeijer finite-sample correction.
    keep_extra : bool
        Retain additional GMM information, including the weight and
        instrument matrices, in the model results.

    Returns
    -------
    gretl.Model_dpanel
        A dynamic panel model instance derived from the abstract
        ``gretl.Model`` base class. Call ``fit()`` to estimate the model.

    Notes
    -----
    The dependent variable and regressors should be specified in levels;
    the transformations required for difference GMM are performed
    automatically.

    If no instrument specification is supplied, all independent variables
    are treated as strictly exogenous. If instruments are specified,
    strictly exogenous regressors should also be included explicitly in the
    instrument list. ``GMM()`` specifies lagged GMM-style instruments for
    differenced equations, while ``GMMlevel()`` specifies such instruments
    for equations in levels.

    One-step GMM with robust standard errors is used by default. For
    two-step estimation, the Windmeijer finite-sample correction is applied
    to the standard errors unless ``asymptotic=True``.

    Tests for first- and second-order residual autocorrelation and tests of
    overidentifying restrictions are reported as applicable.

    Examples
    --------
    ``gretl.dpanel(p=2, formula="y x1 x2 ; GMM(x1,2,3) x1 GMM(x2,2,3)").fit()``

    ``gretl.dpanel(p=[2, 3], formula="y x1 x2 ; GMM(x1,2,3) x1 GMM(x2,2,3)", two_step=True).fit()``
    )doc";

    inline constexpr char tobit[] = R"doc(
    Specify a Tobit model for estimation by maximum likelihood.

    Creates a censored regression model specification with the specified
    dependent variable and regressors. By default, the dependent variable
    is assumed to be left-censored at zero and uncensored on the right.

    Parameters
    ----------
    formula : str or list
        Model specification as a gretl-style variable list or formula,
        or as a list of variable names or IDs. The dependent variable
        comes first; the value 0 denotes the constant term.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
    llimit : float
        Specify the left censoring bound.
    rlimit : float
        Specify the right censoring bound.
    vcv : bool
        Print the covariance matrix of the coefficient estimates.
    robust : bool
        Use QML (Huber-White) robust standard errors.
    opg : bool
        Compute the covariance matrix using the Outer Product of the
        Gradient (OPG).
    cluster : str
        Compute clustered standard errors using the specified variable to
        define clusters.
    verbose : bool
        Print details of the numerical iterations.
    quiet : bool
        Suppress printed output.

    Returns
    -------
    gretl.Model_tobit
        A Tobit model instance derived from the abstract ``gretl.Model``
        base class. Call ``fit()`` to estimate the model.

    Notes
    -----
    By default, censoring is assumed at zero on the left, with no right
    censoring. ``llimit`` and ``rlimit`` may be used to specify alternative
    censoring bounds. If only ``rlimit`` is specified, the dependent
    variable is treated as uncensored on the left.

    By default, standard errors are based on the negative inverse of the
    Hessian. ``robust=True`` computes a QML sandwich covariance matrix,
    while ``opg=True`` uses the Outer Product of the Gradient alone.

    The Tobit model is a special case of interval regression.

    Examples
    --------
    ``gretl.tobit("y const x1 x2").fit()``
    ``gretl.tobit(["y", "const", "x1", "x2"], llimit=0).fit()``
    )doc";

    inline constexpr char var[] = R"doc(
    Specify a vector autoregression (VAR) model.

    Creates a VAR specification for a set of endogenous variables. Lags of
    the endogenous variables are generated automatically according to the
    specified order. Optional exogenous variables may be included via
    ``xlist``.

    Parameters
    ----------
    order : int
        Lag order of the VAR. With ``lagselect=True``, this specifies the
        maximum lag order to consider.
    ylist : str or list
        Endogenous variables of the VAR. Do not include their lags explicitly;
        they are generated automatically.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
    xlist : str or list
        Exogenous variables to include in each equation.
    nc : bool
        Do not include a constant.
    trend : bool
        Include a linear trend.
    seasonals : bool
        Include seasonal dummy variables.
    robust : bool
        Use heteroskedasticity-robust standard errors.
    robust_hac : bool
        Use standard errors robust to heteroskedasticity and autocorrelation.
    quiet : bool
        Suppress output for the individual VAR equations.
    silent : bool
        Suppress all printed output.
    impulse_responses : bool
        Print impulse-response functions.
    variance_decomp : bool
        Print forecast-error variance decompositions.
    lagselect : bool
        Perform lag-order selection instead of estimating a single VAR
        specification.
    minlag : int
        Minimum lag order to consider when ``lagselect=True``.

    Returns
    -------
    gretl.Model_var
        A VAR model instance derived from the abstract ``gretl.Model``
        base class. Call ``fit()`` to estimate the model.

    Notes
    -----
    A constant is included by default. The stochastic variables in ``ylist``
    are treated as endogenous, while variables in ``xlist`` enter each
    equation as exogenous regressors.

    Impulse responses and forecast-error variance decompositions are based
    on a Cholesky decomposition of the contemporaneous covariance matrix.
    Their results therefore depend on the ordering of variables in
    ``ylist``.

    With ``lagselect=True``, ``order`` is interpreted as the maximum lag
    order. Lag specifications are compared using likelihood-ratio tests and
    the AIC, BIC and HQC information criteria. ``minlag`` can be used to
    change the minimum order considered.

    Examples
    --------
    ``gretl.var(order=2, ylist="y1 y2").fit()``

    ``gretl.var(
        order=2, ylist=["y1", "y2"],
        xlist=["x1", "x2"], trend=True, seasonals=True
    ).fit()``
    )doc";

    inline constexpr char vecm[] = R"doc(
    Specify a vector error-correction model (VECM).

    Creates a VECM specification for a set of cointegrated I(1) endogenous
    variables. The VAR lag order and cointegration rank are specified
    explicitly, with optional unrestricted and restricted exogenous variables.

    Parameters
    ----------
    order : int
        Lag order of the underlying VAR system. The corresponding VECM
        contains ``order - 1`` lags of the differenced endogenous variables.
    rank : int
        Cointegration rank, that is, the number of cointegrating vectors.
    ylist : str or list
        Endogenous variables, specified in levels.

    Other Parameters
    ----------------
    data : gretl.Dataset, optional
        Dataset to use for model estimation. If omitted, the currently
        default dataset is used.
    xlist : str or list
        Exogenous variables that enter the model unrestrictedly.
    rxlist : str or list
        Exogenous variables restricted to the cointegration space.
    nc : bool
        Include no constant.
    rc : bool
        Include a constant restricted to the cointegration space.
    uc : bool
        Include an unrestricted constant.
    crt : bool
        Include a constant and a trend restricted to the cointegration
        space.
    ct : bool
        Include a constant and an unrestricted trend.
    seasonals : bool
        Include centered seasonal dummy variables.
    quiet : bool
        Suppress output for the individual VECM equations.
    silent : bool
        Suppress all printed output.
    impulse_responses : bool
        Print impulse-response functions.
    variance_decomp : bool
        Print forecast-error variance decompositions.

    Returns
    -------
    gretl.Model_vecm
        A VECM model instance derived from ``gretl.Model_var``. Call
        ``fit()`` to estimate the model.

    Notes
    -----
    The variables in ``ylist`` should be I(1) and are specified in levels.
    The cointegration rank must be positive and cannot exceed the number
    of endogenous variables.

    By default, an unrestricted constant is included. The options ``nc``,
    ``rc``, ``uc``, ``crt`` and ``ct`` select alternative deterministic
    specifications and are mutually exclusive.

    Variables in ``xlist`` enter the model unrestrictedly, while variables
    in ``rxlist`` are restricted to the cointegration space. Centered
    seasonal dummies can be included for quarterly or monthly data using
    ``seasonals=True``.

    Following estimation, the Johansen alpha and beta matrices and the
    estimated covariance matrix of beta are available through the
    corresponding model results.

    Examples
    --------
    ``gretl.vecm(order=2, rank=1, ylist="y1 y2").fit()``

    ``gretl.vecm(order=2, rank=1, ylist=["y1", "y2"], xlist=["x1", "x2"], rc=True, seasonals=True).fit()``
    )doc";

    inline constexpr char system[] = R"doc(
    Specify a system of equations for joint estimation.

    Creates a multiple-equation model specification using one of the
    supported system estimators. Equations, endogenous variables,
    instruments and identities can be supplied as keyword arguments.

    Parameters
    ----------
    method : str
        Estimation method. Supported values are ``"ols"``, ``"tsls"``,
        ``"sur"``, ``"3sls"``, ``"fiml"`` and ``"liml"``.

    Other Parameters
    ----------------
    equations : list
        System equations, specified as gretl-style variable lists. Each
        element defines one equation.
    endogs : str or list
        Endogenous variables in a simultaneous-equations system.
    instruments : str or list
        Instruments to use in instrumental-variable system estimation.
    identities : list
        Identities linking variables in the system. These are used for FIML
        estimation and ignored by estimators for which they are not
        applicable.
    verbose : bool
        Print additional details of the estimation procedure.
    quiet : bool
        Suppress printed output.

    Returns
    -------
    gretl.System
        A system-of-equations model instance derived from the abstract
        ``gretl.Model`` base class. Call ``fit()`` to estimate the system.

    Notes
    -----
    ``"ols"`` estimates the equations separately by ordinary least squares.
    ``"tsls"`` uses two-stage least squares, ``"sur"`` uses seemingly
    unrelated regressions, and ``"3sls"`` uses three-stage least squares.
    ``"fiml"`` and ``"liml"`` select full-information and limited-information
    maximum likelihood, respectively.

    For simultaneous-equation methods, ``endogs`` identifies the endogenous
    variables in the system. Instruments may be supplied explicitly where
    required. Identities are primarily relevant to FIML estimation.

    Following estimation, system results include equation residuals and
    fitted values, the stacked coefficient vector and its covariance matrix,
    and the cross-equation residual covariance matrix. Structural-form
    matrices are also available where applicable.

    Cross-equation restrictions can be imposed on the fitted model using
    ``restrict()``.

    Examples
    --------
    ``gretl.system(
        method="sur",
        equations=[
            "y1 const x1 x2",
            "y2 const x1 x3"
        ]
    ).fit()``

    ``gretl.system(
        method="3sls",
        equations=[
            "y1 const x1 x2",
            "y2 const x1 x3"
        ],
        endogs="y1 y2"
    ).fit()``
    )doc";

    inline constexpr char save_system_uhat[] = R"doc(
    Add residuals from a specified equation to the dataset.

    Parameters
    ----------
    eqn : int
        1-based equation number.
    name : str, optional
        Name of the generated series. If omitted, the name follows the
        pattern ``m_ID_uhat``, where ``ID`` is the equation's ID.

    Returns
    -------
    str
        Name of the series added to the dataset.
    )doc";

} // namespace doc::Model

namespace doc::Model_VAR {

    inline constexpr char irf[] = R"doc(
    Compute impulse response functions for the VAR or VECM.

    Parameters
    ----------
    target : int or str
        Target variable, specified by name or 1-based index. Zero denotes
        all target variables.
    shock : int or str
        Shock variable, specified by name or 1-based index. Zero denotes
        all shock variables.
    alpha : float, optional
        Significance level for bootstrap confidence intervals. If zero,
        only point estimates are computed.

    Returns
    -------
    gretl.Matrix
        Impulse response estimates. If ``alpha`` is positive, the matrix
        also contains lower and upper confidence bounds.

    Notes
    -----
    Responses are expressed in the units of the target variable and
    correspond to a one-standard-deviation innovation in the shock
    variable.
    )doc";

    inline constexpr char save_ec_term[] = R"doc(
    Add an error-correction term to the dataset.

    Parameters
    ----------
    nr : int
        1-based number of the error-correction term to retrieve.
    name : str, optional
        Name of the generated series. If omitted, a name is generated
        from the model identifier and error-correction term number.

    Returns
    -------
    str
        Name of the series added to the dataset.
    )doc";

    inline constexpr char set_cholesky_order[] = R"doc(
    Set the Cholesky ordering of endogenous variables in a VAR or VECM.

    The ordering is used to identify orthogonalized shocks in subsequent
    impulse-response and forecast-error variance decomposition calculations.
    It does not change the estimated VAR/VECM equations.

    Parameters
    ----------
    varlist : str or list, optional
        A complete permutation of the endogenous variables, specified as
        a space-separated string or a Python list of variable names or
        dataset variable IDs. An empty string or list restores the default
        ordering from the original model specification.

    Returns
    -------
    None

    Notes
    -----
    The selected ordering is retained by the model until changed or reset.
    Integer values refer to dataset variable IDs, not zero-based equation
    indices.

    Examples
    --------
    ``m.set_cholesky_order("IBO LRM IDE LRY")``
    ``m.set_cholesky_order(["IBO", "LRM", "IDE", "LRY"])``
    ``m.set_cholesky_order([3, 1, 4, 2])``
    ``m.set_cholesky_order()``
    )doc";

} // namespace doc::Model_VAR
