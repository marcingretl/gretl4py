import _gretl
import numpy
import pandas as pd
import json

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
                    s = pd.Series(val['data'])
                    s = pd.to_numeric(s, errors='coerce')
                    json_dict[key] = s
                elif val['type'] == 'gretl_list':
                    json_dict[key] = list(val['data'])[1:]

    def fit(self):
        if self.cpython_function and self.computed is False:
            model_dict = self.cpython_function(*self.args, **self.options)
            json_dict = json.loads(model_dict['json_buffer'])
            
            # Define summary property used for __repr__
            self.summary = model_dict['summary']
            del json_dict['type']
            self.__process_gretl_objects(json_dict)
            model_dict['results'] = json_dict
            del model_dict['json_buffer']
            self.__load_dict_model(model_dict)
            self._id = model_dict['_id']
            del model_dict['_id']
            
        self.computed = True
        return self

    def id(self):
        return self._id
    
    def coeff(self):
        return self.results['coeff'][:, 0]
    
    def stderr(self):
        return self.results['stderr'][:, 0]
    
    def yhat(self):
        return self.results['yhat']
    
    def rsq(self):
        return self.results['rsq']

    # statsmodels like attributes: properties
    params = property(fget=coeff, fset=None, fdel=None)
    bse = property(fget=stderr, fset=None, fdel=None)
    rsquared = property(fget=rsq, fset=None, fdel=None)
    fittedvalues = property(fget=yhat, fset=None, fdel=None)

if __name__ == "__main__":
    pass
