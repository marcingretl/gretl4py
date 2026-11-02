"""
 *
 *  gretl -- Gnu Regression, Econometrics and Time-series Library
 *  Copyright (C) 2001 Allin Cottrell and Riccardo "Jack" Lucchetti
 *
 *  gretl4py -- Python bindings for gretl
 *  Copyright (C) 2024 Marcin Błażejowski
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 """

"""Model-specific plotting interfaces for gretl4py.

These proxy classes expose named plotting methods and delegate the work to
native libgretl bindings. Every plotting method returns the object produced
by the underlying binding (normally a matplotlib Figure).
"""


class _NSEPlots:
    """Plotting interface for single-equation models."""

    __slots__ = ("_model",)

    def __init__(self, model):
        self._model = model

    def residuals(self, xvar: str = ""):
        """Plot residuals, optionally against a named variable."""
        return self._model._plot("residuals", xvar=xvar)

    def fitted_actual(self, xvar: str = ""):
        """Plot observed and fitted values, optionally against a variable."""
        return self._model._plot("fitted_actual", xvar=xvar)

    def qqplot(self):
        """Plot the residual normal Q-Q graph."""
        return self._model._plot("qqplot")

    def correlogram(self):
        """Plot the residual correlogram."""
        return self._model._plot("correlogram")

    def periodogram(self):
        """Plot the residual periodogram."""
        return self._model._plot("periodogram")


class _VARPlots:
    """Plotting interface for VAR models."""

    __slots__ = ("_model",)

    def __init__(self, model):
        self._model = model

    def residuals(self, equation: int = 0, mode: str = "combined"):
        """Plot VAR residuals, optionally selecting an equation or layout.

        Parameters
        ----------
        equation : int, default 0
            Equation number; 0 requests the combined residual plot.
        mode : {"combined", "multiple"}, default "combined"
            Use "multiple" for a grid of separate equation plots.
        """
        return self._model._plot("residuals", equation=equation, mode=mode)

    def inverse_roots(self):
        """Plot inverse roots of the VAR characteristic polynomial."""
        return self._model._plot("inverse_roots")

    def irf_combined(
        self,
        periods: int | None = None,
        alpha: float = 0.0,
        error_bars: bool = False,
    ):
        """Plot the grid of impulse responses for all equation pairs.

        Parameters
        ----------
        periods : int or None, default None
            IRF horizon; None uses the libgretl default horizon.
        alpha : float, default 0.0
            Bootstrap confidence-interval tail probability; 0 omits the
            intervals (for example, 0.10 requests 90% intervals).
        error_bars : bool, default False
            Show error bars instead of shaded confidence intervals.
        """
        return self._model._plot(
            "irf_combined",
            periods=periods,
            alpha=alpha,
            error_bars=error_bars,
        )


class _VECMPlots(_VARPlots):
    """VAR plots extended with VECM-specific error-correction plots."""

    def ec_plot(self):
        """Plot the VECM error-correction terms (requires the _plot_ec binding)."""
        # This native method belongs to Model_vecm only, not Model_var.
        return self._model._plot_ec()


def _install_plot_proxies(Model_NSE, Model_var, Model_vecm):
    """Install read-only ``model.plot`` properties on pybind11 model types.

    Call this once from the package initializer, after importing the native
    model classes from ``._gretl``. More specific properties override the
    inherited property for VECM, while NSE subclasses inherit NSE plotting.
    """
    Model_NSE.plot = property(
        _NSEPlots, doc="Named plotting methods for a single-equation model."
    )
    Model_var.plot = property(
        _VARPlots, doc="Named plotting methods for a VAR model."
    )
    Model_vecm.plot = property(
        _VECMPlots, doc="Named plotting methods for a VECM model."
    )
