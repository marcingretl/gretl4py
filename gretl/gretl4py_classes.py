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

import _gretl

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
def get_data(source):
    """source: either path to the file or dict"""
    if isinstance(source, str) is True:
        name = source
    elif isinstance(source, dict) is True:
        import inspect

        frame = inspect.currentframe()
        frame = inspect.getouterframes(frame)[1]
        string = inspect.getframeinfo(frame[0]).code_context[0].strip()
        args = string[string.find('(') + 1:-1].split(',')
        assert len(args) == 1
        if args[0].find('=') != -1:
            name = args[0].split('=')[1].strip()
        else:
            name = args[0]
        name += " (dict)"
    else:
        raise RuntimeError(f'{str(type(source)):s} type is not supported\n')

    dset = _gretl.GretlDataset(source)
    dset.set_name(name)
    print(f'Dataset loaded from: {dset.get_name():s}\n') # this should be controlled by GRETL4PY_VERBOSE env

    return dset


if __name__ == "__main__":
    pass
