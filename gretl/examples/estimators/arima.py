import importlib.resources as resources
import gretl
import shutil

def run_example():
    data_dir = resources.files('gretl').joinpath('data')
    d1 = gretl.get_data(str(data_dir.joinpath('bjg.gdt')))
    d1.set_as_default()

    m = gretl.arima(y='lg', order=(1,1,1)).fit()
    print(m)

    m = gretl.arima(y='lg', order=(1,1,1), seasonal=(1,1,1)).fit()
    print(m)

    m = gretl.arima(y='lg', order=(1,1,1), verbose=True).fit()
    print(m)

    m = gretl.arima(y='lg', order=(1,1,1), vcv=True).fit()
    print(m)

    m = gretl.arima(y='lg', order=(1,1,1), hessian=True).fit()
    print(m)

    m = gretl.arima(y='lg', order=(1,1,1), opg=True).fit()
    print(m)

    m = gretl.arima(y='lg', order=(1,1,1), nc=True).fit()
    print(m)

    m = gretl.arima(y='lg', order=(1,1,1), conditional=True).fit()
    print(m)

    d = gretl.about()
    if shutil.which(d['built_in_strings']['x12a']) is not None:
        m = gretl.arima(y='lg', order=(1,1,1), x_12_arima=True).fit()
        print(m)

    m = gretl.arima(y='lg', order=(1,1,1), lbfgs=True).fit()
    print(m)

if __name__ == "__main__":
    run_example()
