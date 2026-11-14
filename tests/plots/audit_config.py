"""Local directories for the gretl package plotting audit.

Change PROJECT_ROOT here if the audit project is moved elsewhere.
"""

from pathlib import Path

# Main directory containing this audit toolkit.
PROJECT_ROOT = Path(
    "/home/marcin/cvs/Gretl/gretl-gretl4py/tests/plots/gretl_package_plot_audit"
)

# Downloaded contributed packages are installed from these cached files.
AUDIT_ROOT = PROJECT_ROOT / "audit"
CONTRIBUTED_DIR = AUDIT_ROOT / "contributed"
RESULTS_ROOT = PROJECT_ROOT / "package_plot_results"
