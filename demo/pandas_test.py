import sys
sys.path.append("../")

import gretl
import pandas
from gretl_print import gretl_model_print
from df2dict import df2dict

dframe = pandas.read_csv('data9-7.csv')
MyDict = df2dict(dframe)
ha = gretl.arma(data=MyDict, y="QNC", p=1, d=0, q=1, print=True)


