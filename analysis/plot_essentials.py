#!/user/bin/env python3
import matplotlib.pyplot as plt
import numpy as np
import sys
from pathlib import Path

sys.path.append(str(Path(__file__).parent.parent))   # Add parent folder to make imports of parallel directory possible
from larus_data.larus_to_df import Larus2Df

from matplotlib import colors
from geopy.distance import great_circle

def plot_mag_track(df, path = None):
    t = (df.index / 100.0 / 60.0).to_numpy()  # 100Hz ticks to minutes for the time axis

    nav_ind_abs = np.sqrt(
        np.square(df['dev_nav_ind_mag_north']) + np.square(df['dev_nav_ind_mag_east']) + np.square(df['dev_nav_ind_mag_down']))

    # Plot the data
    figure, axis = plt.subplots(2, 1, sharex=True)
    title = "Induction in earth system \n {}".format(path)
    figure.suptitle(title, size="small")
    plt.autoscale(enable=True, axis='y')
    axis[0,].grid()
    axis[0,].set_xlabel('t [minutes]')

    axis[0,].plot(t, nav_ind_abs.to_numpy(), "k", linewidth=1)
    axis[0,].legend(["nav ind abs"], loc="lower left")
    par1 = axis[0,].twinx()
    par1.plot(t, df['dev_nav_ind_mag_north'].to_numpy(), "r-", linewidth=0.5)
    par1.plot(t, df['dev_nav_ind_mag_east'].to_numpy(), "g-", linewidth=0.5)
    par1.plot(t, df['dev_nav_ind_mag_down'].to_numpy(), "b-", linewidth=0.5)

    minimum = df['dev_nav_ind_mag_north'].min()
    maximum = df['dev_nav_ind_mag_north'].max()
    if df['dev_nav_ind_mag_east'].min() < minimum:
        minimum = df['dev_nav_ind_mag_east'].min()
    if df['dev_nav_ind_mag_east'].max() > maximum:
        maximum = df['dev_nav_ind_mag_east'].max()
    if df['dev_nav_ind_mag_down'].min() < minimum:
        minimum = df['dev_nav_ind_mag_down'].min()
    if df['dev_nav_ind_mag_down'].max() > maximum:
        maximum = df['dev_nav_ind_mag_down'].max()
    par1.set_ylim(minimum - 0.5, maximum + 1.5)
    par1.legend(['dev_nav_ind_mag_north', 'dev_nav_ind_mag_east', 'dev_nav_ind_mag_down'], loc="lower right")


    parameter = "track_ground"
    axis[1,].plot(t, df[parameter].to_numpy(), "k", linewidth=0.5)
    axis[1,].legend([parameter], loc="lower left")
    axis[1,].grid()

    parameter = "turn_rate"
    par1 = axis[1,].twinx()
    par1.plot(t, df[parameter].to_numpy(), "c", linewidth=0.5)
    par1.legend([parameter], loc="lower right")
    plt.show()



def plot_mag(df, path = None):
    t = (df.index / 100.0 / 60.0).to_numpy()  # 100Hz ticks to minutes for the time axis

    nav_ind_abs = np.sqrt(
        np.square(df['dev_nav_ind_mag_north']) + np.square(df['dev_nav_ind_mag_east']) + np.square(
            df['dev_nav_ind_mag_down']))

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
    par1.plot(t, df['dev_nav_ind_mag_north'].to_numpy(), "r-", linewidth=0.5)
    par1.plot(t, df['dev_nav_ind_mag_east'].to_numpy(), "g-", linewidth=0.5)
    par1.plot(t, df['dev_nav_ind_mag_down'].to_numpy(), "b-", linewidth=0.5)

    minimum = df['dev_nav_ind_mag_north'].min()
    maximum = df['dev_nav_ind_mag_north'].max()
    if df['dev_nav_ind_mag_east'].min() < minimum:
        minimum = df['dev_nav_ind_mag_east'].min()
    if df['dev_nav_ind_mag_east'].max() > maximum:
        maximum = df['dev_nav_ind_mag_east'].max()
    if df['dev_nav_ind_mag_down'].min() < minimum:
        minimum = df['dev_nav_ind_mag_down'].min()
    if df['dev_nav_ind_mag_down'].max() > maximum:
        maximum = df['dev_nav_ind_mag_down'].max()
    par1.set_ylim(minimum - 0.5, maximum + 1.5)
    par1.legend(['dev_nav_ind_mag_north', 'dev_nav_ind_mag_east', 'dev_nav_ind_mag_down'], loc="lower right")
    plt.show()

def plot_track(df, path = None):
    # Calculate correct aspect ratio for track plot
    distance_north_south = great_circle((df['latitude'].min(), 0), (df['latitude'].max(), 0))
    latitude_medium = df['latitude'].min() + 0.5 * (df['latitude'].max() - df['latitude'].min())
    distance_east_west = great_circle((latitude_medium, df['longitude'].min()), (latitude_medium, df['longitude'].max()))
    aspect_ratio = distance_north_south / distance_east_west

    figure, ax = plt.subplots()
    title = "Flight Track \n {}".format(path)
    figure.suptitle(title, size="small")
    ax.plot(df['longitude'].to_numpy(), df['latitude'].to_numpy())
    ax.grid()
    ax.set_xlabel("Longitude [°]")
    ax.set_ylabel("Latitude [°]")
    ax.set_aspect(aspect_ratio)
    plt.show()

def plot_wind(df, path = None):
    # Prepare the data
    t = (df.index / 100.0 / 60.0).to_numpy()   # 100Hz ticks to minutes for the time axis
    wind_direction_avg = np.arctan2( - df['wind_average_east'], - df['wind_average_north']) / 2 / np.pi * 360
    wind_direction_inst = np.arctan2( - df['wind_east'], - df['wind_north']) / 2 / np.pi * 360
    wind_speed_avg = np.sqrt(np.square(df['wind_average_east']) + np.square(df['wind_average_north']))
    wind_speed_inst = np.sqrt(np.square(df['wind_east']) + np.square(df['wind_north']))
    heading = df['yaw_angle'] / 2 / np.pi * 360
    roll_deg = df['roll_angle'] / 2 / np.pi * 360
    nav_ind_abs = np.sqrt(np.square(df['dev_nav_ind_mag_north'])*np.square(df['dev_nav_ind_mag_north'] + df['dev_nav_ind_mag_east'])*np.square(df['dev_nav_ind_mag_east']) + df['dev_nav_ind_mag_down'])*np.square(df['dev_nav_ind_mag_down'])

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
    par1.plot(t, df['wind_average_north'].to_numpy(), "r-", linewidth=0.5)
    par1.plot(t, df['wind_average_east'].to_numpy(), "g-", linewidth=0.5)
    par1.plot(t, df['wind_north'].to_numpy(), "r--", linewidth=0.5, alpha=0.6)
    par1.plot(t, df['wind_east'].to_numpy(), "g--", linewidth=0.5, alpha=0.6)
    par1.legend(['wind_average_north', 'wind_average_east', 'wind_north', 'wind_east'], loc="lower right")

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
    axis[1, 0].plot(t, df['turn_rate'].to_numpy(), "b--", alpha=1, linewidth=0.5)
    axis[1, 0].legend(["turn rate"], loc="lower left")
    axis[1, 0].grid()
    axis[1, 0].set_xlabel('t [minutes]')
    par1 = axis[1, 0].twinx()
    par1.plot(t, df['circle_mode'].to_numpy(), "y-", linewidth=0.5)
    par1.legend(["circle mode"], loc="lower right")

    # TAS and height
    axis[1, 1].plot(t, df['tas'].to_numpy(), "b-", alpha=1, linewidth=0.5)
    axis[1, 1].legend(["tas"], loc="lower left")
    axis[1, 1].grid()
    axis[1, 1].set_xlabel('t [minutes]')
    par1 = axis[1, 1].twinx()
    par1.plot(t, - df['position_down'].to_numpy(), "b--", alpha=1, linewidth=0.5)
    par1.legend(["position up"], loc="lower right")
    plt.show()


def plot_ahrs(df, path = None):
    # Prepare the data
    t = (df.index / 100.0 / 60.0).to_numpy()   # 100Hz ticks to minutes for the time axis

    heading = df['yaw_angle'] / 2 / np.pi * 360
    roll_deg = df['roll_angle'] / 2 / np.pi * 360
    pitch_deg = df['pitch_angle'] / 2 / np.pi * 360
    slip_deg = df['slip_angle'] / 2 / np.pi * 360

    # Plot the data
    figure, axis = plt.subplots(3, 1, sharex=True)
    title = "AHRS data \n {}".format(path)
    figure.suptitle(title, size="small" )
    plt.autoscale(enable=True, axis='y')

    # Plot pitch
    axis[0,].plot(t, pitch_deg.to_numpy(), "b", linewidth=0.5)
    axis[0,].legend(["pitch_angle"], loc="lower left")
    axis[0,].grid()
    par1 = axis[0,].twinx()

    # Plot roll
    axis[1,].plot(t, roll_deg.to_numpy(), "r", linewidth=0.5)
    axis[1,].legend(["roll_angle"], loc="lower left")
    axis[1,].grid()
    #par1 = axis[1,].twinx()
    #second_plot_param = "track GNSS"  # "turn rate"
    #par1.plot(t, df[second_plot_param].to_numpy(), "c", linewidth=0.5)
    #par1.legend([second_plot_param], loc="lower right")
    #y_lim_max = 1.1 * df[second_plot_param].max()
    #y_lim_min = 1.1 * df[second_plot_param].min()
    #par1.set_ylim(y_lim_min, y_lim_max)

    # Plot slip angle
    axis[2,].plot(t, slip_deg.to_numpy(), "g", linewidth=0.5)
    axis[2,].legend(["slip_angle"], loc="lower left")
    axis[2,].grid()
    par1 = axis[2,].twinx()
    plt.show()

def plot_gnss(df, path = None):
    # Prepare the data
    t = (df.index / 100.0 / 60.0).to_numpy()  # 100Hz ticks to minutes for the time axis

    roll_deg = df['roll_angle'] / 2 / np.pi * 360
    pitch_deg = df['pitch_angle'] / 2 / np.pi * 360

    # Plot the data
    figure, host = plt.subplots()
    ax1 = host

    title = "GNSS status \n {}".format(path)
    figure.suptitle(title, size="small")
    plt.autoscale(enable=True, axis='y')
    ax1.grid()
    ax1.set_xlabel('t [minutes]')

    # Plot pitch
    p1 = ax1.plot(t, pitch_deg.to_numpy(), "b", linewidth=0.5, alpha=0.5, label="pitch angle")
    p2 = ax1.plot(t, roll_deg.to_numpy(), "r", linewidth=0.5, alpha=0.8, label="roll angle")
    ax1.grid()
    ax1.set_ylabel('deg [°]')

    color1, color2, color3 = plt.cm.viridis([0, .5, .9])

    # Plot number of satellites
    ax2 = host.twinx()
    p3 = ax2.plot(t, df['sat_count'].to_numpy(), color=color2, linewidth=0.5, label='Satellites', alpha=0.5)
    ax2.set_ylabel('Satellites', color=color2)
    ax2.set_ylim(0, 40)


    # Plot GNSS status
    ax3 = host.twinx()

    ax3.spines['right'].set_position(('outward', 60))  # Separate scale
    df_sat_fix_type = df['sat_fix_type'] # 0(no-fix), 1(normal fix), 3(dgnss heading fix)
    d_gnss_heading_info = ""
    if 3 not in df_sat_fix_type.values:
        # There is no gnss heading fix at all. So probably a single gnss version.
        d_gnss_label = 'No GNSS Fix'
        df_sat_fix_type = df_sat_fix_type.loc[df_sat_fix_type != 1]
        t_sat_fix_type = (df_sat_fix_type.index / 100.0 / 60.0).to_numpy()  # 100Hz ticks to minutes for the time axis

        # Drop normal fixes
        df_sat_fix_type.replace(0, 'x', inplace=True)
        df_sat_fix_type.replace(3, 'x', inplace=True)

    else:
        longest_interval = 0
        tick = 0
        index = 0
        minute = 0
        for i in df_sat_fix_type:
            index = index +1
            if i != 3:
                tick = tick + 1
            else:
                if tick > longest_interval:
                    longest_interval = tick
                    minute =  index / 100 / 60
                tick = 0

        if longest_interval > 0:
            d_gnss_heading_info = "Longest DGNSS Heading Gap \n duration {}s \n position: {}m ".format(longest_interval / 100.0, round(minute,2))

        # Drop d_gnss fixes (3)
        d_gnss_label = 'No DGNSS Heading'
        df_sat_fix_type = df_sat_fix_type.loc[df_sat_fix_type != 3]
        t_sat_fix_type = (df_sat_fix_type.index / 100.0 / 60.0).to_numpy()  # 100Hz ticks to minutes for the time axis
        df_sat_fix_type.replace(0, 'x', inplace=True)
        df_sat_fix_type.replace(1, 'x', inplace=True)

    p4 = ax3.plot(t_sat_fix_type, df_sat_fix_type, color=color1, linewidth=0.0, label=d_gnss_label, alpha=0.3, marker='x')

    host.legend(handles=p1 + p2 + p3 + p4, loc='lower right')
    host.text(0, 40, d_gnss_heading_info, fontsize=10, bbox = dict(facecolor = 'white', alpha = 1))
    plt.show()


def plot_altitude_speed(df, path = None):
    # Prepare the data
    t = (df.index / 100.0 / 60.0).to_numpy()   # 100Hz ticks to minutes for the time axis

    altitude_m = - df['position_down']
    speed_kmh = df['tas'] * 3.6

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


def  plot_attitude_histogram(df, path = None):
    # Plot the data
    figure, axis = plt.subplots()
    title = "Roll & Pitch Histogram \n {}".format(path)
    figure.suptitle(title, size="small")
    axis.grid()
    axis.set_ylabel('nick angle [°]')
    axis.set_xlabel('roll angle [°]')

    x_data = df['roll_angle'] / 2 / np.pi * 360
    y_data = df['pitch_angle'] / 2 / np.pi * 360

    # We can increase the number of bins on each axis
    axis.hist2d(x_data, y_data, bins=500, norm=colors.LogNorm())
    plt.show()


if __name__ == "__main__":
    basepath = '/home/mbetz/Documents/vmware/shared_folder/'
    # D-GNSS Flight data:
    #file = basepath + '/230430.f37'                # DGNSS OM flight.
    #file = basepath + '/240830_short.f37'          # DGNSS Stefly WM Texas
    #file = basepath + '/250824_091030.f37'         # DGNSS OM new sensor good magnetic data but not prior calibration

    # S-GNSS flight data:
    #file = basepath + '/240520_091630_nomag.f37'   # Single GNSS Magnetic calibration test with slightly wrong roll, pitch configuration
    #file = basepath + '/250522_142340.f37'         # Single GNSS short flight
    #file = basepath + '/251003_090210.f37'         # ASK21 Winch launch
    file = basepath + '/250510_112200.f37'          # Duo Discus Thermal Flight (no prior magnetic calibration)

    data = Larus2Df(file).get_df()
    plot_attitude_histogram(data, file)
    plot_mag_track(data, file)
    plot_mag(data, file)
    plot_gnss(data, file)
    plot_ahrs(data, file)
    plot_altitude_speed(data, file)
    plot_wind(data, file)
    plot_track(data, file)
