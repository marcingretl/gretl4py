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

import json
import _gretl
import numpy

class Model:

    def __init__(self, name, *args, **kwargs):
        self.name = name
        try:
            self.cpython_function = getattr(_gretl, name)
        except Exception as err:
            print(f"Unexpected {err=}, {type(err)=}")
            raise
        self.args = args
        self.options = kwargs
        self.computed = False
        self._id = None
        self.results = {}
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

    def __process_gretl_objects(self, json_dict):
        for key, val in json_dict.items():
            if isinstance(val, dict):
                if not 'type' in val:
                    continue
                elif val['type'] == 'gretl_matrix':
                    a = numpy.asarray(val['data'])
                    r = val['rows']
                    c = val['cols']
                    json_dict[key] = numpy.reshape(a, [r, c], order = 'F')
                elif val['type'] == 'gretl_series':
                    json_dict[key] = val['data']
                elif val['type'] == 'gretl_list':
                    json_dict[key] = list(val['data'])[1:]

    def fit(self):
        if self.cpython_function and self.computed is False:
            model_dict = self.cpython_function(*self.args, **self.options)
            json_dict = json.loads(model_dict['json_buffer'])
            self.summary = model_dict['summary'] # Define summary property used for __repr__
            del json_dict['type']
            self.__process_gretl_objects(json_dict)
            model_dict['results'] = json_dict
            del model_dict['json_buffer']
            self.__load_dict_model(model_dict)
            self._id = model_dict['_id']
            del model_dict['_id']
        self.computed = True
        return self

    def coeff(self):
        return self.results['coeff'][:, 0]

    def stderr(self):
        return self.results['stderr'][:, 0]

    def yhat(self):
        return self.results['yhat']

    def uhat(self):
        return self.results['uhat']

    # strings
    def parnames(self):
        return self.results['parnames']

    #scalars
    def id(self):
        return self._id

    def rsq(self):
        return self.results['rsq']

    def sigma(self):
        return self.results['sigma']

    def aic(self):
        return self.results['aic']

    def bic(self):
        return self.results['bic']

    def hqc(self):
        return self.results['hqc']


if __name__ == "__main__":
    pass
