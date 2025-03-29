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
        self.summary = {}

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
            self._id = model_dict['_id']
            del model_dict['_id']
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
            exit(f'*** error: oddsratios not available for {self.name:s} estimator ***\n')
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
        try:
            frame = inspect.currentframe()
            frame = inspect.getouterframes(frame)[1]
            string = inspect.getframeinfo(frame[0]).code_context[0].strip()

            # extract variable name from the function call
            args = string[string.find('(') + 1:-1].split(',')
            assert len(args) == 1

            if '=' in args[0]:
                source = args[0].split('=')[1].strip()
            else:
                source = args[0].strip()

        except Exception:
            pass  # don't immediately fall back, but try scanning variables

        # If name extraction failed, search for the dictionary in caller's scope
        if not source:
            caller_globals = frame[0].f_globals
            caller_locals = frame[0].f_locals

            # find the variable in locals or globals
            for var_name, var_value in {**caller_globals, **caller_locals}.items():
                if var_value is src:  # check if value matches our dictionary
                    source = var_name
                    break

        if not source:
            source = "unknown"  # fallback if we couldn't find any reference

        source += " (dict)"
    else:
        raise RuntimeError(f'Unsupported data src type: {type(src).__name__}\n')

    # Create dataset
    dset = _gretl.GretlDataset(src)
    dset.source = source

    # if os.getenv("GRETL4PY_VERBOSE", "1") == "1":
    # print(f'Dataset loaded from: {dset.source}\n')

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
            print(f'{d}: {dset.get_id():d}, {dset.source:s} {defaut:s}')

if __name__ == "__main__":
    pass
