import _gretl # by I'd like to have 'import gretl' here
import numpy as np
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

	def __repr__(self):
		if self.computed:
			out = self.summary
		else:
			out = '\n'
			out += "Model " + self.name.upper()
		return out

	def __load_dict_model(self, dict_model):
		if dict_model:
			for _ in dict_model: setattr(self, _, dict_model[_])

	def __process_gretl_objects(self, d):
		for key, val in d.items():
			if isinstance(val, dict):
				if val['type'] == 'gretl_matrix':
					a = np.asarray(val['data'])
					r = val['rows']
					c = val['cols']
					d[key] = np.reshape(a, [r, c], order = 'F')
				elif val['type'] == 'gretl_series':
					s = pd.Series(val['data'])
					d[key] = s.replace('NA', np.nan)
				elif val['type'] == 'gretl_list':
					d[key] = list(val['data'])[1:]

	def fit(self):
		if self.cpython_function:
			model_dict = self.cpython_function(*self.args, **self.options)
			json_dict = json.loads(model_dict['json_buffer'])
			del json_dict['type']
			self.__process_gretl_objects(json_dict)
			model_dict['results'] = json_dict
			del model_dict['json_buffer']
			self.__load_dict_model(model_dict)

		self.computed = True


if __name__ == "__main__":
	pass
