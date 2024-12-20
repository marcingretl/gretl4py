import gretl

''' we load bjg.csv to dict() '''
bjg = gretl.csv2dict('bjg.csv', 12)

''' we load data9-7.csv to dict() '''
data97 = gretl.csv2dict('data9-7.csv', 4)

print('Loading global dataset: bjg.csv') # this dataset will be used later
gretl.attach_data(bjg)

'''Example 1: data is send on function call ('data9-7.csv')'''
print('\nExample 1:')
print('arma(1,1,1,False)\n')
res = gretl.arima(y="QNC", order=(1,1,1), data=data97).fit()
print(res)

print('\nOLS:')
res = gretl.ols('QNC ~ const + PRICE + INCOME', data=data97).fit()
print(res)
print(res.args)
print(res) # print(res.summary)
print(res.results['vcv'])
print(f'\nIs Model {res.id():d} asymptotic?: {res.results["asymptotic"]}')

'''Example 2: we use data send before function call ('bjg.csv')'''
print('\nExample 2:')
print('arma(1,0,1,print=False)\n')
res = gretl.arima(y="lg", order=(1,1)).fit()
print(res)

print('\nOLS:\n')
gretl.attach_data(data97)
res = gretl.ols('QNC ~ const + PRICE + INCOME').fit()
print(res)
