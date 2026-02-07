import numpy as np
from . import _gretl

_plt = None

def _setup_matplotlib(backend: str | None = None):
    """
    Setup and return the matplotlib.pyplot module with an appropriate backend.

    This function chooses a suitable interactive backend depending on the platform,
    unless the user explicitly provides a backend via the `backend` parameter.

    Platform defaults (used if `backend` is None):
        - macOS: "MacOSX"
        - Windows: "TkAgg"
        - Other platforms: "TkAgg"

    If the backend setup fails (e.g., no GUI available), it falls back to
    the non-interactive "Agg" backend, suitable for saving figures to files.

    The function caches the imported matplotlib.pyplot module in the global
    variable `_plt` to avoid re-importing or reconfiguring the backend multiple times.

    Parameters
    ----------
    backend : str, optional
        Name of the matplotlib backend to use (e.g., 'TkAgg', 'Qt5Agg', 'Agg').
        If None, a platform-specific default is chosen.

    Returns
    -------
    matplotlib.pyplot
        The pyplot module configured with the selected backend.

    Notes
    -----
    - For interactive plotting in scripts or PowerShell, this will typically
      show figures immediately when plt.show() is called.
    - For headless environments (no display), the "Agg" backend is used and
      figures can be saved to files but not displayed.
    """
    global _plt

    if _plt is not None:
        return _plt

    import sys
    import matplotlib

    try:
        if backend is not None:
            matplotlib.use(backend, force=True)
        else:
            if sys.platform == "darwin":
                matplotlib.use("MacOSX")
            elif sys.platform.startswith("win"):
                matplotlib.use("TkAgg")
            else:
                matplotlib.use("TkAgg")
    except Exception:
        # fallback for headless or unsupported environments
        matplotlib.use("Agg")

    import matplotlib.pyplot as plt
    _plt = plt

    return plt

def _set_params_for_plot(model: "GretlModel", y: np.ndarray) -> dict:
    """Set numerical parameters for plots."""
    pd = int(model.dataset.get_accessor("$pd"))
    n = max(int(model.T), len(y))
    obs_labels = model.dataset.obs_labels
    xtics = obs_labels[-n:] if obs_labels is not None else None

    if not xtics:
        max_xtics = min(model._dataset.n, 10)
    else:
        max_xtics = max(pd, 4)

    return {"xtics": xtics, "max_xtics": max_xtics, "n": n}

def _set_matrices_for_plot(model: "GretlModel", accessor: str, eqn: int) -> dict:
    """Set input data (yhat, uhat, etc) for plots."""
    if eqn < 0:
        raise ValueError("Equation number must not be negative.")

    y_name = []
    data = None

    # we set variables to plot (names, we loop over it)
    if isinstance(model, _gretl.Model_var):
        if eqn > model.neqns:
            raise ValueError(f"Equation number bigger than system size ({model.neqns})")
        elif eqn == 0:
            y_name = [model.dataset.varnames()[var] for var in model.ylist]
        else:
            y_name.append(model.dataset.varnames()[model.ylist[eqn-1]])
    else:
        y_name.append(model.depvar)
    if len(y_name) == 0:
        raise ValueError("No fitted series found")

    # we get yhat. uhat values
    if isinstance(model, _gretl.Model_NSE):
        if accessor == "$yhat":
            data = model.yhat
        elif accessor == "$uhat":
            data = model.uhat
        else:
            raise ValueError(f"Unknown accessor: {accessor}")
    else:
        data = model.get_accessor(accessor)
    if not isinstance(data, np.ndarray):
        raise ValueError("Error in getting data for plot")

    return {"y_name": y_name, "data": data}

def _time_series_plot(
    y: np.ndarray,
    xs: list[np.ndarray] | None = None,
    n: int | None = None,
    xtics: list[np.ndarray | list] | None = None,
    xlabels: list[str] | None = None,
    xlabel: str | None = None,
    ylabel: str | None = None,
    ytitle: str | None = None,
    title: str | None = None,
    max_xtics: int = 10,
    ax=None,
):
    """
    Plot a time series with a primary series and optional auxiliary series.

    This function draws a line plot of a mandatory primary series ``y`` against
    a common time axis. Optionally, one or more auxiliary series can be overlaid.
    All series share the same x-axis (time).

    The function supports automatic or user-specified x-axis labels (``xtics``),
    controls tick density for dense time series, and works both as a standalone
    plot or within an existing matplotlib Axes object.

    Parameters
    ----------
    y : np.ndarray
        Mandatory primary series to plot. Must be a one-dimensional array.
    xs : list of np.ndarray, optional
        List of auxiliary one-dimensional series to overlay on the plot.
        Each array must be one-dimensional. Default is None.
    n : int, optional
        Length of the time series. Used only if it cannot be inferred from
        ``y``. If None, defaults to ``len(y)``.
    xtics : list of array-like, optional
        Custom x-axis tick labels. Must be a list with the same length as ``xs``.
        Since all series share a common time axis, only the first entry is used
        to label the x-axis.
    xlabels : list of str, optional
        Labels for auxiliary series, used in the legend. Must have the same
        length as ``xs``.
    xlabel : str, optional
        Label for the x-axis. Defaults to ``"Time"``.
    ylabel : str, optional
        Label for the y-axis. Defaults to ``"Value"``.
    ytitle : str, optional
        Label for the primary series ``y`` in the legend.
    title : str, optional
        Title of the plot.
    max_xtics : int, optional
        Maximum number of x-axis ticks to display. Tick positions are
        automatically thinned for dense time series. Default is 10.
    ax : matplotlib.axes.Axes, optional
        Axes object on which to draw the plot. If None, a new figure and axes
        are created and displayed immediately.

    Notes
    -----
    - The primary series ``y`` is always required and must be one-dimensional.
    - Auxiliary series are passed explicitly as a list to avoid ambiguity
      associated with multidimensional arrays.
    - All series share a common time axis; multiple x-axes are not supported.
    - If ``ax`` is None, the plot is shown immediately via ``plt.show()``.
      When plotting into subplots, call ``plt.show()`` explicitly after all
      plots are created.

    Examples
    --------
    >>> _time_series_plot(y=fitted)
    >>> _time_series_plot(y=fitted, xs=[actual], xlabels=["actual"])
    >>> fig, ax = plt.subplots()
    >>> _time_series_plot(y=fitted, xs=[actual, benchmark],
    ...                   xlabels=["actual", "benchmark"], ax=ax)
    """

    from ._plotting import _setup_matplotlib
    plt = _setup_matplotlib()

    # --- Validation -----------------------------------------------------

    if y.ndim != 1:
        raise ValueError("y must be a one-dimensional numpy array")

    if xs is not None:
        if not isinstance(xs, (list, tuple)):
            raise TypeError("xs must be a list of one-dimensional numpy arrays")

        for i, x in enumerate(xs):
            if not isinstance(x, np.ndarray) or x.ndim != 1:
                raise ValueError(f"xs[{i}] must be a one-dimensional numpy array")

        if xtics is not None and len(xtics) != len(xs):
            raise ValueError("xtics must have the same length as xs")

        if xlabels is not None and len(xlabels) != len(xs):
            raise ValueError("xlabels must have the same length as xs")

    # --- Time axis ------------------------------------------------------

    length = n if n is not None else len(y)
    time = np.arange(1, length + 1)

    # --- Axes setup -----------------------------------------------------

    standalone = ax is None
    if standalone:
        fig, ax = plt.subplots(figsize=(10, 5))

    # --- Plot primary series -------------------------------------------

    ax.plot(time, y, label=ytitle or "fitted", linewidth=2)

    # --- Plot auxiliary series -----------------------------------------

    if xs is not None:
        for i, x in enumerate(xs):
            label = xlabels[i] if xlabels is not None else f"x[{i}]"
            ax.plot(time[: len(x)], x, linestyle="--", alpha=0.8, label=label)

    # --- Labels and title ----------------------------------------------

    ax.set_xlabel(xlabel or "Time")
    ax.set_ylabel(ylabel or "Value")

    if title:
        ax.set_title(title)

    # --- Legend and grid -----------------------------------------------

    if ax.get_legend_handles_labels()[0]:
        ax.legend()

    ax.grid(True)

    # --- X-ticks handling ----------------------------------------------

    if xtics is not None:
        labels = xtics[0]
        step = max(1, len(labels) // max_xtics)
        tick_positions = time[::step]
        tick_labels = [labels[i] for i in range(0, len(labels), step)]
        ax.set_xticks(tick_positions)
        ax.set_xticklabels(tick_labels, rotation=45)
    else:
        ax.set_xticks(time)

    # --- Finalize -------------------------------------------------------

    if standalone:
        plt.tight_layout()
        plt.show(block=True)

if __name__ == "__main__":
    pass
