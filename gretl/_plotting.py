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
