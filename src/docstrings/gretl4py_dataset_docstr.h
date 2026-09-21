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

namespace doc::Dataset {

    inline constexpr char __copy__[] =
    R"doc(Return a deep copy of the dataset.)doc";

    inline constexpr char __deepcopy__[] =
    R"doc(Return a deep copy of the dataset.)doc";

    inline constexpr char sample[] = R"doc(
    Set or modify the current sample.

    Restricts the dataset sample by observation range, offsets, a dummy
    variable, a Boolean condition, missing-value criteria, or random
    sampling. The full sample can also be restored.

    Parameters
    ----------
    start : str | int, optional
        First observation of the sample, an offset relative to the current
        sample, a dummy variable, a Boolean restriction, or the number of
        observations for random sampling, depending on the selected option.
    end : str | int, optional
        Last observation of the sample or an offset relative to the current
        sample.

    Other Parameters
    ----------------
    dummy : bool
        Interpret ``start`` as a dummy variable and retain observations for
        which it equals 1.
    restrict : bool
        Interpret ``start`` as a Boolean condition defining the sample.
    replace : bool
        Replace an existing sample restriction instead of applying the new
        restriction cumulatively.
        no_missing : bool
        Exclude observations having a missing value for any selected
        variable.
        no_all_missing : bool
        Exclude observations for which all selected variables are missing.
        contiguous : bool
        Trim missing observations from the beginning and end of a
        time-series sample and require the remaining sample to contain no
        missing values.
        random : bool
        Select the specified number of observations randomly, without
        replacement.
        permanent : bool
        Make the sample restriction permanent by discarding observations
        outside the resulting sample.
        preserve_panel : bool
        Preserve the nominally balanced structure of panel data when
        applying a restriction.
        unit : bool
        Interpret ``start`` and ``end`` as a range of panel units.
        time : bool
        Interpret ``start`` and ``end`` as a range of panel time periods.
        dates : bool
        Force ``start`` and ``end`` to be interpreted as dates rather than
        sequential observation numbers.
        quiet : bool
        Suppress reporting of the resulting sample range.

    Notes
    -----
    Sample restrictions are cumulative by default. Use ``replace=True`` to
    replace an existing restriction. ``replace`` cannot be combined with
    ``contiguous``.

    With ``no_missing``, observations are excluded if at least one selected
    variable is missing; with ``no_all_missing``, they are excluded only if
    all selected variables are missing.

    Random sampling is performed without replacement. Set the random-number
    generator seed beforehand to obtain reproducible samples.

    Restrictions based on dummy variables, Boolean conditions, or missing
    values may alter time-series or panel structure. For panel data,
    ``preserve_panel=True`` can be used to retain a nominally balanced panel.

    A permanent restriction cannot be undone by restoring the full sample.
    )doc";

} // namespace doc::Dataset

