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

namespace doc::Bundle {

    inline constexpr char bundle[] = R"doc(
    A gretl Bundle.

    A Bundle may be created empty or initialized with elements supplied
    as keyword arguments.

    Parameters
    ----------
    **kwargs
        Initial Bundle elements. Supported values are bool, int, float,
        str, NumPy numeric arrays, gretl.Bundle, and gretl.Matrix objects.

    Examples
    --------
    ``b = gretl.Bundle()``
    ``b = gretl.Bundle(x=1.0, name="foo")``
    )doc";

    inline constexpr char copy[] = R"doc(
    Return a deep copy of the Bundle.

    Returns
    -------
    gretl.Bundle
        An independent copy of the Bundle.
    )doc";

    inline constexpr char deepcopy[] = R"doc(
    Return a deep copy of the Bundle.

    Parameters
    ----------
    memo : dict
        Memoization dictionary used by Python's copy protocol.

    Returns
    -------
    gretl.Bundle
        An independent copy of the Bundle.
    )doc";

} // namespace doc::Bundle
