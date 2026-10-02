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

    inline constexpr char getitem[] = R"doc(
    Return the Bundle element stored under the given key.

    Parameters
    ----------
    key : str
        Bundle element name.

    Returns
    -------
    object
        Value associated with the key.
    )doc";

    inline constexpr char setitem[] = R"doc(
    Set a Bundle element.

    Parameters
    ----------
    key : str
        Bundle element name.
    value : bool, int, float, str, numpy.ndarray, gretl.Bundle, or gretl.Matrix
        Value to store.
    )doc";

    inline constexpr char delitem[] = R"doc(
    Delete the Bundle element stored under the given key.

    Parameters
    ----------
    key : str
        Bundle element name.
    )doc";

    inline constexpr char pop[] = R"doc(
    Remove and return the Bundle element stored under the given key.

    Parameters
    ----------
    key : str
        Bundle element name.

    Returns
    -------
    object
        Value associated with the key.
    )doc";

    inline constexpr char keys[] = R"doc(
    Return the Bundle keys.

    Returns
    -------
    list of str
        Names of the elements stored in the Bundle. If the Bundle is
        empty, an empty list is returned.
    )doc";

    inline constexpr char print[] = R"doc(
    Print the contents of the Bundle in gretl style.

    Parameters
    ----------
    tree : bool, optional
        If True, recursively print the contents of nested Bundles.
        Otherwise, print only the top-level elements.
    )doc";

    inline constexpr char to_dict[] = R"doc(
    Convert the Bundle to a Python dictionary.

    Nested Bundle objects are converted recursively to dictionaries.

    Returns
    -------
    dict
        Dictionary containing the Bundle elements.
    )doc";

} // namespace doc::Bundle
