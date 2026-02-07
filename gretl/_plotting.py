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
    x: np.ndarray | None = None,
    n: int | None = None,
    xtics: np.ndarray | list | None = None,
    ytics: str | None = None,
    xlabel: str | None = None,
    ylabel: str | None = None,
    xtitle: str | None = None,
    ytitle: str | None = None,
    title: str | None = None,
    max_xtics: int = 10,
    ax=None,
):
    """
    Time-series plot of fitted values, optionally overlayed with actual values.

    This function creates a line plot of fitted values `y` versus time and,
    optionally, the actual observed values `x`. It handles automatic x-axis
    labels (`xtics`), adjusts tick density for dense time series, and supports
    both single plots and subplots if an Axes object is provided via `ax`.

    Parameters
    ----------
    y : np.ndarray
        Fitted values to plot (mandatory).
    x : np.ndarray, optional
        Actual observed values to plot alongside fitted values. Default is None.
    n : int, optional
        Length of the time series, used if `xtics` is not provided.
        If None, defaults to `len(y)`.
    xtics : np.ndarray or list, optional
        Custom labels for the x-axis (e.g., dates or observation numbers).
        If None, integers 1..n are used.
    ytics : str, optional
        Fallback label for the y-axis if `ylabel` is not provided.
    xlabel : str, optional
        Label for the x-axis. Defaults to `"Time"`.
    ylabel : str, optional
        Label for the y-axis. Overrides `ytics` if provided.
    xtitle : str, optional
        Label for the actual series (used in the legend if `x` is provided).
    ytitle : str, optional
        Label for the fitted series (used in the legend).
    title : str, optional
        Main title for the plot.
    max_xtics : int, optional
        Maximum number of x-axis ticks to display. Automatically adjusts
        tick positions for dense time series. Default is 10.
    ax : matplotlib.axes.Axes, optional
        Axes object on which to plot. If None, a new figure and axes are created.
        This allows integration with subplots.

    Notes
    -----
    - If `ax` is None, a new figure is created and `plt.show()` is called
      immediately, blocking script execution until the window is closed.
    - If `ax` is provided (subplot scenario), no figure is shown immediately;
      call `plt.show()` manually after plotting all subplots.
    - Handles automatic rotation of x-axis tick labels for readability.
    - Works for both interactive sessions (Jupyter, IPython) and scripts.

    Example
    -------
    >>> _time_series_plot(y=fitted, x=actual, n=50, ylabel="GDP", title="Fitted vs Actual GDP")
    >>> fig, ax = plt.subplots()
    >>> _time_series_plot(y=fitted, x=actual, ax=ax, title="Single subplot example")
    """

    from ._plotting import _setup_matplotlib
    plt = _setup_matplotlib()

    # Determine x-axis (time)
    if xtics is not None:
        time = np.arange(len(xtics))
        labels = xtics
    else:
        length = n if n is not None else len(y)
        time = np.arange(1, length + 1)
        labels = None

    if ax is None:
        fig, ax = plt.subplots(figsize=(10, 5))

    # Plot fitted values
    ax.plot(time, y, label=ytitle or "fitted", color="blue")

    # Plot actual values if provided
    if x is not None:
        ax.plot(time, x, label=xtitle or "actual", color="orange")

    # Set axis labels
    ax.set_xlabel(xlabel or "Time")
    ax.set_ylabel(ylabel or ytics or "Value")

    # Set plot title
    if title:
        ax.set_title(title)

    # Legend if any labels exist
    handles, _ = ax.get_legend_handles_labels()
    if handles:
        ax.legend()

    # Grid
    ax.grid(True)

    # Adjust x-ticks for dense data
    if labels is not None:
        step = max(1, len(labels) // max_xtics)
        tick_positions = time[::step]
        tick_labels = [labels[i] for i in range(0, len(labels), step)]
        ax.set_xticks(tick_positions)
        ax.set_xticklabels(tick_labels, rotation=45)
    else:
        ax.set_xticks(time)

    # Show figure if standalone
    if ax is None:
        plt.tight_layout()
        plt.show(block=True)

if __name__ == "__main__":
    pass
