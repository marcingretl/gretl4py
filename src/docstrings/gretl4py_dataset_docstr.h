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

    inline constexpr char clone[] =
    R"doc(Return a deep copy of the dataset.)doc";

    inline constexpr char sample[] = R"doc(
    Set, modify, or report the current sample.

    Restricts the dataset sample by observation range, offsets, a dummy
    variable, a Boolean condition, missing-value criteria, or random
    sampling. The full sample can also be restored. Called without
    arguments, reports the current sample.

    Parameters
    ----------
    startobs : str or int, optional
        First observation of the sample, or an argument interpreted according
        to the selected sampling option.
    endobs : str or int, optional
        Last observation of the sample. Used primarily for observation,
        panel-unit, or panel-time ranges.

    Other Parameters
    ----------------
    varlist : str
        Variable list used by sampling options that operate on a specified
        set of variables.
    dummy : bool
        Interpret ``startobs`` as a dummy variable and retain observations
        for which it equals 1.
    restrict : bool
        Interpret ``startobs`` as a Boolean condition defining the sample.
        replace : bool
        Replace an existing sample restriction instead of applying the new
        restriction cumulatively.
        no_missing : bool
        Exclude observations having a missing value for any variable in
        ``varlist``.
        no_all_missing : bool
        Exclude observations for which all variables in ``varlist`` are
        missing.
        contiguous : bool
        Trim missing observations from the beginning and end of the sample
        and require the remaining sample to contain no missing values.
        random : bool
        Select the number of observations specified by ``startobs`` randomly,
        without replacement.
        permanent : bool
        Make the current or resulting sample restriction permanent.
        preserve_panel : bool
        Preserve the nominally balanced structure of panel data when
        applying a restriction.
        unit : bool
        Interpret ``startobs`` and ``endobs`` as a range of panel units.
        time : bool
        Interpret ``startobs`` and ``endobs`` as a range of panel time
        periods.
        dates : bool
        Force ``startobs`` and ``endobs`` to be interpreted as dates rather
        than sequential observation numbers.
        quiet : bool
        Suppress reporting of the resulting sample range.
        full : bool
        Restore the full sample.

    Returns
    -------
    None

    Notes
    -----
    Sample restrictions are cumulative by default. Use ``replace=True`` to
    replace an existing restriction.

    With ``no_missing``, observations are excluded if at least one variable
    in ``varlist`` is missing; with ``no_all_missing``, they are excluded
    only if all variables in ``varlist`` are missing.

    Random sampling is performed without replacement. Set the random-number
    generator seed beforehand to obtain reproducible samples.

    ``permanent=True`` may also be used by itself to make the current sample
    permanent. A permanent restriction cannot subsequently be undone by
    restoring the full sample.

    Examples
    --------
    ``data.sample()``
    ``data.sample("2000:1", "2020:4")``
    ``data.sample(10, 100)``
    ``data.sample("d", dummy=True)``
    ``data.sample("x1 > 0", restrict=True)``
    ``data.sample(100, random=True)``
    ``data.sample(no_missing=True, varlist="x1 x2 x3")``
    ``data.sample(full=True)``
    )doc";

} // namespace doc::Dataset

