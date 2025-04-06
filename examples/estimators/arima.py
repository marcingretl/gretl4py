import gretl
import shutil

d1 = gretl.get_data(gretl.gretl_data_path('misc/bjg.gdt'))
m = gretl.arima(y='lg', order=(1,1,1)).fit()
print(m)

# SARIMA
m = gretl.arima(y='lg', order=(1,1,1), seasonal=(1,1,1)).fit()
print(m)

# verbose option
m = gretl.arima(y='lg', order=(1,1,1), verbose=True).fit()
print(m)

# print covariance matrix
m = gretl.arima(y='lg', order=(1,1,1), vcv=True).fit()
print(m)

# disable the fallback to outer product of the gradient (OPG)
m = gretl.arima(y='lg', order=(1,1,1), hessian=True).fit()
print(m)

# force use of the OPG method
m = gretl.arima(y='lg', order=(1,1,1), opg=True).fit()
print(m)

# model without constant
m = gretl.arima(y='lg', order=(1,1,1), nc=True).fit()
print(m)

# use conditional maximum likelihood
m = gretl.arima(y='lg', order=(1,1,1), conditional=True).fit()
print(m)

# use X-12- or X-13-ARIMA
# we ensure that x12 or x13 is installed via contents of gretl.about()
d = gretl.about()
if shutil.which(d['built_in_strings']['x12a']) is not None:
    m = gretl.arima(y='lg', order=(1,1,1), x_12_arima=True).fit()
    print(m)

# use L-BFGS-B maximizer
m = gretl.arima(y='lg', order=(1,1,1), lbfgs=True).fit()
print(m)

# ARIMA lag selection based on information criteria and log-likelihood
# warning: this option is temporarily unavailable
#m = gretl.arima(y='lg', order=(1,1,1), lagselect=True).fit()
#print(m)

# SARIMA lag selection based on information criteria and log-likelihood
# warning: this option is temporarily unavailable
#m = gretl.arima(y='lg', order=(1,1,1), seasonal=(1,1,1), lagselect=True).fit()
#print(m)
