import numpy as np
import gretl


def printhead(title, full=True):
    line = "#" * 64
    if full:
        print(f"\n{line}\n***\t{title}\t***\n{line}")
    else:
        print(f"\n***\t{title}\t***")


def linenr(nr):
    print(f"\n[{nr}]:")


def runtime(t):
    print(f"\n*** total execution time: {t:.6f} seconds ***\n")


def summary_res(res_gretl: gretl.Model, res_sm: np.ndarray, const_pos: str = "first"):
    rows = res_gretl.coeff.rows

    if rows != len(res_sm.params):
        raise RuntimeError("models are not conformable")

    if const_pos not in ("first", "last"):
        raise RuntimeError("const_pos takes 'first' or 'last' only")

    res_coeff = gretl.Matrix(rows, 2)
    res_stderr = gretl.Matrix(rows, 2)

    # --- gretl side (always column vector -> flatten safely)
    g_coeff = res_gretl.coeff.numpy_view[:, 0].ravel()
    g_stderr = res_gretl.stderr.numpy_view[:, 0].ravel()

    # assign gretl (right column)
    res_coeff.numpy_view[:, 1] = g_coeff
    res_stderr.numpy_view[:, 1] = g_stderr

    # --- statsmodels side
    sm_params = np.asarray(res_sm.params).ravel()
    sm_bse = np.asarray(res_sm.bse).ravel()

    if const_pos == "last":
        sm_params = np.concatenate(([sm_params[-1]], sm_params[:-1]))
        sm_bse = np.concatenate(([sm_bse[-1]], sm_bse[:-1]))

    # assign statsmodels (left column)
    res_coeff.numpy_view[:, 0] = sm_params
    res_stderr.numpy_view[:, 0] = sm_bse

    res_coeff.rnameset(res_gretl.parnames)
    res_stderr.rnameset(res_gretl.parnames)
    res_coeff.cnameset(["statsmodels", "gretl4py"])
    res_stderr.cnameset(["statsmodels", "gretl4py"])

    print("coefficients:")
    print(res_coeff)
    print("std. errors:")
    print(res_stderr)

def runtime_ratio(sm_time: float, gr_time: float):
    if sm_time <= 0.0:
        raise RuntimeError("invalid statsmodels runtime (must be > 0)")

    ratio = gr_time / sm_time
    print(f"gretl4py / statsmodels runtime ratio = {ratio:.6f}\n")

import time

def benchmark_fit(fn, repeat=100):
    # warm-up
    fn()

    t0 = time.perf_counter()
    for _ in range(repeat):
        res = fn()
    t1 = time.perf_counter()

    return (t1 - t0) / repeat
