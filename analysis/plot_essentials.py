#!/user/bin/env python3
import numpy as np
import matplotlib.pyplot as plt
from geopy.distance import great_circle

def plot_mag(df):
    t = (df.index / 100.0 / 60.0).to_numpy()  # 100Hz ticks to minutes for the time axis

    nav_ind_abs = np.sqrt(
        np.square(df['nav ind mag N']) + np.square(df['nav ind mag E']) + np.square(df['nav ind mag D']))

    # Plot the data
    figure, axis = plt.subplots()
    figure.suptitle("Induction in earth system: ")
    plt.autoscale(enable=True, axis='y')
    axis.grid()
    axis.set_xlabel('t [minutes]')

    axis.plot(t, nav_ind_abs.to_numpy(), "k", linewidth=1)
    axis.legend(["nav ind abs"], loc="lower left")
    par1 = axis.twinx()
    par1.plot(t, df['nav ind mag N'].to_numpy(), "r-", linewidth=0.5)
    par1.plot(t, df['nav ind mag E'].to_numpy(), "g-", linewidth=0.5)
    par1.plot(t, df['nav ind mag D'].to_numpy(), "b-", linewidth=0.5)

    minimum = df['nav ind mag N'].min()
    maximum = df['nav ind mag N'].max()
    if df['nav ind mag E'].min() < minimum:
        minimum = df['nav ind mag E'].min()
    if df['nav ind mag E'].max() > maximum:
        maximum = df['nav ind mag E'].max()
    if df['nav ind mag D'].min() < minimum:
        minimum = df['nav ind mag D'].min()
    if df['nav ind mag D'].max() > maximum:
        maximum = df['nav ind mag D'].max()
    par1.set_ylim(minimum - 0.5, maximum + 1.5)
    par1.legend(['nav ind mag N', 'nav ind mag E', 'nav ind mag D'], loc="lower right")
    plt.show()

def plot_track(df):
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

def plot_wind(df):
    # Prepare the data
    t = (df.index / 100.0 / 60.0).to_numpy()   # 100Hz ticks to minutes for the time axis
    windDirectionAvg = np.arctan2( - df['wind avg E'], - df['wind avg N']) / 2 / np.pi * 360
    windDirectionInst = np.arctan2( - df['wind E'], - df['wind N']) / 2 / np.pi * 360
    windSpeedAvg = np.sqrt(np.square(df['wind avg E']) + np.square(df['wind avg N']))
    windSpeedInst = np.sqrt(np.square(df['wind E']) + np.square(df['wind N']))
    heading = df['yaw'] / 2 / np.pi * 360
    roll_deg = df['roll'] / 2 / np.pi * 360
    nav_ind_abs = np.sqrt(np.square(df['nav ind mag N'])*np.square(df['nav ind mag N'] + df['nav ind mag E'])*np.square(df['nav ind mag E']) + df['nav ind mag D'])*np.square(df['nav ind mag D'])

    # Plot the data:
    figure, axis = plt.subplots(2, 2, sharex=True)
    figure.suptitle("Wind data")
    plt.autoscale(enable=True, axis='y')

    # Wind N / E instantaneous and average
    axis[0, 0].plot(t, roll_deg.to_numpy(), "b--", alpha=0.5, linewidth=0.5)
    axis[0, 0].legend(["roll angle"], loc="lower left")
    axis[0, 0].grid()
    par1 = axis[0, 0].twinx()
    par1.plot(t, df['wind avg N'].to_numpy(), "r-", linewidth=0.5)
    par1.plot(t, df['wind avg E'].to_numpy(), "g-", linewidth=0.5)
    par1.plot(t, df['wind N'].to_numpy(), "r--", linewidth=0.5, alpha=0.6)
    par1.plot(t, df['wind E'].to_numpy(), "g--", linewidth=0.5, alpha=0.6)
    par1.legend(['wind avg N', 'wind avg E', 'wind N', 'wind E'], loc="lower right")

    # Wind direction and speed instantaneous and average
    axis[0, 1].plot(t, windDirectionAvg.to_numpy(), "b-", label='Average', alpha=1, linewidth=0.5)
    axis[0, 1].plot(t, windDirectionInst.to_numpy(), "b--", label='Instantaneous', alpha=1, linewidth=0.5)
    axis[0, 1].plot(t, heading.to_numpy(), "g-", label='Instantaneous', alpha=1, linewidth=0.7)
    axis[0, 1].grid()
    par1 = axis[0, 1].twinx()
    par1.plot(t, windSpeedAvg.to_numpy(), "r-", label='Average', alpha=1, linewidth=0.5)
    par1.plot(t, windSpeedInst.to_numpy(), "r--", label='Instantaneous', alpha=1, linewidth=0.5)
    axis[0, 1].legend(["average wind direction", "inst wind direction", "heading"], loc="lower left")
    par1.legend(["average wind speed", "inst wind speed"], loc="lower right")

    # Turn rate and circle mode
    axis[1, 0].plot(t, df['turn rate'].to_numpy(), "b--", alpha=1, linewidth=0.5)
    axis[1, 0].legend(["turn rate"], loc="lower left")
    axis[1, 0].grid()
    axis[1, 0].set_xlabel('t [minutes]')
    par1 = axis[1, 0].twinx()
    par1.plot(t, df['circle mode'].to_numpy(), "y-", linewidth=0.5)
    par1.legend(["circle mode"], loc="lower right")

    # TAS and height
    axis[1, 1].plot(t, df['TAS'].to_numpy(), "b-", alpha=1, linewidth=0.5)
    axis[1, 1].legend(["TAS"], loc="lower left")
    axis[1, 1].grid()
    axis[1, 1].set_xlabel('t [minutes]')
    par1 = axis[1, 1].twinx()
    par1.plot(t, - df['pos DWN'].to_numpy(), "b--", alpha=1, linewidth=0.5)
    par1.legend(["pos UP"], loc="lower right")

    plt.show()

def plot_vario(df):
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
    from larus_to_df import Larus2Df
    obj = Larus2Df('240520_091630.f37')
    data_frame = obj.get_df()

    plot_mag(data_frame)
    plot_vario(data_frame)
    plot_wind(data_frame)
    plot_track(data_frame)