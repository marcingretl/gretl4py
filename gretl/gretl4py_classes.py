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
        try:
            self.cpython_function = getattr(_gretl, name)
        except Exception as err:
            print(f'Unexpected {err=}, {type(err)=}')
            raise
        self.args = args
        self.options = kwargs
        self.computed = False
        self._id = None
        self._model_id = None
        self.summary = {}
        register_model(self)  # track the wrapper instance

    def __del__(self):
        try:
            if self._model_id is not None:
                _gretl.GretlModel_destructor_wrapper(self._model_id)
        except TypeError:
            pass

    def __repr__(self):
        if self.computed:
            out = self.summary
        else:
            out = '\n'
            out += "Model " + self.name.upper()
        return out

    def __load_dict_model(self, model_dict):
        if model_dict:
            for _ in model_dict:
                setattr(self, _, model_dict[_])

    def fit(self):
        if self.cpython_function and self.computed is False:
            model_dict = self.cpython_function(*self.args, **self.options)
            self.summary = model_dict['summary'] # Define summary property used for __repr__
            self.__load_dict_model(model_dict)
            self._id = model_dict['_ids']['_id']
            self._model_id = model_dict['_ids']['_model_id']
            del model_dict['_ids']
        self.computed = True
        return self

    def attrs(self):
        for attname in dir(self):
            if not attname.startswith("_"):
                attr = getattr(self, attname)
                if isinstance(attr, float):
                    print(f"{attname}: {attr:.8g}")
                elif isinstance(attr, int):
                    print(f"{attname}: {attr}")
                elif isinstance(attr, str) and len(attr) < 32:
                    print(f"{attname}: {attr}")
                else:
                    print(f"{attname}: {type(attr)}")

    def oddsratio(self):
        if self.name != 'logit':
            raise RuntimeError(f'*** error: oddsratios not available for {self.name:s} estimator ***\n')
        else:
            try:
                from tabulate import tabulate
                print(tabulate(self.oddsratios, showindex=self.parnames[1:],
                               headers=['odds ratio', 'std. error', 'low95', 'high95'],
                               tablefmt="plain"))
            except KeyError:
                print('*** warning: oddsratios not available for current model ***\n')
            except ModuleNotFoundError:
                print('*** warning: oddsratios printing requires \'tabulate\' package ***\n')

    def fcast(self, *args, **kwargs):
        if self.computed and self._model_id is not None:
            _gretl.fcast(self._model_id, *args, **kwargs)

    def get_accessor(self, accessor):
        if self.computed and self._model_id is not None:
            return _gretl.get_accessor(self._model_id, accessor)

    def restrict(self, impose, *args, **kwargs):
        if self.computed and self._model_id is not None:
            res = _gretl.restrict(self._model_id, impose, *args, **kwargs)
            setattr(self, "restrict", res)

    def test(self, type, vars=None, obs=None, *args, **kwargs):
        if self.computed and self._model_id is not None:
            if vars is not None and obs is not None:
                raise RuntimeError("you cannot use 'vars' and 'obs' together")
            elif vars is not None:
                res = _gretl.test(self._model_id, type, vars, *args, **kwargs)
            elif obs is not None:
                res = _gretl.test(self._model_id, type, obs, *args, **kwargs)
            else:
                res = _gretl.test(self._model_id, type, *args, **kwargs)
            setattr(self, type, res)

    # scalars
    def id(self):
        return self._id


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
