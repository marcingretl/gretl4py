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

		self.model_args = args
		self.model_kwargs = kwargs
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
					d[key] = s

	def fit(self):
		if self.cpython_function:
			model_result = self.cpython_function(*self.model_args, **self.model_kwargs)
			res = json.loads(model_result['json_buffer'])
			self.__process_gretl_objects(res)
			model_result['results'] = res
			del model_result['json_buffer']
			self.__load_dict_model(model_result)

		self.computed = True


if __name__ == "__main__":
	pass
