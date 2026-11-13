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

namespace doc::Matrix {

    inline constexpr char summary[] = R"doc(
    Compute summary statistics for the matrix.

    Computes descriptive statistics for the columns of the matrix.

    Other Parameters
    ----------------
    simple : bool
        Report basic statistics only.
    quiet : bool
        Suppress printed output.

    Returns
    -------
    gretl.Matrix
        Matrix containing the summary statistics.

    Examples
    --------
    ``res = M.summary()``
    ``res = M.summary(simple=True)``
    )doc";

    inline constexpr char plot[] = R"doc(
    Generates a graph using the columns of this matrix.

    This method provides an interface to libgretl's 'gnuplot' command,
    with the '--matrix' option supplied automatically.

    Parameters
    ----------
    cols : str or list, optional
        Matrix columns to be plotted, specified using 1-based column
        numbers. The argument may be a space-separated string or a
        Python list containing integers or their string representations.

        By default, all columns except the last are plotted on the
        vertical axis, with the last column used on the horizontal
        axis. If 'time_series=True' is specified, all columns are
        plotted against time.

    **kwargs
        Additional options accepted by libgretl's 'gnuplot' command.
        Python keyword names use underscores instead of hyphens.
        Common options include 'time_series', 'with_lines', 'with_lp',
        'fit', and 'output'. The 'matrix', 'input', and 'inbuf'
        options are not supported.

    Returns
    -------
    matplotlib.figure.Figure or None
    The generated Matplotlib figure, or None when the graph
    is sent to an output file rather than displayed.

    Examples
    --------
    >>> m = gretl.Matrix(100, 4, "normal")
    >>> fig = m.plot([2, 1], with_lines=True)
    >>> fig = m.plot("1 2", time_series=True, with_lines=True)
    >>> m.plot(output="matrix_plot.gp")

    Notes
    -----
    The 'output' option controls whether the graph is displayed or
    written to a file. By default, a Matplotlib figure is generated.
    )doc";

    inline constexpr char boxplot[] = R"doc(
    Generates boxplots for selected columns of this matrix.

    This method provides an interface to libgretl's 'boxplot' command,
    with the '--matrix' option supplied automatically.

    Parameters
    ----------
    cols : str or list, optional
        Columns for which boxplots are generated, specified using
        1-based column numbers. The argument may be a space-separated
        string or a Python list containing integers or their string
        representations. By default, all columns are included.

    **kwargs
        Additional options accepted by libgretl's 'boxplot' command.
        Common options include 'notches' and 'output'. The 'matrix'
        option is handled automatically.

    Returns
    -------
    matplotlib.figure.Figure or None
        The generated Matplotlib figure, or None when the graph
        is sent to an output file rather than displayed.

    Examples
    --------
    >>> m = gretl.Matrix(100, 4, "normal")
    >>> fig = m.boxplot()
    >>> fig = m.boxplot([1, 3], notches=True)
    >>> m.boxplot("1 2 3", output="boxplot.gp")

    Notes
    -----
    Each boxplot summarizes the distribution of the corresponding
    matrix column, including its median, quartiles, whiskers,
    and potential outliers.
    )doc";

    inline constexpr char scatters[] = R"doc(
    Generates multiple pairwise plots from columns of this matrix.

    This method provides an interface to libgretl's 'scatters' command,
    with the '--matrix' option supplied automatically.

    Parameters
    ----------
    cols : str or list
        Selection of matrix columns, specified using 1-based column
        numbers. The selection must define two groups separated by
        a semicolon: either one dependent column followed by several
        horizontal-axis columns, or several dependent columns followed
        by one horizontal-axis column.

        Use a string such as '1 ; 2 3 4' or '1 2 3 ; 4' to specify
        the separator.

    **kwargs
        Additional options accepted by libgretl's 'scatters' command,
        including 'with_lines' and 'output'. The 'matrix' option is
        handled automatically.

    Returns
    -------
    matplotlib.figure.Figure or None
        A Matplotlib figure containing the generated plots, or None
        when the graph is sent to an output file rather than displayed.

    Examples
    --------
    >>> m = gretl.Matrix(100, 4, "normal")
    >>> fig = m.scatters("1 ; 2 3 4")
    >>> fig = m.scatters("1 2 3 ; 4", with_lines=True)
    >>> m.scatters("1 ; 2 3", output="scatters.gp")

    Notes
    -----
    By default, individual observations are displayed as points.
    Setting 'with_lines=True' produces line graphs instead.

    The maximum number of individual plots is 16; additional
    plots are ignored by libgretl.
    )doc";

} // namespace doc::Matrix
