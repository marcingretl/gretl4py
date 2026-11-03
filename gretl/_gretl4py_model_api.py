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

from __future__ import annotations
from typing import TYPE_CHECKING
from ._gretl4py_classes import _adf, _bds, _coint, _corrgm, _difftest, _fractint
from ._gretl4py_classes import _johansen, _kpss, _levinlin, _meantest, _normtest
from ._gretl4py_classes import _runs, _vartest, _add, _bkw, _chow, _cusum, _leverage
from ._gretl4py_classes import _omit, _qlrtest, _reset, _white, _vif, _normality
from ._gretl4py_classes import _nonlinearity, _autocorr, _arch, _breusch_pagan
from ._gretl4py_classes import _groupwise_heteroskedasticity, _panspec, _comfac, _xdepend, _coeffsum

if TYPE_CHECKING:
    from matplotlib.figure import Figure

"""
Configure estimator-specific methods for gretl model classes.

Methods are attached only to estimator classes for which the corresponding
gretl command is potentially available. Restrictions that depend on the
dataset or on properties of an individual fitted model are checked at
run time by libgretl.
"""

from ._gretl4py_classes import (
    _add,
    _arch,
    _autocorr,
    _bkw,
    _breusch_pagan,
    _chow,
    _coeffsum,
    _comfac,
    _cusum,
    _groupwise_heteroskedasticity,
    _leverage,
    _nonlinearity,
    _normality,
    _omit,
    _panspec,
    _qlrtest,
    _reset,
    _vif,
    _white,
    _xdepend,
)


# ------------------------------------------------------------------
# Model plotting proxy classes
# ------------------------------------------------------------------
# Named plotting methods delegate to the native _plot() bindings.
# VECM extends the VAR interface with error-correction plots.

class _NSEPlots:
    """Plotting interface for single-equation models."""

    __slots__ = ("_model",)

    def __init__(self, model):
        self._model = model

    def residuals(self, xvar: str = "") -> Figure | None:
        """Plot residuals, optionally against a named variable."""
        return self._model._plot("residuals", xvar=xvar)

    def fitted_actual(self, xvar: str = "") -> Figure | None:
        """Plot observed and fitted values, optionally against a variable."""
        return self._model._plot("fitted", xvar=xvar)



class _VARPlots:
    """Plotting interface for VAR models."""

    __slots__ = ("_model",)

    def __init__(self, model):
        self._model = model

    def residuals(self, mode: str = "combined", equation: int = 0) -> Figure | None:
        """Plot VAR residuals, optionally selecting a layout or equation.

        Parameters
        ----------
        mode : {"combined", "multiple"}, default "combined"
            Use "multiple" for a grid of separate equation plots.
        equation : int, default 0
            Equation number; 0 requests the combined residual plot.
        """
        return self._model._plot("residuals", equation=equation, mode=mode)

    def inverse_roots(self) -> Figure | None:
        """Plot inverse roots of the VAR characteristic polynomial."""
        return self._model._plot("inverse_roots")

    def irf_combined(
        self,
        periods: int | None = None,
        alpha: float = 0.0,
        error_bars: bool = False,
    ) -> Figure | None:
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

    def ec_plot(self) -> Figure | None:
        """Plot the VECM error-correction terms (requires the _plot_ec binding)."""
        # This native method belongs to Model_vecm only, not Model_var.
        return self._model._plot_ec()


def setup_model_api(_gretl):
    """
    Attach methods to the appropriate model classes.

    Estimator-level availability generally follows libgretl's
    command_ok_for_model() logic. The Python API is restricted where
    appropriate to the estimators for which a test is intended.

    Restrictions that depend on the dataset or on properties of an
    individual fitted model are left to libgretl's run-time checks.
    """

    def attach(classes, name):
        """Attach the wrapper named ``_<name>`` to each class."""
        method = globals()[f"_{name}"]

        for cls in classes:
            setattr(cls, name, method)

    # ------------------------------------------------------------------
    # Single-equation model groups
    # ------------------------------------------------------------------

    list_models = (
        _gretl.Model_arima,
        _gretl.Model_ols,
        _gretl.Model_wls,
        _gretl.Model_logit,
        _gretl.Model_probit,
        _gretl.Model_poisson,
        _gretl.Model_negbin,
        _gretl.Model_duration,
        _gretl.Model_heckit,
        _gretl.Model_logistic,
        _gretl.Model_ar,
        _gretl.Model_ar1,
        _gretl.Model_lad,
        _gretl.Model_garch,
        _gretl.Model_mpols,
        _gretl.Model_panel,
        _gretl.Model_ivreg,
        _gretl.Model_hsk,
        _gretl.Model_intreg,
        _gretl.Model_midasreg,
        _gretl.Model_quantreg,
        _gretl.Model_tobit,
        _gretl.Model_dpanel,
    )

    nonlist_models = (
        _gretl.Model_nls,
        _gretl.Model_mle,
        _gretl.Model_gmm,
    )

    # Model_biprobit is deliberately excluded from the groups above:
    # libgretl permits only RESTRICT for this estimator.

    # MIDASREG is represented by a regular model class in gretl4py but
    # command_ok_for_model() treats it as NLS for most command checks.
    # It therefore requires special treatment below.

    # ------------------------------------------------------------------
    # Broadly available tests
    # ------------------------------------------------------------------

    # Belsley-Kuh-Welsch diagnostics.
    #
    # BKW is handled before the NONLIST_MODEL check in libgretl, so it
    # may also be available for NLS, MLE, GMM and MIDASREG. Requirements
    # concerning the number of coefficients and availability of VCV/X'X
    # are checked at run time.
    attach(
        list_models + nonlist_models,
        "bkw",
    )

    # ARCH test.
    #
    # GARCH and MIDASREG are excluded at the estimator level.
    # Requirements concerning time-series data and missing observations
    # are checked at run time.
    attach(
        tuple(
            cls for cls in list_models
            if cls not in (
                _gretl.Model_garch,
                _gretl.Model_midasreg,
            )
        ),
        "arch",
    )

    # Coefficient-sum test.
    #
    # MIDASREG is excluded because command_ok_for_model() treats it as
    # NLS. Requirements concerning the number of coefficients are
    # checked at run time.
    attach(
        tuple(
            cls for cls in list_models
            if cls is not _gretl.Model_midasreg
        ),
        "coeffsum",
    )

    # Residual normality.
    #
    # LOGIT, HECKIT and DURATION are excluded. Among non-list models,
    # NLS and GMM are supported, while MLE is excluded. MIDASREG is
    # supported because it is treated as NLS for this check.
    attach(
        tuple(
            cls for cls in list_models
            if cls not in (
                _gretl.Model_logit,
                _gretl.Model_heckit,
                _gretl.Model_duration,
            )
        ) + (
            _gretl.Model_nls,
            _gretl.Model_gmm,
        ),
        "normality",
    )

    # ------------------------------------------------------------------
    # Model modification
    # ------------------------------------------------------------------

    # ADD.
    #
    # ARMA, GARCH, HECKIT, INTREG and MIDASREG are excluded.
    # Restrictions concerning particular PANEL specifications and
    # command options are checked at run time.
    attach(
        tuple(
            cls for cls in list_models
            if cls not in (
                _gretl.Model_arima,
                _gretl.Model_garch,
                _gretl.Model_heckit,
                _gretl.Model_intreg,
                _gretl.Model_midasreg,
            )
        ),
        "add",
    )

    # OMIT.
    #
    # ARMA, GARCH, INTREG, MIDASREG and DPANEL are excluded.
    # Restrictions concerning particular PANEL specifications and
    # command options are checked at run time.
    attach(
        tuple(
            cls for cls in list_models
            if cls not in (
                _gretl.Model_arima,
                _gretl.Model_garch,
                _gretl.Model_intreg,
                _gretl.Model_midasreg,
                _gretl.Model_dpanel,
            )
        ),
        "omit",
    )

    # ------------------------------------------------------------------
    # Estimator-specific tests
    # ------------------------------------------------------------------

    # Serial correlation.
    #
    # Available for OLS, IVREG, ARMA and PANEL models. Requirements
    # concerning time-series or panel data, missing observations and
    # particular panel specifications are checked at run time.
    attach(
        (
            _gretl.Model_ols,
            _gretl.Model_ivreg,
            _gretl.Model_arima,
            _gretl.Model_panel,
        ),
        "autocorr",
    )

    # Common-factor restriction.
    #
    # Available for AR(1) models only. Further restrictions concerning
    # the particular AR(1) specification are checked at run time.
    attach(
        (_gretl.Model_ar1,),
        "comfac",
    )

    # OLS-only tests.
    #
    # Some of these tests impose additional requirements on the dataset:
    # for example CUSUM and QLR require time-series data, while PANSPEC
    # requires panel data. These conditions are checked at run time.
    for name in (
        "breusch_pagan",
        "chow",
        "cusum",
        "leverage",
        "nonlinearity",
        "panspec",
        "qlrtest",
        "reset",
    ):
        attach(
            (_gretl.Model_ols,),
            name,
        )

    # Groupwise heteroskedasticity.
    #
    # Available following OLS or PANEL estimation. In the OLS case the
    # model must have been estimated on panel data. Dataset and model
    # restrictions are checked at run time.
    attach(
        (
            _gretl.Model_ols,
            _gretl.Model_panel,
        ),
        "groupwise_heteroskedasticity",
    )

    # White test.
    #
    # Available for OLS and IVREG. For IVREG only the standard White
    # variant is available; white_nocross=True is rejected by libgretl.
    attach(
        (
            _gretl.Model_ols,
            _gretl.Model_ivreg,
        ),
        "white",
    )

    # Variance inflation factors.
    #
    # IVREG, ARMA, GARCH, PANEL and DPANEL are excluded. MIDASREG is
    # treated as NLS by command_ok_for_model() and is therefore also
    # excluded. The requirement for at least two independent variables
    # is checked at run time.
    attach(
        tuple(
            cls for cls in list_models
            if cls not in (
                _gretl.Model_ivreg,
                _gretl.Model_arima,
                _gretl.Model_garch,
                _gretl.Model_panel,
                _gretl.Model_dpanel,
                _gretl.Model_midasreg,
            )
        ),
        "vif",
    )

    # Cross-sectional dependence.
    #
    # Exposed for OLS and PANEL models. An OLS model may have been
    # estimated on panel data (pooled OLS), so panel-data status cannot
    # be inferred from the estimator class alone. The requirement for
    # panel data and restrictions concerning particular panel
    # specifications are checked at run time.
    attach(
        (
            _gretl.Model_ols,
            _gretl.Model_panel,
        ),
        "xdepend",
    )

    # ------------------------------------------------------------------
    # VAR/VECM tests
    # ------------------------------------------------------------------

    # Model_vecm inherits from Model_var, so attaching these methods to
    # Model_var makes them available for both VAR and VECM models.
    for name in (
        "arch",
        "autocorr",
        "normality",
    ):
        attach(
            (_gretl.Model_var,),
            name,
        )

    # ------------------------------------------------------------------
    # Model plotting interfaces
    # ------------------------------------------------------------------

    # All single-equation model subclasses inherit the NSE plotting methods.
    _gretl.Model_NSE.plot = property(
        _NSEPlots, doc="Named plotting methods for a single-equation model."
    )

    # VAR models expose their own residual, root and IRF plots.
    _gretl.Model_var.plot = property(
        _VARPlots, doc="Named plotting methods for a VAR model."
    )

    # Override the inherited VAR property with the VECM-specific proxy.
    _gretl.Model_vecm.plot = property(
        _VECMPlots, doc="Named plotting methods for a VECM model."
    )
