import gretl
from numpy import genfromtxt


def run_example():
    # Load the CSV file as a NumPy array, skipping the header and index column.
    np = genfromtxt(gretl.gretldir + "/data/longley.csv",
                    delimiter=",")[1:, 1:]

    # Convert the NumPy array to a dictionary of series.
    d = gretl.nparray2dict(np)

    # Show the dictionary keys.
    print(d.keys())

    # Create a gretl dataset and display its summary.
    dset = gretl.get_data(d)
    print(dset)


if __name__ == "__main__":
    run_example()

