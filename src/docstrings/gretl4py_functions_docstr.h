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

} // namespace doc::Dataset

