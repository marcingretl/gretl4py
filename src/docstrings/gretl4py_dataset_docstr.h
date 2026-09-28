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

    inline constexpr char to_file[] = R"doc(
    Save dataset data to a file.

    Saves the current dataset, or a selected subset of its series, to a
    file. The output format is determined by the filename extension or
    by the specified options.

    Parameters
    ----------
    filename : str
        Name or path of the output file.
    varlist : str or list, optional
        Series to save, specified by names or indices. By default, all
        currently defined series are saved.

    Other Parameters
    ----------------
    omit_obs : bool
        Omit observation labels when writing CSV data.
    no_header : bool
        Omit variable names when writing CSV data.
    gnu_octave : bool
        Write data in GNU Octave format.
    gnu_R : bool
        Write data in a format suitable for R ``read.table``.
    gzipped : bool or int
        Apply gzip compression. An integer specifies the compression
        level from 0 to 9.
    jmulti : bool
        Write data in JMulTi ASCII format.
    dat : bool
        Write data in PcGive ASCII format.
    decimal_comma : bool
        Use a decimal comma and semicolon field separator in CSV output.
    database : bool
        Write data in gretl database format.
    overwrite : bool
        Overwrite existing series when writing to a gretl database.
    comment : str
        Add a comment when writing CSV or database data.
    matrix : str
        Write the specified matrix as a dataset.

    Returns
    -------
    None

    Notes
    -----
    If the dataset is subsampled, only observations in the current sample
    are saved. If ``filename`` does not contain a full path, the file is
    written to the current gretl working directory.

    Native ``.gdt`` and ``.gdtb`` formats are supported, as well as CSV,
    text, GNU Octave and Stata formats. If no filename extension is given,
    ``.gdt`` is assumed.

    Examples
    --------
    ``data.to_file("data.gdt")``
    ``data.to_file("subset.csv", ["y", "x1", "x2"])``
    ``data.to_file("data.gdt", gzipped=6)``
    )doc";

    inline constexpr char to_matrix[] = R"doc(
    Convert dataset data to a matrix.

    Creates a gretl Matrix from selected variables and observations of
    the current dataset.

    Parameters
    ----------
    list : str or list, optional
        Variables to include, specified by names or indices. By default,
        all variables are included.
    startobs : int, optional
        First observation to include. If 0, the start of the current
        sample is used.
    endobs : int, optional
        Last observation to include. If 0, the end of the current sample
        is used.
    missing : {"ok", "error", "skip"}, optional
        Treatment of missing values. ``"ok"`` retains missing values,
        ``"error"`` raises an error if missing values are encountered,
        and ``"skip"`` excludes observations containing missing values.
        Default is ``"ok"``.

    Returns
    -------
    gretl.Matrix
        Matrix containing the selected dataset values.

    Notes
    -----
    The rows of the returned matrix correspond to observations and its
    columns correspond to the selected variables.

    Examples
    --------
    ``m = data.to_matrix()``
    ``m = data.to_matrix(["y", "x1", "x2"])``
    ``m = data.to_matrix("y x1 x2", missing="skip")``
    ``m = data.to_matrix(["y", "x1"], 10, 100)``
    )doc";

    inline constexpr char add_series[] = R"doc(
    Add a series to the dataset.

    Adds a new series from a NumPy array or gretl Matrix. The input must
    represent a vector.

    Parameters
    ----------
    data : numpy.ndarray or gretl.Matrix
        Data to add. The input must be a row or column vector. A
        ``gretl.Matrix`` must contain real-valued data.
    name : str
        Name of the series to add.

    Returns
    -------
    None

    Notes
    -----
    The input data are copied into the dataset. If a series with the
    specified name already exists, it is replaced.

    Examples
    --------
    ``data.add_series(x, "x")``
    ``data.add_series(gretl.Matrix(x), "x")``
    )doc";

    inline constexpr char new_series[] = R"doc(
    Generate a new series in the dataset.

    Creates a series using a gretl expression. The specification may be
    given as a complete formula or as a series name and value expression.

    Parameters
    ----------
    formula : str
        Complete gretl expression defining the new series.
    name : str
        Name of the new series.
    value : str
        Gretl expression defining the values of the new series.

    Returns
    -------
    None

    Examples
    --------
    ``data.new_series("x2 = x^2")``
    ``data.new_series("x2", "x^2")``
    )doc";

    inline constexpr char rename[] = R"doc(
    Rename a series or change the case of all series names.

    Renames a series identified by name or ID number, or changes the
    naming convention of all series in the current dataset.

    Parameters
    ----------
    series : str or int
        Name or ID number of the series to rename.
    newname : str
        New name for the series.

    Other Parameters
    ----------------
    quiet : bool
        Suppress printed output.
    case : {"lower", "upper", "camel", "snake"}
        Change the case of all series names. When this option is used,
        ``series`` and ``newname`` are omitted.

    Returns
    -------
    None

    Notes
    -----
    A new series name must be at most 31 characters long, start with a
    letter, and contain only letters, digits and underscores. It must not
    duplicate the name of an existing object.

    Examples
    --------
    ``data.rename("x2", "income")``
    ``data.rename(2, "income")``
    ``data.rename(case="lower")``
    )doc";

    inline constexpr char new_list[] = R"doc(
    Generate a new named list of variables.

    Creates a gretl list using a complete gretl expression or a list name
    and value expression.

    Parameters
    ----------
    formula : str
        Complete gretl expression defining the list.
    name : str
        Name of the new list.
    value : str
        Gretl expression specifying the variables in the list, by name
        or variable ID.

    Returns
    -------
    None

    Examples
    --------
    ``data.new_list("list A = x1 x2 x3")``
    ``data.new_list("A", "x1 x2 x3")``
    )doc";

    inline constexpr char dummify[] = R"doc(
    Generate dummy variables from discrete variables.

    For each suitable variable in ``varlist``, creates a set of dummy
    variables representing its distinct values.

    Parameters
    ----------
    varlist : str or list
        Variables to dummify, specified by names or indices.

    Other Parameters
    ----------------
    drop_first : bool
        Omit the dummy corresponding to the lowest value.
    drop_last : bool
        Omit the dummy corresponding to the highest value.

    Returns
    -------
    None

    Notes
    -----
    Variables are suitable if they are explicitly marked as discrete, or
    take a relatively small number of sufficiently round distinct values.
    By default, one dummy variable is created for each distinct value.

    Examples
    --------
    ``data.dummify("x")``
    ``data.dummify("x1 x2", drop_first=True)``
    )doc";

    inline constexpr char square[] = R"doc(
    Generate squares and cross-products of variables.

    Creates new series containing the squares of the variables in
    ``varlist`` and, optionally, their cross-products.

    Parameters
    ----------
    varlist : str or list
        Variables to transform, specified by names or indices.

    Other Parameters
    ----------------
    cross : bool
        Generate cross-products in addition to squares.

    Returns
    -------
    None

    Notes
    -----
    Dummy variables are not squared, since their squares are identical
    to the original variables.

    Examples
    --------
    ``data.square("x1 x2")``
    ``data.square(["x1", "x2"], cross=True)``
    )doc";

    inline constexpr char discrete[] = R"doc(
    Mark variables as discrete or continuous.

    Marks the variables in ``varlist`` as discrete. This affects their
    treatment by operations that distinguish discrete from continuous
    variables.

    Parameters
    ----------
    varlist : str or list
        Variables to mark, specified by names or indices.

    Other Parameters
    ----------------
    reverse : bool
        Mark the variables as continuous instead of discrete.

    Returns
    -------
    None

    Notes
    -----
    By default, variables are treated as continuous. Marking a variable
    as discrete affects its treatment in frequency plots and makes it
    available for ``dummify()``.

    Examples
    --------
    ``data.discrete("x")``
    ``data.discrete(["x1", "x2"])``
    ``data.discrete("x", reverse=True)``
    )doc";

    inline constexpr char stdize[] = R"doc(
    Standardize or center variables.

    Creates standardized versions of the variables in ``varlist``. By
    default, each variable is centered and divided by its sample standard
    deviation.

    Parameters
    ----------
    varlist : str or list
        Variables to transform, specified by names or indices.

    Other Parameters
    ----------------
    no_df_corr : bool
        Do not apply a degrees-of-freedom correction when computing the
        standard deviation.
    center_only : bool
        Subtract the mean without dividing by the standard deviation.

    Returns
    -------
    None

    Notes
    -----
    By default, the generated series have the prefix ``s_`` and the
    standard deviation is computed with a degrees-of-freedom correction
    of 1. With ``center_only=True``, the generated series have the prefix
    ``c_``.

    Examples
    --------
    ``data.stdize("x1 x2")``
    ``data.stdize(["x1", "x2"], no_df_corr=True)``
    ``data.stdize("x1", center_only=True)``
    )doc";

    inline constexpr char diff[] = R"doc(
    Generate first differences of variables.

    Computes the first difference of each variable in ``varlist`` and
    stores the result as a new series with the prefix ``d_``.

    Parameters
    ----------
    varlist : str or list
        Variables to difference, specified by names or indices.

    Returns
    -------
    None

    Examples
    --------
    ``data.diff("x1 x2")``
    ``data.diff(["x1", "x2"])``
    )doc";

    inline constexpr char ldiff[] = R"doc(
    Generate log differences of variables.

    Computes the first difference of the natural logarithm of each
    variable in ``varlist`` and stores the result as a new series with
    the prefix ``ld_``.

    Parameters
    ----------
    varlist : str or list
        Variables to transform, specified by names or indices.

    Returns
    -------
    None

    Examples
    --------
    ``data.ldiff("x1 x2")``
    ``data.ldiff(["x1", "x2"])``
    )doc";

    inline constexpr char sdiff[] = R"doc(
    Generate seasonal differences of variables.

    Computes the seasonal difference of each variable in ``varlist`` and
    stores the result as a new series with the prefix ``sd_``.

    Parameters
    ----------
    varlist : str or list
        Variables to difference, specified by names or indices.

    Returns
    -------
    None

    Notes
    -----
    This transformation is available only for seasonal time-series data.

    Examples
    --------
    ``data.sdiff("x1 x2")``
    ``data.sdiff(["x1", "x2"])``
    )doc";

    inline constexpr char logs[] = R"doc(
    Generate natural logarithms of variables.

    Computes the natural logarithm of each variable in ``varlist`` and
    stores the result as a new series with the prefix ``l_``.

    Parameters
    ----------
    varlist : str or list
        Variables to transform, specified by names or indices.

    Returns
    -------
    None

    Examples
    --------
    ``data.logs("x1 x2")``
    ``data.logs(["x1", "x2"])``
    )doc";

    inline constexpr char orthdev[] = R"doc(
    Generate forward orthogonal deviations.

    Computes forward orthogonal deviations for each variable in ``varlist``
    and stores the result as a new series with the prefix ``o_``.

    Parameters
    ----------
    varlist : str or list
        Variables to transform, specified by names or indices.

    Returns
    -------
    None

    Notes
    -----
    This transformation is available only for panel data.

    The values are stored one period ahead of their true temporal
    location. Thus the first observation of each time series is lost,
    rather than the last, for compatibility with first differences.

    Examples
    --------
    ``data.orthdev("x1 x2")``
    ``data.orthdev(["x1", "x2"])``
    )doc";

    inline constexpr char lags[] = R"doc(
    Generate lagged values of variables.

    Creates lagged versions of each variable in ``varlist``. By default,
    the number of lags generated equals the periodicity of the dataset.

    Parameters
    ----------
    varlist : str or list
        Variables to lag, specified by names or indices.
    order : int, optional
        Maximum lag order. If 0, the dataset periodicity is used.
        Default is 0.

    Returns
    -------
    None

    Notes
    -----
    The generated series are named using the original variable name and
    the lag order. For example, lags 1 through 4 of ``x`` are named
    ``x_1``, ``x_2``, ``x_3`` and ``x_4``.

    Examples
    --------
    ``data.lags("x")``
    ``data.lags("x y", 12)``
    ``data.lags(["x1", "x2"], 4)``
    )doc";

    inline constexpr char print[] = R"doc(
    Print values of dataset series.

    Prints the values of selected series in this dataset. If ``varlist``
    is omitted, all series in the dataset are printed.

    Parameters
    ----------
    varlist : str or list, optional
        Series to print, specified by names or indices. By default, all
        series are printed.

    Other Parameters
    ----------------
    byobs : bool
        Print data by observation instead of by variable.
    no_dates : bool
        Use simple observation numbers instead of dates or observation
        markers when printing by observation.
    range : str
        Restrict output to the specified observation range, given as
        ``"start:stop"``.
    head : bool or int
        Print only the first observations. An integer specifies their
        number; the default is 5.
    tail : bool or int
        Print only the last observations. An integer specifies their
        number; the default is 5.
    midas : bool
        Print a MIDAS list at its native frequency.

    Returns
    -------
    None

    Notes
    -----
    This is a dataset-specific version of the general ``gretl.print()``
    facility. It operates on this dataset rather than the default dataset
    and prints dataset series only; user-defined objects are not handled.

    The ``data`` option of the general printer is not available, since
    the dataset is determined by the instance.

    Examples
    --------
    ``data.print()``
    ``data.print("x1 x2")``
    ``data.print(["x1", "x2"], byobs=True)``
    ``data.print("x1 x2", head=10)``
    )doc";

    inline constexpr char get_series[] = R"doc(
    Retrieve a series as a gretl Matrix.

    Returns the values of the specified dataset series over the current
    sample range as a column vector.

    Parameters
    ----------
    series : str
        Name of the series to retrieve.

    Returns
    -------
    gretl.Matrix
        Column vector containing the series values over the current sample.

    Examples
    --------
    ``x = data.get_series("x")``
    )doc";

    inline constexpr char hpfilt[] = R"doc(
    Apply the Hodrick-Prescott filter to a series.

    Returns the cycle or trend component obtained by applying the
    Hodrick-Prescott filter to the specified series.

    Parameters
    ----------
    y : str
        Series to filter.
    hp_lambda : float, optional
        Smoothing parameter. If omitted, gretl uses 100 times the square
        of the dataset periodicity.
    trend : bool, optional
        Return the trend component instead of the cycle component.
        Default is False.
    one_sided : bool, optional
        Use the one-sided variant of the filter, with no look-ahead.
        Default is False.

    Returns
    -------
    gretl.Matrix
        Filtered series.

    Notes
    -----
    The default smoothing parameter is 100 for annual data, 1600 for
    quarterly data, and so on. The one-sided filter follows Stock and
    Watson (1999).

    Examples
    --------
    ``cycle = data.hpfilt("y")``
    ``trend = data.hpfilt("y", trend=True)``
    ``cycle = data.hpfilt("y", hp_lambda=1600, one_sided=True)``
    )doc";

    inline constexpr char bwfilt[] = R"doc(
    Apply a low-pass Butterworth filter to a series.

    Applies a low-pass Butterworth filter of order ``n`` and frequency
    cutoff ``omega`` to the specified series.

    Parameters
    ----------
    y : str
        Series to filter.
    n : int
        Order of the filter.
    omega : float
        Frequency cutoff in degrees. Must be greater than 0 and less
        than 180.

    Returns
    -------
    gretl.Matrix
        Filtered series.

    Notes
    -----
    Smaller cutoff values restrict the pass-band to lower frequencies
    and produce a smoother trend. Higher filter orders produce a sharper
    cutoff, at the cost of possible numerical instability.

    Examples
    --------
    ``trend = data.bwfilt("y", 8, 67)``
    )doc";

    inline constexpr char bkfilt[] = R"doc(
    Apply the Baxter-King band-pass filter to a series.

    Applies the Baxter-King band-pass filter to the specified series.

    Parameters
    ----------
    y : str
        Series to filter.
    f1 : int, optional
        Lower bound of the frequency range to extract. If 0, a default
        based on the dataset periodicity is used.
    f2 : int, optional
        Upper bound of the frequency range to extract. If 0, a default
        based on the dataset periodicity is used.
    k : int, optional
        Approximation order. If 0, a default based on the dataset
        periodicity is used.

    Returns
    -------
    gretl.Matrix
        Filtered series.

    Notes
    -----
    The default values of ``f1``, ``f2`` and ``k`` are 2, 8 and 3 for
    annual data; 6, 32 and 12 for quarterly data; and 18, 96 and 36 for
    monthly data.

    If ``f2`` is greater than or equal to the number of available
    observations, the low-pass version of the filter is applied and the
    result should be interpreted as an estimate of the trend component.

    Examples
    --------
    ``cycle = data.bkfilt("y")``
    ``cycle = data.bkfilt("y", 6, 32, 12)``
    )doc";

    inline constexpr char get_accessor[] = R"doc(
    Retrieve a gretl dollar accessor.

    Parameters
    ----------
    name : str
        Name of the accessor, with or without the leading '$'.

    Returns
    -------
    object
        Value of the requested accessor.

    Notes
    -----
    The accessor may be associated with the current Dataset instance or
    may be a general gretl accessor, such as '$version'.
    )doc";

    inline constexpr char corr[] = R"doc(
    Compute pairwise correlations for selected variables.

    Parameters
    ----------
    varlist : str or list, optional
        Variables to use. If omitted, all variables are used.

    Other Parameters
    ----------------
    uniform : bool, optional
        Use a common sample for all pairwise correlations.
    spearman : bool, optional
        Compute Spearman's rank correlation.
    kendall : bool, optional
        Compute Kendall's rank correlation.
    verbose : bool, optional
        Print rankings for rank correlations.
    triangle : bool, optional
        Plot only the lower triangle of the correlation matrix.
    quiet : bool, optional
        Suppress printed output.

    Returns
    -------
    Matrix
        Matrix containing the computed correlations.

    Notes
    -----
    Pearson's product-moment correlation is computed by default.
    The `spearman` and `kendall` options are mutually exclusive and
    require exactly two variables.
    )doc";

    inline constexpr char cov[] = R"doc(
    Compute the covariance matrix for selected variables.

    Parameters
    ----------
    varlist : str or list, optional
        Variables to use. If omitted, all variables are used.

    Returns
    -------
    Matrix
        Covariance matrix of the selected variables.
    )doc";

    inline constexpr char xcorrgm[] = R"doc(
    Compute the cross-correlogram of two series.

    Computes the sample cross-correlations between the current value of
    the first series and successive leads and lags of the second series.

    Parameters
    ----------
    varlist : str or list
        Two series, specified by names or indices.
    series1 : str
        First series.
    series2 : str
        Second series.
    order : int, optional
        Maximum number of leads and lags. If 0, the order is selected
        automatically based on the dataset periodicity and sample size.

    Other Parameters
    ----------------
    plot : str
        Plot mode or output filename. Use ``"none"`` to suppress the plot
        or ``"display"`` to force display.
    quiet : bool
        Suppress printed output.

    Returns
    -------
    gretl.Matrix
    Two-column matrix containing lags in the first column and
    cross-correlations in the second.

    Examples
    --------
    ``m = data.xcorrgm("x y", 12)``
    ``m = data.xcorrgm(["x", "y"], 12)``
    ``m = data.xcorrgm("x", "y", 12)``
    )doc";

    inline constexpr char pca[] = R"doc(
    Perform Principal Components Analysis.

    Computes principal components for the variables in ``varlist``,
    using the correlation matrix by default.

    Parameters
    ----------
    varlist : str or list, optional
        Variables to include, specified by names or indices.

    Other Parameters
    ----------------
    covariance : bool
        Use the covariance matrix instead of the correlation matrix.
    save : bool or int
        Save major components as series. If True, components with
        eigenvalues greater than the mean are saved. An integer specifies
        the number of leading components to save.
    save_all : bool
        Save all components as series.
    quiet : bool
        Suppress printed results.

    Returns
    -------
    gretl.Bundle
    Bundle containing the PCA results. Its principal members are
    ``n`` (number of observations used), ``n_dropped`` (number of
    observations dropped), ``eigenvals`` (eigenvalue results),
    ``loadings`` (component loadings), and ``matrix_type``
    (``"correlation"`` or ``"covariance"``).

    Notes
    -----
    Unless ``quiet=True``, the eigenvalues, proportions of joint variance,
    and corresponding eigenvectors (component loadings) are printed.

    Saved components are added to the dataset as ``PC1``, ``PC2``, and
    so on.

    Examples
    --------
    ``res = data.pca("x1 x2 x3")``
    ``res = data.pca(["x1", "x2", "x3"], covariance=True)``
    ``res = data.pca("x1 x2 x3", save=2)``
    )doc";

    inline constexpr char kmeans[] = R"doc(
    Perform k-means clustering.

    Applies the Hartigan-Wong k-means algorithm (ASA136) to the variables
    in ``X``, minimizing the within-cluster sum of squared Euclidean
    distances.

    Parameters
    ----------
    X : str or list
        Variables defining the features, specified by names or indices.
        Missing values are not permitted.
    centers : int or gretl.Matrix
        Either the number of clusters or a matrix containing the initial
        cluster centroids. In the latter case, the matrix must have one
        row per cluster and one column per feature.
    opts : gretl.Bundle, optional
        Options controlling initialization and execution. Supported
        members are ``init``, ``n_draws`` and ``verbosity``.

    Returns
    -------
    gretl.Bundle
        Bundle containing ``cluster_id``, ``centroids``,
        ``within_variation`` and ``total_variation``.

    Notes
    -----
    ``init`` selects the automatic initialization method and may be
    ``"pc"`` (Principal Components, the default), ``"hw"`` (Hartigan-Wong)
    or ``"random"``. It is ignored if ``centers`` is a matrix.

    ``n_draws`` specifies the number of random starting points, with
    default 8. ``verbosity`` takes an integer from 0 to 3, with 0
    giving silent operation.

    ``cluster_id`` is an m-vector containing the 1-based cluster
    assignment for each observation. ``centroids`` has one row per
    cluster; its feature columns contain the centroid coordinates,
    followed by the number of observations in the cluster and the
    within-cluster sum of squared deviations.

    ``within_variation`` is the total within-cluster variation, while
    ``total_variation`` is the total variation around the global centroid.

    Examples
    --------
    ``res = data.kmeans("x1 x2 x3", 3)``
    ``res = data.kmeans(["x1", "x2", "x3"], 3)``
    ``res = data.kmeans("x1 x2 x3", initial_centers)``
    )doc";

    inline constexpr char xtab[] = R"doc(
    Compute cross-tabulations of discrete variables.

    Computes contingency tables for the variables in ``varlist``. All
    variables must be marked as discrete.

    Parameters
    ----------
    varlist : str or list, optional
        Variables to cross-tabulate, specified by names or indices.
        Two lists may be separated by a semicolon in the string form.

    Other Parameters
    ----------------
    row : bool
        Display row percentages instead of frequency counts.
    column : bool
        Display column percentages instead of frequency counts.
    zeros : bool
        Display cells with zero counts explicitly.
    no_totals : bool
        Suppress marginal counts.
    matrix : str
        Use a named matrix containing precomputed frequencies instead
        of dataset variables.
    quiet : bool
        Suppress printed output.
    tex : bool or str
        Produce LaTeX output, optionally writing it to the specified file.
    equal : bool
        Highlight equal row and column values in LaTeX output.

    Returns
    -------
    gretl.Matrix
    Contingency table containing the cross-tabulation results.

    Notes
    -----
    With a single list, a cross-tabulation is computed for each pair of
    variables. If two lists are separated by a semicolon, each variable
    in the first list is cross-tabulated by row against each variable in
    the second list by column.

    Pearson's chi-square test for independence is computed when the
    expected frequencies satisfy the required minimum condition. For a
    2-by-2 table, Fisher's exact test is also reported.

    Examples
    --------
    ``tab = data.xtab("x1 x2")``
    ``data.xtab("x1 ; x2 x3", row=True)``
    ``data.xtab(["x1", "x2"], quiet=True)``
    )doc";

    inline constexpr char hurst[] = R"doc(
    Estimate the Hurst exponent of a time series.

    Estimates the Hurst exponent, a measure of persistence or long memory,
    for a time-series variable having at least 128 observations.

    Parameters
    ----------
    series : str
        Series for which the Hurst exponent is to be estimated.

    Returns
    -------
    gretl.Matrix
        Matrix containing the estimated Hurst exponent and its standard
        error.

    Notes
    -----
    The exponent is estimated using binary sub-sampling and a regression
    of the logarithm of the rescaled range on the logarithm of sample
    size.

    A Hurst exponent of 0.5 corresponds to the benchmark of white noise.
    Values above 0.5 indicate persistence, while values below 0.5 indicate
    anti-persistence.

    Examples
    --------
    ``res = data.hurst("x")``
    )doc";

    inline constexpr char set_as_default[] = R"doc(
    Set this dataset as the default dataset.

    Makes this Dataset instance the default dataset used by gretl4py
    functions that operate on a dataset without receiving one explicitly.

    The first dataset loaded into gretl4py is automatically set as the
    default dataset. Calling this method changes the default to the current
    instance.

    Returns
    -------
    None

    Examples
    --------
    ``data.set_as_default()``
    )doc";

    inline constexpr char info[] = R"doc(
    Print basic information about the dataset.

    Displays the dataset structure, including its type, frequency,
    observation range and number of observations, followed by a list
    of variables.

    Returns
    -------
    None

    Examples
    --------
    ``data.info()``
    )doc";

    inline constexpr char summary[] = R"doc(
    Compute summary statistics for dataset variables.

    Computes descriptive statistics for the variables in ``varlist``,
    or for all variables in the dataset if ``varlist`` is omitted.

    Parameters
    ----------
    varlist : str or list, optional
        Variables to summarize, specified by names or indices.

    Other Parameters
    ----------------
    simple : bool
        Report basic statistics only.
    weight : str
        Variable to use for weighting the observations.
    by : str
        Discrete variable by which to group the observations.
    quiet : bool
        Suppress printed output.

    Returns
    -------
    gretl.Matrix or None
        Matrix containing the summary statistics. With ``by``, a matrix
        is returned only when a single variable is explicitly specified;
        otherwise ``None`` is returned.

    Notes
    -----
    The ``by`` and ``weight`` options are mutually exclusive.

    For summary statistics of a matrix, use ``gretl.Matrix.summary()``.

    Examples
    --------
    ``res = data.summary()``
    ``res = data.summary("x1 x2")``
    ``res = data.summary(["x1", "x2"], simple=True)``
    ``res = data.summary("x1", by="group")``
    )doc";

} // namespace doc::Dataset

