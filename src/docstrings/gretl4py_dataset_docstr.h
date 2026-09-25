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

    inline constexpr char addobs[] = R"doc(
    Add observations to the dataset.

    Adds the specified number of observations to the end of the current
    dataset. This is primarily intended to extend the dataset for
    forecasting.

    Parameters
    ----------
    n : int
        Number of observations to add. Must be positive.

    Other Parameters
    ----------------
    panel_time : bool
        For panel data, add ``n`` time-series observations for each
        cross-sectional unit instead of adding ``n`` new units.

    Returns
    -------
    None

    Notes
    -----
    Values of most variables in the added observations are set to missing.
    Deterministic variables recognized by gretl, including a linear trend
    and periodic dummy variables, are extended automatically.

    For panel data, the default is to add ``n`` cross-sectional units.
    Set ``panel_time=True`` to extend the time dimension instead.

    Examples
    --------
    ``data.addobs(24)``
    ``data.addobs(2, panel_time=True)``
    )doc";

    inline constexpr char insobs[] = R"doc(
    Insert an observation into the dataset.

    Inserts a single observation at the specified position, shifting all
    subsequent observations by one place.

    Parameters
    ----------
    n : int
        Position at which to insert the observation. Must be positive and
        no greater than the current number of observations.

    Returns
    -------
    None

    Notes
    -----
    The dataset is extended by one observation. All variables except the
    constant are set to missing at the inserted observation.

    This operation is not available for panel datasets.

    Examples
    --------
    ``data.insobs(10)``
    )doc";

    inline constexpr char compact[] = R"doc(
    Compact time-series data to a lower frequency.

    Converts all series in the dataset to a lower frequency by aggregating
    groups of higher-frequency observations. By default, observations are
    aggregated using their mean.

    Parameters
    ----------
    freq : int or str
        Target frequency. This should normally be lower than the current
        frequency, for example 4 when converting monthly data to quarterly.
    method : str, optional
        Aggregation method. Supported values are ``"sum"``, ``"first"``,
        ``"last"`` and ``"spread"``. If omitted, the mean is used.

    Other Parameters
    ----------------
    repday : int
        Representative day for daily-to-weekly compaction, from 0 (Sunday)
        to 6 (Saturday). The value for this day is used as the weekly
        observation.
    weekstart : int
        Starting day of the week for daily-to-weekly compaction: 0 for
        Sunday or 1 for Monday. For 7-day data the default is Monday.

    Returns
    -------
    None

    Notes
    -----
    This operation is available only for time-series datasets.

    The ``"sum"`` method sums the higher-frequency observations;
    ``"first"`` and ``"last"`` select the first and last observation,
    respectively; and ``"spread"`` distributes the sub-period observations
    across separate series.

    Daily data with frequency 5, 6 or 7 may be compacted to weekly frequency
    by specifying ``freq=52``. The ``repday`` and ``weekstart`` options apply
    only to daily-to-weekly compaction.

    Examples
    --------
    ``data.compact(4)``
    ``data.compact(4, "last")``
    ``data.compact(52, repday=3)``
    ``data.compact(52, weekstart=0)``
    )doc";

    inline constexpr char expand[] = R"doc(
    Expand time-series data to a higher frequency.

    Expands all series in an annual or quarterly dataset to a higher
    frequency by repeating the existing values.

    Parameters
    ----------
    freq : int, optional
        Target frequency. Annual data may be expanded to quarterly or
        monthly frequency, and quarterly data to monthly frequency.
        For annual data, the default target frequency is quarterly.

    Returns
    -------
    None

    Notes
    -----
    This operation is available only for annual or quarterly time-series
    datasets. Annual data can be expanded to quarterly or monthly frequency,
    while quarterly data can be expanded to monthly frequency.

    Expansion simply repeats the existing lower-frequency values. For more
    sophisticated temporal disaggregation, use ``gretl.tdisagg()``.

    Examples
    --------
    ``data.expand()``
    ``data.expand(12)``
    )doc";

    inline constexpr char transpose[] = R"doc(
    Transpose the current dataset.

    Interchanges observations and variables: each observation in the
    current dataset becomes a variable, and each variable becomes an
    observation.

    Returns
    -------
    None

    Notes
    -----
    This operation can be useful when data have been imported from a source
    in which rows represent variables rather than observations.

    Examples
    --------
    ``data.transpose()``
    )doc";

    inline constexpr char sortby[] = R"doc(
    Sort the dataset by one or more variables.

    Reorders all observations in the dataset in ascending order according
    to the specified variable or variables.

    Parameters
    ----------
    varlist : str or list
        Variable or list of variables to use as sorting keys. Variables may
        be specified by name or index.

    Returns
    -------
    None

    Notes
    -----
    If multiple variables are specified, sorting is hierarchical. Ties with
    respect to the first variable are resolved using the second variable,
    and so on until the tie is broken or the sorting keys are exhausted.

    This operation is available only for undated datasets.

    Examples
    --------
    ``data.sortby("x1")``
    ``data.sortby("x1 x2 x3")``
    ``data.sortby(["x1", "x2", "x3"])``
    ``data.sortby([1, 2, 3])``
    )doc";

    inline constexpr char dsortby[] = R"doc(
    Sort the dataset by one or more variables in descending order.

    Reorders all observations in the dataset in descending order according
    to the specified variable or variables.

    Parameters
    ----------
    varlist : str or list
        Variable or list of variables to use as sorting keys. Variables may
        be specified by name or index.

    Returns
    -------
    None

    Notes
    -----
    If multiple variables are specified, sorting is hierarchical. Ties with
    respect to the first variable are resolved using the second variable,
    and so on until the tie is broken or the sorting keys are exhausted.

    This operation is available only for undated datasets.

    Examples
    --------
    ``data.dsortby("x1")``
    ``data.dsortby("x1 x2 x3")``
    ``data.dsortby(["x1", "x2", "x3"])``
    ``data.dsortby([1, 2, 3])``
    )doc";

    inline constexpr char resample[] = R"doc(
    Resample the dataset with replacement.

    Draws observations randomly with replacement from the current dataset
    and replaces the dataset with the resulting resampled data.

    Parameters
    ----------
    n : int
        Number of observations to draw.

    Returns
    -------
    None

    Notes
    -----
    Sampling is performed with replacement, so an observation may appear
    more than once in the resampled dataset.

    The random-number generator seed can be set beforehand to obtain
    reproducible results.

    Examples
    --------
    ``data.resample(100)``
    )doc";

    inline constexpr char pad_daily[] = R"doc(
    Pad a daily dataset with missing observations.

    Adds missing observations to a daily time-series dataset so that it
    conforms to the specified number of days per week.

    Parameters
    ----------
    days : int
        Number of days per week in the padded dataset. Must be 5, 6 or
        7 and greater than or equal to the current daily frequency.

    Returns
    -------
    None

    Notes
    -----
    Newly inserted observations contain missing values for all series.
    Existing observations and their dates are preserved.

    This operation applies only to daily time-series datasets.

    Examples
    --------
    ``data.pad_daily(7)``
    )doc";

    inline constexpr char unpad_daily[] = R"doc(
    Remove padding from a daily dataset.

    Removes artificial observations containing missing values that were
    added to regularize a daily time-series dataset.

    Returns
    -------
    None

    Notes
    -----
    This operation applies only to daily time-series datasets. It removes
    observations that contain missing values for all series, reducing the
    daily frequency where appropriate.

    Examples
    --------
    ``data.unpad_daily()``
    )doc";

    inline constexpr char tdisagg[] = R"doc(
    Perform temporal disaggregation of time-series data.

    Converts low-frequency observations in ``Y`` to a higher frequency.
    The expansion factor ``s`` specifies the number of high-frequency
    observations corresponding to each low-frequency observation. Variables
    in the current dataset may be used directly as input or covariates.

    Parameters
    ----------
    Y : str, list or gretl.Matrix
        Low-frequency time-series data to be disaggregated. A string or list
        identifies one or more series in the current dataset.
    s : int
        Frequency expansion factor, for example 3 when converting quarterly
        data to monthly frequency.

    Other Parameters
    ----------------
    X : str, list or gretl.Matrix
        High-frequency covariates used to aid the disaggregation. A string
        or list identifies variables in the current dataset.
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
        ``data.tdisagg("y", 3)``
        ``data.tdisagg(["y1", "y2"], 3)``
        ``data.tdisagg("y", 3, X="x")``
        ``data.tdisagg("y", 4, opts=opts, results=results)``
        )doc";

    inline constexpr char to_dict[] = R"doc(
    Export the dataset as a Python dictionary.

    Converts the current dataset to the dictionary representation accepted
    by gretl4py as dataset input.

    Returns
    -------
    dict
        Dictionary containing the variable names, observation labels,
        data as a NumPy array, and dataset periodicity.

    Notes
    -----
    The returned dictionary contains the entries ``varname``,
    ``obs_labels``, ``data_array`` and ``periodicity``.

    Examples
    --------
    ``d = data.to_dict()``
    )doc";

} // namespace doc::Dataset

