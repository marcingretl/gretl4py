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

namespace doc::Functions {

    inline constexpr char pvalue[] = R"doc(
    Compute the right-tail probability for a probability distribution.

    Returns the probability ``P(X > x)`` for the specified distribution.
    The calculation can be applied to a scalar, a gretl matrix, a data
    series, or a Python sequence.

    Parameters
    ----------
    c : str
        Distribution identifier. Supported distributions include ``"z"``
        (standard normal), ``"t"`` (Student's t), ``"X"`` (chi-square),
        ``"F"`` (F), ``"G"`` (gamma), ``"B"`` (binomial), ``"P"``
        (Poisson), ``"exp"`` (exponential), ``"W"`` (Weibull), as well as
        the Laplace and generalized error distributions.
    x : float, gretl.Matrix, str, or sequence
        Value or values at which to evaluate the right-tail probability.
        A string specifies the name of a series in the current dataset.
    params : optional
        Distribution-specific parameter or parameters. The required
        parameters depend on ``c``.

    Returns
    -------
    float or gretl.Matrix
        Right-tail probability ``P(X > x)``. A scalar ``x`` returns a
        scalar; matrix, series and sequence inputs return a ``gretl.Matrix``
        of the same shape.

    Notes
    -----
    The distribution-specific parameters are: degrees of freedom for
    Student's t and chi-square; numerator and denominator degrees of freedom
    for F; shape and scale for gamma; success probability and number of
    trials for binomial; lambda for Poisson; scale for exponential; and
    shape and scale for Weibull. The standard normal distribution requires
    no additional parameters.

    For the gamma distribution, if the distribution is described in terms
    of mean and variance, the scale is ``variance / mean`` and the shape is
    ``mean / scale``.

    Matrix and sequence inputs must contain real-valued elements.

    Examples
    --------
    ``gretl.pvalue("z", 2.2)``
    ``gretl.pvalue("t", 3.0, 25)``
    ``gretl.pvalue("F", 5.67, [3, 30])``
    ``gretl.pvalue("X", [2.0, 4.0, 6.0], 3)``
    )doc";

    inline constexpr char critical[] = R"doc(
    Compute a critical value for a probability distribution.

    Returns the value ``x`` such that ``P(X > x) = p`` for the specified
    distribution. The calculation can be applied to a scalar, a gretl
    matrix, a data series, or a Python sequence.

    Parameters
    ----------
    c : str
        Distribution identifier. Supported distributions are ``"z"``,
        ``"n"`` or ``"N"`` (standard normal), ``"t"`` (Student's t),
        ``"c"``, ``"x"`` or ``"X"`` (chi-square), ``"f"`` or ``"F"``
        (F), ``"b"`` or ``"B"`` (binomial), ``"p"`` or ``"P"``
        (Poisson), ``"l"`` or ``"L"`` (Laplace), and ``"E"``
        (standardized GED).
    p : float, gretl.Matrix, str, or sequence
        Right-tail probability or probabilities. A string specifies the
        name of a series in the current dataset.
    params : optional
        Distribution-specific parameter or parameters. The required
        parameters depend on ``c``.

    Returns
    -------
    float or gretl.Matrix
        Critical value ``x`` satisfying ``P(X > x) = p``. A scalar ``p``
        returns a scalar; matrix, series and sequence inputs return a
        ``gretl.Matrix`` of the same shape.

    Notes
    -----
    The distribution-specific parameters are: degrees of freedom for
    Student's t and chi-square; numerator and denominator degrees of freedom
    for F; success probability and number of trials for binomial; mean for
    Poisson; mean and scale for Laplace; and shape for the standardized GED.
    The standard normal distribution requires no additional parameters.

    Matrix and sequence inputs must contain real-valued elements.

    Examples
    --------
    ``gretl.critical("z", 0.025)``
    ``gretl.critical("t", 0.025, 20)``
    ``gretl.critical("F", 0.05, [4, 48])``
    ``gretl.critical("X", [0.10, 0.05, 0.01], 3)``
    )doc";

    inline constexpr char pdf[] = R"doc(
    Evaluate a probability density or mass function.

    Returns the probability density at ``x`` for the specified distribution.
    For discrete distributions, the returned value is the probability mass
    at ``x``. The calculation can be applied to a scalar, a gretl matrix,
    a data series, or a Python sequence.

    Parameters
    ----------
    c : str
        Distribution identifier. Supported distributions include normal,
        Student's t, chi-square, F, gamma, beta, exponential, Weibull,
        Laplace, generalized error, binomial and Poisson distributions.
        Noncentral variants of Student's t, chi-square and F are also
        supported.
    x : float, gretl.Matrix, str, or sequence
        Value or values at which to evaluate the density or probability
        mass. A string specifies the name of a series in the current dataset.
    params : optional
        Distribution-specific parameter or parameters. The required
        parameters depend on ``c``.

    Returns
    -------
    float or gretl.Matrix
        Probability density or mass at ``x``. A scalar ``x`` returns a
        scalar; matrix, series and sequence inputs return a ``gretl.Matrix``
        of the same shape.

    Notes
    -----
    For continuous distributions, this function evaluates the probability
    density function. For the binomial and Poisson distributions, it
    evaluates the probability mass function.

    The parameters required for a given distribution follow the standard
    gretl distribution parameterization. Noncentral Student's t, chi-square
    and F distributions require an additional noncentrality parameter.

    Matrix and sequence inputs must contain real-valued elements.

    Examples
    --------
    ``gretl.pdf("N", -2.5)``
    ``gretl.pdf("X", [2.0, 4.0, 6.0], 3)``
    ``gretl.pdf("W", 2.5, [1.5, 2.0])``
    )doc";

    inline constexpr char cdf[] = R"doc(
    Evaluate a cumulative distribution function.

    Returns the probability ``P(X <= x)`` for the specified distribution.
    The calculation can be applied to a scalar, a gretl matrix, a data
    series, or a Python sequence.

    Parameters
    ----------
    c : str
        Distribution identifier. Supported distributions include normal,
        truncated normal, logistic, Student's t, chi-square, F, gamma,
        beta, binomial, Poisson, exponential, Weibull, Laplace and
        generalized error distributions, including noncentral variants
        of Student's t, chi-square and F.
    x : float, gretl.Matrix, str, or sequence
        Value or values at which to evaluate the cumulative probability.
        A string specifies the name of a series in the current dataset.
    params : optional
        Distribution-specific parameter or parameters. The required
        parameters depend on ``c``.

    Returns
    -------
    float or gretl.Matrix
        Cumulative probability ``P(X <= x)``. A scalar ``x`` returns a
        scalar; matrix, series and sequence inputs return a ``gretl.Matrix``
        of the same shape.

    Notes
    -----
    The supported distributions and their parameters are:

        - ``"z"``, ``"n"``, ``"N"``: standard normal; no parameters.
        - ``"tn"``: truncated normal; left and right truncation points.
        - ``"lgt"``, ``"s"``: logistic; no parameters.
        - ``"t"``: Student's t; degrees of freedom.
        - ``"c"``, ``"x"``, ``"X"``: chi-square; degrees of freedom.
        - ``"f"``, ``"F"``: F; numerator and denominator degrees of freedom.
        - ``"g"``, ``"G"``: gamma; shape and scale.
        - ``"beta"``: beta; two shape parameters.
        - ``"b"``, ``"B"``: binomial; success probability and number of trials.
        - ``"p"``, ``"P"``: Poisson; mean.
        - ``"exp"``: exponential; scale.
        - ``"w"``, ``"W"``: Weibull; shape and scale.
        - ``"l"``, ``"L"``: Laplace; mean and scale.
        - ``"E"``: generalized error; shape.
        - ``"ncX"``: noncentral chi-square; degrees of freedom and
        noncentrality parameter.
        - ``"ncF"``: noncentral F; numerator and denominator degrees of freedom
        and noncentrality parameter.
        - ``"nct"``: noncentral Student's t; degrees of freedom and
        noncentrality parameter.

    For the truncated normal distribution, ``None`` may be used for an
    infinite left or right truncation point if supported by the parameter
    conversion interface.

    Matrix and sequence inputs must contain real-valued elements.

    Examples
    --------
    ``gretl.cdf("N", -2.5)``
    ``gretl.cdf("X", 5.67, 3)``
    ``gretl.cdf("F", [2.0, 3.0, 4.0], [3, 30])``
    )doc";

    inline constexpr char invcdf[] = R"doc(
    Evaluate an inverse cumulative distribution function.

    For a continuous distribution, returns ``x`` such that
    ``P(X <= x) = u``. For a discrete distribution, returns the smallest
    ``x`` such that ``P(X <= x) >= u``. The calculation can be applied to
    a scalar, a gretl matrix, a data series, or a Python sequence.

    Parameters
    ----------
    c : str
        Distribution identifier. Supported distributions include standard
        normal, gamma, Student's t, chi-square, F, binomial, Poisson,
        Laplace and standardized generalized error distributions, including
        noncentral variants of Student's t, chi-square and F.
    u : float, gretl.Matrix, str, or sequence
        Cumulative probability or probabilities, in the interval from 0
        to 1. A string specifies the name of a series in the current dataset.
    params : optional
        Distribution-specific parameter or parameters. The required
        parameters depend on ``c``.

    Returns
    -------
    float or gretl.Matrix
        Quantile corresponding to ``u``. A scalar ``u`` returns a scalar;
        matrix, series and sequence inputs return a ``gretl.Matrix`` of the
        same shape.

    Notes
    -----
    The supported distributions and their parameters are:

        - ``"z"``, ``"n"``, ``"N"``: standard normal; no parameters.
        - ``"g"``, ``"G"``: gamma; shape and scale.
        - ``"t"``: Student's t; degrees of freedom.
        - ``"c"``, ``"x"``, ``"X"``: chi-square; degrees of freedom.
        - ``"f"``, ``"F"``: F; numerator and denominator degrees of freedom.
        - ``"b"``, ``"B"``: binomial; success probability and number of trials.
        - ``"p"``, ``"P"``: Poisson; mean.
        - ``"l"``, ``"L"``: Laplace; mean and scale.
        - ``"E"``: standardized generalized error; shape.
        - ``"ncX"``: noncentral chi-square; degrees of freedom and
        noncentrality parameter.
        - ``"ncF"``: noncentral F; numerator and denominator degrees of freedom
        and noncentrality parameter.
        - ``"nct"``: noncentral Student's t; degrees of freedom and
        noncentrality parameter.

    For the binomial and Poisson distributions, the result is the smallest
    value whose cumulative probability is at least ``u``.

    Matrix and sequence inputs must contain real-valued elements.

    Examples
    --------
    ``gretl.invcdf("N", 0.975)``
    ``gretl.invcdf("t", 0.975, 20)``
    ``gretl.invcdf("F", 0.95, [4, 48])``
    ``gretl.invcdf("X", [0.90, 0.95, 0.99], 3)``
    )doc";

    inline constexpr char cov[] = R"doc(
    Compute a covariance matrix.

    Computes the covariance matrix for the columns of a two-dimensional
    array or matrix. Each column is treated as a variable and each row as
    an observation.

    Parameters
    ----------
    X : numpy.ndarray or gretl.Matrix
        Data containing at least two columns.
    dfcorr : int, optional
        Degrees-of-freedom correction used in computing the covariance
        matrix. The default is 1.

    Returns
    -------
    gretl.Matrix
        Covariance matrix. If ``X`` has ``k`` columns, the result is a
        ``k`` by ``k`` matrix.

    Examples
    --------
    ``gretl.cov(X)``
    ``gretl.cov(X, dfcorr=0)``
    )doc";

    inline constexpr char corr[] = R"doc(
    Compute a Pearson correlation matrix.

    Computes the pairwise Pearson product-moment correlation coefficients
    for the columns of a two-dimensional array or matrix. Each column is
    treated as a variable and each row as an observation.

    Parameters
    ----------
    X : numpy.ndarray or gretl.Matrix
        Data containing at least two columns.

    Returns
    -------
    gretl.Matrix
        Correlation matrix. If ``X`` has ``k`` columns, the result is a
        ``k`` by ``k`` symmetric matrix with unit diagonal.

    Notes
    -----
    This function computes Pearson product-moment correlations. Spearman's
    rho and Kendall's tau are not supported by this interface.

    Examples
    --------
    ``gretl.corr(X)``
    )doc";

    inline constexpr char kmeans[] = R"doc(
    Perform k-means clustering.

    Applies the Hartigan-Wong k-means algorithm (ASA136) to partition
    observations into clusters while minimizing within-cluster variation.
    Initial centroids may be selected automatically or supplied explicitly.

    Parameters
    ----------
    X : gretl.Matrix
        Input data with observations in rows and features in columns.
        Missing values are not allowed.
    centers : int or gretl.Matrix
        Number of clusters, or a matrix of initial cluster centroids. If an
        integer ``k`` is given, initial centroids are selected automatically.
        If a matrix is given, it must contain ``k`` rows and the same number
        of columns as ``X``.
    opts : gretl.Bundle, optional
        Options controlling initialization, repeated runs and verbosity.

    Other Parameters
    ----------------
    init : str
        Automatic initialization method: ``"pc"`` (principal components,
        the default), ``"hw"`` (Hartigan-Wong), or ``"random"``. Ignored
        when ``centers`` is a matrix.
    n_draws : int
        Number of random starting points used in addition to deterministic
        initialization. The default is 8.
    verbosity : int
        Level of printed output, from 0 (silent, the default) to 3
        (most detailed).

    Returns
    -------
    gretl.Bundle
    Bundle containing the clustering results. Its members are
    ``cluster_id``, ``centroids``, ``within_variation`` and
    ``total_variation``.

    Notes
    -----
    ``cluster_id`` is an observation-length vector containing the 1-based
    cluster assignment for each observation.

    ``centroids`` is a matrix with ``k`` rows and ``n + 2`` columns, where
    ``n`` is the number of features. The first ``n`` columns contain the
    cluster centroids, followed by the number of observations in each
    cluster and the sum of squared deviations from the centroid.

    ``within_variation`` gives the total squared deviations from the
    assigned cluster centroids, while ``total_variation`` gives the total
    squared deviations from the global centroid.

    The k-means objective may have multiple local optima, and the algorithm
    is not guaranteed to find the global optimum. By default, deterministic
    initialization is followed by several random starts and the best result
    is retained.

    Examples
    --------
    ``gretl.kmeans(X, 3)``
    ``gretl.kmeans(X, initial_centers)``
    ``gretl.kmeans(X, 3, opts)``
    )doc";

    inline constexpr char tdisagg[] = R"doc(
    Perform temporal disaggregation of time-series data.

    Converts low-frequency observations in ``Y`` to a higher frequency.
    The expansion factor ``s`` specifies the number of high-frequency
    observations corresponding to each low-frequency observation. Optional
    high-frequency covariates may be supplied in ``X``.

    Parameters
    ----------
    Y : gretl.Matrix
        Low-frequency time-series data to be disaggregated.
    s : int
        Frequency expansion factor, for example 3 when converting quarterly
        data to monthly frequency.
    X : gretl.Matrix, optional
        High-frequency covariates used to aid the disaggregation.

    Other Parameters
    ----------------
    opts : gretl.Bundle
        Options controlling the disaggregation procedure.
    results : gretl.Bundle
        Bundle in which details of the disaggregation procedure are stored.

    Returns
    -------
    gretl.Matrix
    Disaggregated high-frequency data.

    Notes
    -----
    When ``X`` is supplied, it must contain data at the target high
    frequency. The ``opts`` bundle controls the details of the temporal
    disaggregation procedure, while ``results`` can be supplied to retrieve
    additional results from the calculation.

    Examples
    --------
    ``gretl.tdisagg(Y, 3)``
    ``gretl.tdisagg(Y, 3, X=X)``
    ``gretl.tdisagg(Y, 4, opts=opts, results=results)``
    )doc";

    inline constexpr char kpsscrit[] = R"doc(
    Compute critical values for the KPSS stationarity test.

    Returns the critical values at the 10, 5 and 1 percent significance
    levels for a KPSS test, based on the specified sample size.

    Parameters
    ----------
    T : int
        Number of observations.
    trend : bool, optional
        If ``True``, compute critical values for a test including a
        deterministic trend. The default is ``False``.

    Returns
    -------
    gretl.Matrix
        Row vector containing the critical values at the 10, 5 and
        1 percent significance levels, in that order.

    Notes
    -----
    The critical values are based on response surfaces estimated using
    the method described by Sephton (Economics Letters, 1995).

    Examples
    --------
    ``gretl.kpsscrit(100)``
    ``gretl.kpsscrit(100, trend=True)``
    )doc";

    inline constexpr char varlist[] = R"doc(
    List currently defined gretl objects.

    By default, prints the series in the current dataset. The listing may
    be restricted to objects of a specified type.

    Other Parameters
    ----------------
    type : str
        Type of objects to list. Supported values are ``"series"``,
        ``"scalar"``, ``"matrix"``, ``"list"``, ``"string"``,
        ``"bundle"``, ``"array"`` and ``"accessor"``.

    Returns
    -------
    None
        This function prints the requested listing.

    Notes
    -----
    With ``type="accessor"``, the function lists currently available
    internal accessors, such as ``$nobs`` and ``$uhat``, regardless of
    their underlying type.

    Examples
    --------
    ``gretl.varlist()``
    ``gretl.varlist(type="matrix")``
    ``gretl.varlist(type="accessor")``
    )doc";

    inline constexpr char print[] = R"doc(
    Print series or user-defined gretl objects.

    Prints the values of dataset series or user-defined objects such as
    scalars, matrices, strings, bundles and arrays.

    Parameters
    ----------
    varlist : str, optional
        Series or object names to print. If omitted, all series in the
        default or specified dataset are printed.

    Other Parameters
    ----------------
    data : gretl.Dataset
        Dataset to use instead of the default dataset.
    byobs : bool
        Print series by observation instead of by variable.
    no_dates : bool
        Use simple observation numbers instead of dates or observation
        markers when printing by observation.
    range : str
        Restrict output to the specified range, given as ``"start:stop"``.
    head : bool or int
        Print only the first elements. An integer specifies their number;
        the default is 5.
    tail : bool or int
        Print only the last elements. An integer specifies their number;
        the default is 5.
    midas : bool
        Print a MIDAS list at its native frequency.
    tree : bool
        Recursively list the contents of nested bundles.

    Returns
    -------
    None

    Notes
    -----
    For series, ``range``, ``head`` and ``tail`` refer to observations;
    for matrices to rows, for arrays to elements, and for strings to
        lines.

    Examples
    --------
    ``gretl.print("x1 x2")``
    ``gretl.print("x1 x2", byobs=True)``
    ``gretl.print("A")``
    ``gretl.print("B", tree=True)``
    )doc";

    inline constexpr char summary[] = R"doc(
    Compute summary statistics.

    Computes descriptive statistics for variables in the default Dataset,
    or for the columns of a Matrix specified via ``matrix``.

    Parameters
    ----------
    varlist : str or list, optional
        Variables to summarize in the default Dataset, specified by names
        or indices. If omitted, all variables are summarized.

    Other Parameters
    ----------------
    matrix : gretl.Matrix
        Matrix whose columns are to be summarized. Cannot be used together
        with ``varlist``.
    simple : bool
        Report basic statistics only.
    weight : str
        Variable to use for weighting the observations. Available only
        for the Dataset-based form.
    by : str
        Discrete variable by which to group the observations. Available
        only for the Dataset-based form.
    quiet : bool
        Suppress printed output.

    Returns
    -------
    gretl.Matrix or None
    Matrix containing the summary statistics. For the Dataset-based
    form with ``by``, a matrix is returned only when a single variable
    is explicitly specified; otherwise ``None`` is returned.

    Notes
    -----
    The ``by`` and ``weight`` options are mutually exclusive. They are
    not available when ``matrix`` is specified.

    Examples
    --------
    ``res = gretl.summary()``
    ``res = gretl.summary("x1 x2")``
    ``res = gretl.summary(["x1", "x2"], simple=True)``
    ``res = gretl.summary(matrix=M)``
    )doc";

} // namespace doc::Dataset
