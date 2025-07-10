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
import sys
from . import _gretl

# #######################################################
#   GretlModel class
# #######################################################
def attrs(self):
    for attname in dir(self):
        if not attname.startswith("_"):
            attr = getattr(self, attname)
            if isinstance(attr, float):
                print(f'{attname}: {attr:.8g}')
            elif isinstance(attr, int):
                print(f'{attname}: {attr}')
            elif isinstance(attr, str) and len(attr) < 32:
                print(f'{attname}: {attr}')
            else:
                print(f'{attname}: {type(attr)}')

def oddsratio(self):
    try:
        from tabulate import tabulate
    except ModuleNotFoundError:
        print('*** warning: oddsratios printing requires \'tabulate\' package ***\n')
        return

    try:
        data = self.oddsratios      # assumed to be a NumPy array
        names = self.parnames[1:]   # skip constant/intercept
        print(tabulate(data, showindex=names,
                       headers=["odds ratio", "std. error", "low95", "high95"],
                       tablefmt="plain"))
    except Exception as e:
        print('*** warning: oddsratios not available for current model ***\n')
        print(f'Debug info: {e}')


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
    dset = _gretl.Dataset(src)
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

    datasets_found = [name for name, value in all_globals.items() if isinstance(value, _gretl.Dataset)]

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
