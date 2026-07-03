import gretl
import pandas as pd


def run_example():
    # Read the CSV file into a pandas DataFrame.
    df = pd.read_csv(gretl.gretldir + "/data/longley.csv")

    # Convert the DataFrame to a dictionary of series, preserving observation labels.
    d = gretl.df2dict(df, with_obs=True)

    # Show the dictionary keys.
    print(d.keys())

    # Create a gretl dataset and display its summary.
    dset = gretl.get_data(d)
    print(dset)


if __name__ == "__main__":
    run_example()
