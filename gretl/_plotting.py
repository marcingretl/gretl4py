_plt = None

def _setup_matplotlib():
    global _plt
    
    if _plt is not None:
        return _plt

    import sys
    import matplotlib

    try:
        if sys.platform == "darwin":
            matplotlib.use("MacOSX")
        elif sys.platform.startswith("win"):
            matplotlib.use("TkAgg")
        else:
            matplotlib.use("TkAgg")
    except Exception:
        matplotlib.use("Agg")

    import matplotlib.pyplot as plt
    
    _plt = plt
    
    return plt
