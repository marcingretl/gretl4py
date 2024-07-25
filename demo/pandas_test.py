import sys
sys.path.append("../")

import gretl
import pandas
from gretl_print import gretl_model_print
from df2dict import df2dict

dframe = pandas.read_csv('data9-7.csv')
MyDict = df2dict(dframe)

# arma model
armamod = gretl.arma(data=MyDict, y="QNC", p=1, d=0, q=1)
gretl_model_print(armamod)

# ols with r-style formula
olsmod = gretl.ols('QNC ~ const + PRICE + INCOME', data=MyDict)
gretl_model_print(olsmod)

# probit with native printout
probmod = gretl.probit('WINTER ~ const + PRICE + INCOME', data=MyDict, print=True)

