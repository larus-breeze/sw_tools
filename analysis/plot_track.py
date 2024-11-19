#!/user/bin/env python3
from load_to_df import LoadLarus2Df
import matplotlib.pyplot as plt
import sys
from geopy.distance import great_circle


def plot_track(file):

    data = LoadLarus2Df(file)
    df = data.df

    # Cut some data
    # df = df[460000:560000]

    # Prepare wind data for plotting.
    #print(len(df))
    #winddata = []
    # Plot wind for a timeframe
    #for i in range(0, 1800000):
    #    winddata.append()
    #TODO: implement https://matplotlib.org/stable/gallery/images_contours_and_fields/barb_demo.html#sphx-glr-gallery-images-contours-and-fields-barb-demo-py


    # Calculate correct aspect ratio for track plot
    distance_north_south = great_circle((df['Lat'].min(), 0), (df['Lat'].max(), 0))
    latitude_medium = df['Lat'].min() + 0.5 * (df['Lat'].max() - df['Lat'].min())
    distance_east_west = great_circle((latitude_medium, df['Long'].min()), (latitude_medium, df['Long'].max()))
    aspect_ratio = distance_north_south / distance_east_west

    fig, ax = plt.subplots()
    ax.plot(df['Long'].to_numpy(), df['Lat'].to_numpy())
    ax.grid()
    ax.set_aspect(aspect_ratio)
    plt.show()

if __name__ == "__main__":
    plot_track(sys.argv[1])

