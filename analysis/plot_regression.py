#!/user/bin/env python3
import os
import time

import numpy as np
import matplotlib.pyplot as plt
from matplotlib import colors
from geopy.distance import great_circle
from larus_data.larus_to_df import Larus2Df

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


def plot_wind_comparison(df1, df2, path = None):
    t = (df1.index / 100.0 / 60.0).to_numpy()
    figure, axis = plt.subplots(2, 1, sharex=True)

    # Wind instantaneous comparison
    axis[0,].plot(t, df1['wind N'].to_numpy(), "r-", linewidth=1.5)
    axis[0,].plot(t, df2['wind N'].to_numpy(), "g-", linewidth=0.5)
    axis[0,].legend(["df1 wind N", "df2 wind N"], loc="lower left")
    axis[0,].grid()
    par1 = axis[0,].twinx()
    par1.plot(t, df1['wind E'].to_numpy(), "c--", linewidth=1.5)
    par1.plot(t, df2['wind E'].to_numpy(), "b--", linewidth=0.5)
    par1.legend(['df1 wind E', 'df2 wind E'], loc="lower right")

    # Wind average comparison
    axis[1,].plot(t, df1['wind avg N'].to_numpy(), "r-", linewidth=1.5)
    axis[1,].plot(t, df2['wind avg N'].to_numpy(), "g-", linewidth=0.5)
    axis[1,].legend(["df1 wind avg N", "df2 wind avg N"], loc="lower left")
    axis[1,].grid()
    par1 = axis[1,].twinx()
    par1.plot(t, df1['wind avg E'].to_numpy(), "c--", linewidth=1.5)
    par1.plot(t, df2['wind avg E'].to_numpy(), "b--", linewidth=0.5)
    par1.legend(['df1 wind avg E', 'df2 wind avg E'], loc="lower right")
    plt.show()


def plot_ahrs_comparison(df1, df2, path = None):
    t = (df1.index / 100.0 / 60.0).to_numpy()   # 100Hz ticks to minutes for the time axis

    roll_deg_df1 = df1['roll'] / 2 / np.pi * 360
    pitch_deg_df1 = df1['pitch'] / 2 / np.pi * 360

    roll_deg_df2 = df2['roll'] / 2 / np.pi * 360
    pitch_deg_df2 = df2['pitch'] / 2 / np.pi * 360

    # Plot the data
    figure, axis = plt.subplots(2, 1, sharex=True)
    title = "AHRS data \n {}".format(path)
    figure.suptitle(title, size="small" )
    plt.autoscale(enable=True, axis='y')

    # Plot pitch
    axis[0,].plot(t, pitch_deg_df1.to_numpy(), "r-", linewidth=1.5)
    axis[0,].plot(t, pitch_deg_df2.to_numpy(), "g-", linewidth=1.0)
    axis[0,].legend(["df1 pitch angle", "df2 pitch angle"], loc="lower left")
    axis[0,].grid()

    # Plot roll
    axis[1,].plot(t, roll_deg_df1.to_numpy(), "c--", linewidth=1.5)
    axis[1,].plot(t, roll_deg_df2.to_numpy(), "b--", linewidth=1.0)
    axis[1,].legend(["df1 roll angle", "df2 roll angle"], loc="lower left")
    axis[1,].grid()
    plt.show()

def plot_vario_comparison(df1, df2, path = None):
    t = (df1.index / 100.0 / 60.0).to_numpy()   # 100Hz ticks to minutes for the time axis

    figure, axis = plt.subplots()
    title = "Vario \n {}".format(path)
    figure.suptitle(title, size="small")
    plt.autoscale(enable=True, axis='y')
    axis.grid()
    axis.set_xlabel('t [minutes]')

    axis.plot(t, df1['vario'].to_numpy(), "r-", linewidth=1)
    axis.plot(t, df2['vario'].to_numpy(), "g-", linewidth=1)
    axis.legend(["df1 vario", "df2 vario"], loc="lower left")
    plt.show()


if __name__ == "__main__":
    from larus_data.larus_to_df import Larus2Df
    file = os.getcwd() + '/240520_091630.f37'
    file = os.getcwd() + '/230430.f37'
    file = os.getcwd() + '/240830_short.f37'   # Stefly WM Flug

    versions = ['v0.1.0', 'v0.1.1', 'v0.1.2']

    df_v0_1_0 = Larus2Df(file, version='v0.1.0').get_df()
    #df_v0_1_1 = Larus2Df(file, version='v0.1.1').get_df()
    df_v0_1_2 = Larus2Df(file, version='v0.1.2').get_df()

    #plot_wind_comparison(df_v0_1_0, df_v0_1_2)
    plot_ahrs_comparison(df_v0_1_0, df_v0_1_2)
    plot_vario_comparison(df_v0_1_0, df_v0_1_2)


