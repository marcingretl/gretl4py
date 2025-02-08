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

    #scalars
    def id(self):
        return self._id

if __name__ == "__main__":
    pass
