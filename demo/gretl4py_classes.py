import sys
sys.path.append("../")
import gretl_cython

class Model:
	
	def __init__(self, name, *args, **kwargs):
		self.name = name
		try:
			self.cython_function = getattr(gretl_cython, name)
		except Exception as err:
			print(f"Unexpected {err=}, {type(err)=}")
			raise
		
		self.model_args = args
		self.model_kwargs = kwargs
		self.computed = False
					
	def __repr__(self):
		if self.computed:
			out = '\n'
			'''Function prints libgretl model'''
			DASHEDLINE = "---------------------------------------------------------------\n"
			ratio_type = 'z-score' if self._zscore == True else 't-ratio'
			out += f'\t\tcoefficient\t std. error\t{ratio_type:s}\t p-value\n'
			out += ('  ' + DASHEDLINE)
			coeff_names = self.coeff.keys()
			for variable in coeff_names:
				coeff = self.coeff[variable]
				sderr = self.sderr[variable]
				ratio = coeff/sderr
				pvalue = self.pvalue[variable]
				out += (f'  {variable:s} \t {coeff:9.4f}\t {sderr:9.4f}\t {ratio:5.3f}\t {pvalue:4.2e}\n')
			out += ('\n')
			out += (f'Mean dependent var  {self.ybar:9.4f}\t S.D. dependent var  {self.sdy:10.4f}\n')
			out += (f'Log-likelihood      {self.lnL:9.4f}\t Akaike criterion     {self.criterion["aic"]:9.4f}\n')
			out += (f'Schwarz criterion   {self.criterion["bic"]:9.4f}\t Hannan-Quinn\t      {self.criterion["hqc"]:9.4f}\n')
			out += ('\nvcv matrix\n')
			out += (str(self.vcv) + '\n')
			out += ('\n')
		else:
			out = '\n'
			out += "Model " + self.name.upper()
		return out
		
	def __load_dict_model(self, dict_model):
		if dict_model:
			for _ in dict_model: setattr(self, _, dict_model[_])
	
	def fit(self):
		if self.cython_function:
			model_result = self.cython_function(*self.model_args, **self.model_kwargs)
			self.__load_dict_model(model_result)
		
		self.computed = True
		
		
if __name__ == "__main__":
	pass
