"""
 *
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 """
import inspect
import os
import _gretl
import sys
from gretl.model_registry import register_model

class Model:
    def __init__(self, name, *args, **kwargs):
        self.name = name
        self.args = args
        self.options = kwargs
        self._model = None

        try:
            self.cpython_function = getattr(_gretl, name)
        except AttributeError as err:
            raise RuntimeError(f"Model type '{name}' not found in _gretl module") from err

        # Actually create the C++ model instance
        try:
            self._model = self.cpython_function(*self.args, **self.options)
        except Exception as err:
            raise RuntimeError(f"Failed to instantiate model '{name}': {err}") from err

        # Register the model (if needed)
        register_model(self)

    def __getattr__(self, name):
        if name.startswith('__') and name.endswith('__'):
            raise AttributeError(f"'{type(self).__name__}' object has no attribute '{name}'")

        if self._model is not None and hasattr(self._model, name):
            return getattr(self._model, name)

        raise AttributeError(f"'{type(self).__name__}' object has no attribute '{name}'")

    def __setattr__(self, name, value):
        if name in ("name", "args", "options", "_model", "cpython_function"):
            super().__setattr__(name, value)
        elif "_model" in self.__dict__ and self._model is not None and hasattr(self._model, "__dict__"):
            setattr(self._model, name, value)
        else:
            super().__setattr__(name, value)

    def __repr__(self):
        if self._model is not None and hasattr(self._model, 'summary'):
            return str(self._model.summary)
        return f"\nModel {self.name.upper()}"

    def fit2(self):
        if self._model is None:
            self._model = self.cpython_function(*self.args, **self.options)
        return self._model  # Return the internal model, not self

    def id(self):
        if self._model is not None:
            return getattr(self._model, '_id', None)
        return None

    # Keep or adapt these as necessary
    #def attrs(self): ...
    #def fcast(self, *args, **kwargs): ...
    #def get_accessor(self, accessor): ...
    #def restrict(self, impose, *args, **kwargs): ...
    #def test(self, type, vars=None, obs=None, *args, **kwargs): ...
    #def oddsratio(self): ...



# #######################################################
#   GretlDataset class
# #######################################################
def get_data(src):
    """Load dataset from file path or dictionary.

    Parameters:
    src (str | dict): Either a file path or a dictionary.

    Returns:
    GretlDataset: A dataset object.
    """
    if isinstance(src, str):
        source = src  # file path case
    elif isinstance(src, dict):
        source = None
        frame = inspect.currentframe()
        try:
            outer_frame = inspect.getouterframes(frame)[1]
            caller_frame = outer_frame[0]  # extract caller's frame
            string = inspect.getframeinfo(caller_frame).code_context[0].strip()

            # extract variable name from the function call
            args = string[string.find('(') + 1:-1].split(',')
            if len(args) == 1:
                source = args[0].split('=')[1].strip() if '=' in args[0] else args[0].strip()
        except Exception:
            pass  # if direct extraction fails, scan variables
        finally:
            del frame  # cleanup to prevent reference cycles

        # try to find the variable in caller's scope if name extraction failed
        if not source:
            caller_globals, caller_locals = caller_frame.f_globals, caller_frame.f_locals
            source = next((var for var, val in {**caller_globals, **caller_locals}.items() if val is src), "unknown")

        source += " (dict)"
    else:
        raise RuntimeError(f'Unsupported data src type: {type(src).__name__}\n')

    # create dataset
    dset = _gretl.GretlDataset(src)
    if dset.get_id() == 1:
        dset.set_as_default()
    dset.source = source

    return dset

def show_data_status():
    """List of available datasets."""

    # get the caller's global and local variables
    caller_globals = sys._getframe(1).f_globals
    caller_locals = sys._getframe(1).f_locals

    # combine both to search for datasets
    all_globals = {**caller_globals, **caller_locals}

    datasets_found = [name for name, value in all_globals.items() if isinstance(value, _gretl.GretlDataset)]

    if len(datasets_found) == 0:
        print('No datasets were loaded.')
    else:
        print('Available datasets (id, source):')
        for d in datasets_found:
            dset = all_globals[d]
            if dset.is_default is True:
                defaut = "(default)"
            else:
                defaut = ""
            print(f'{d}: {dset.get_id():d}, {dset.source:s} {defaut:s} - referenced by models: {list(dset.linked_models_list())}')

if __name__ == "__main__":
    pass
