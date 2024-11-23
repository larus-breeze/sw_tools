#!/user/bin/env python3
import os

import numpy as np
import matplotlib.pyplot as plt
from geopy.distance import great_circle

def plot_mag(df, path = None):
    t = (df.index / 100.0 / 60.0).to_numpy()  # 100Hz ticks to minutes for the time axis

    nav_ind_abs = np.sqrt(
        np.square(df['nav ind mag N']) + np.square(df['nav ind mag E']) + np.square(df['nav ind mag D']))

    # Plot the data
    figure, axis = plt.subplots()
    title = "Induction in earth system \n {}".format(path)
    figure.suptitle(title, size="small")
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

def plot_track(df, path = None):
    # Calculate correct aspect ratio for track plot
    distance_north_south = great_circle((df['Lat'].min(), 0), (df['Lat'].max(), 0))
    latitude_medium = df['Lat'].min() + 0.5 * (df['Lat'].max() - df['Lat'].min())
    distance_east_west = great_circle((latitude_medium, df['Long'].min()), (latitude_medium, df['Long'].max()))
    aspect_ratio = distance_north_south / distance_east_west

    figure, ax = plt.subplots()
    title = "Flight Track \n {}".format(path)
    figure.suptitle(title, size="small")
    ax.plot(df['Long'].to_numpy(), df['Lat'].to_numpy())
    ax.grid()
    ax.set_xlabel("Longitude [°]")
    ax.set_ylabel("Latitude [°]")
    ax.set_aspect(aspect_ratio)
    plt.show()

def plot_wind(df, path = None):
    # Prepare the data
    t = (df.index / 100.0 / 60.0).to_numpy()   # 100Hz ticks to minutes for the time axis
    wind_direction_avg = np.arctan2( - df['wind avg E'], - df['wind avg N']) / 2 / np.pi * 360
    wind_direction_inst = np.arctan2( - df['wind E'], - df['wind N']) / 2 / np.pi * 360
    wind_speed_avg = np.sqrt(np.square(df['wind avg E']) + np.square(df['wind avg N']))
    wind_speed_inst = np.sqrt(np.square(df['wind E']) + np.square(df['wind N']))
    heading = df['yaw'] / 2 / np.pi * 360
    roll_deg = df['roll'] / 2 / np.pi * 360
    nav_ind_abs = np.sqrt(np.square(df['nav ind mag N'])*np.square(df['nav ind mag N'] + df['nav ind mag E'])*np.square(df['nav ind mag E']) + df['nav ind mag D'])*np.square(df['nav ind mag D'])

    # Plot the data:
    figure, axis = plt.subplots(2, 2, sharex=True)
    title = "Wind data \n {}".format(path)
    figure.suptitle(title, size="small")
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
    axis[0, 1].plot(t, wind_direction_avg.to_numpy(), "b-", label='Average', alpha=1, linewidth=0.5)
    axis[0, 1].plot(t, wind_direction_inst.to_numpy(), "b--", label='Instantaneous', alpha=1, linewidth=0.5)
    axis[0, 1].plot(t, heading.to_numpy(), "g-", label='Instantaneous', alpha=1, linewidth=0.7)
    axis[0, 1].grid()
    par1 = axis[0, 1].twinx()
    par1.plot(t, wind_speed_avg.to_numpy(), "r-", label='Average', alpha=1, linewidth=0.5)
    par1.plot(t, wind_speed_inst.to_numpy(), "r--", label='Instantaneous', alpha=1, linewidth=0.5)
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


def plot_ahrs(df, path = None):
    # Prepare the data
    t = (df.index / 100.0 / 60.0).to_numpy()   # 100Hz ticks to minutes for the time axis

    heading = df['yaw'] / 2 / np.pi * 360
    roll_deg = df['roll'] / 2 / np.pi * 360
    nick_deg = df['nick'] / 2 / np.pi * 360
    slip_deg = df['slip angle'] / 2 / np.pi * 360

    # Plot the data:
    figure, axis = plt.subplots(3, 1, sharex=True)
    title = "AHRS data \n {}".format(path)
    figure.suptitle(title, size="small" )
    plt.autoscale(enable=True, axis='y')

    # Plot nick
    axis[0,].plot(t, nick_deg.to_numpy(), "b", linewidth=0.5)
    axis[0,].legend(["nick angle"], loc="lower left")
    axis[0,].grid()
    par1 = axis[0,].twinx()

    # Plot roll
    axis[1,].plot(t, roll_deg.to_numpy(), "r", linewidth=0.5)
    axis[1,].legend(["roll angle"], loc="lower left")
    axis[1,].grid()
    par1 = axis[1,].twinx()

    # Plot slip angle
    axis[2,].plot(t, slip_deg.to_numpy(), "g", linewidth=0.5)
    axis[2,].legend(["slip angle"], loc="lower left")
    axis[2,].grid()
    axis[2,].set_xlabel('t [minutes]')
    par1 = axis[2,].twinx()
    plt.show()


def plot_altitude_speed(df, path = None):
    # Prepare the data
    t = (df.index / 100.0 / 60.0).to_numpy()   # 100Hz ticks to minutes for the time axis

    altitude_m = - df['pos DWN']
    speed_kmh = df['TAS'] * 3.6

    # Plot the data:
    figure, axis = plt.subplots(2, 1, sharex=True)
    title = "Altitude and Speed data \n {}".format(path)
    figure.suptitle(title, size="small")
    plt.autoscale(enable=True, axis='y')

    # Plot altitude
    axis[0,].plot(t, altitude_m.to_numpy(), "b", linewidth=0.5)
    axis[0,].legend(["GNSS altitude [m]"], loc="lower left")
    axis[0,].grid()
    par1 = axis[0,].twinx()

    # Plot speed
    axis[1,].plot(t, speed_kmh.to_numpy(), "r", linewidth=0.5)
    axis[1,].legend(["speed_kmh (TAS)"], loc="lower left")
    axis[1,].grid()
    axis[1,].set_xlabel('t [minutes]')
    par1 = axis[1,].twinx()

    plt.show()

if __name__ == "__main__":
    from larus_to_df import Larus2Df

    value = 'nick'
    value = 'sat fix type f'
    value = 'sat number'
    value = 'sat fix type'
    #value = 'ubatt'
    #value = 'Magnetic Disturbance'


    file = os.getcwd() + '/240520_091630.f37'
    file = os.getcwd() + '/230430.f37'

    data = Larus2Df(file).get_df()


    plot_ahrs(data, file)
    plot_mag(data, file)
    plot_altitude_speed(data, file)
    plot_wind(data, file)
    plot_track(data, file)