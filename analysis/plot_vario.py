#!/user/bin/env python3
from load_to_df import LoadLarus2Df
import numpy as np
import matplotlib.pyplot as plt
import sys

def plot_vario(file):

    data = LoadLarus2Df(file)
    df = data.df

    # Prepare the data
    t = (df.index / 100.0 / 60.0).to_numpy()   # 100Hz ticks to minutes for the time axis

    # Create figure with 3 independent y-axis
    fig, host = plt.subplots(figsize=(8, 5), layout='constrained')
    ax2 = host.twinx()
    ax3 = host.twinx()

    # Set y-axis limits
    host.set_ylim(df["Pressure-altitude"].min(), df["Pressure-altitude"].max())
    ax2.set_ylim(df["vario"].min(), df["vario"].max())
    ax3.set_ylim(15.0, df["TAS"].max() + 100.0)

    host.set_xlabel("t [minutes]")
    host.set_ylabel("Pressure-altitude")
    ax2.set_ylabel("Vario")
    ax3.set_ylabel("TAS")

    color1, color2, color3, color4 = plt.cm.viridis([0, .25, .5, .9])

    p1 = host.plot(t, df["Pressure-altitude"].to_numpy(), color=color1, label="Pressure-altitude")
    p2 = ax2.plot(t, df["vario"].to_numpy(), color=color2, label="vario")
    p2b = ax2.plot(t, df["vario integrator"].to_numpy(), color=color3, label="vario integrator")
    p3 = ax3.plot(t, df["TAS"].to_numpy(), color=color4, label="TAS")

    host.legend(handles=p1 + p2 + p2b + p3, loc='best')

    # right, left, top, bottom
    ax3.spines['right'].set_position(('outward', 60))

    host.yaxis.label.set_color(p1[0].get_color())
    ax2.yaxis.label.set_color(p2[0].get_color())
    ax3.yaxis.label.set_color(p3[0].get_color())

    plt.show()


if __name__ == "__main__":
    plot_vario(sys.argv[1])
