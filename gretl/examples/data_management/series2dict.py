import gretl
import pandas as pd


def run_example():
    # Read the CSV file into a pandas DataFrame.
    df = pd.read_csv(
        gretl.gretldir + "/data/longley.csv",
        index_col="obs",
        parse_dates=True,
        date_format="%Y",
    )

    # Extract the 'employ' series.
    s = df["employ"]

    # Convert the Series to a dictionary compatible with gretl.
    d = gretl.series2dict(s)

    # Show the dictionary keys.
    print(d.keys())

    # Create a gretl dataset and display its summary.
    dset = gretl.get_data(d)
    print(dset)


if __name__ == "__main__":
    run_example()
