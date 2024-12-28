#!/user/bin/env python3
import os
import numpy as np
import matplotlib.pyplot as plt
from larus_data.larus_to_df import Larus2Df

class LarusLibComparison:
    def __init__(self, binaryfile, version1, version2):
        self.file = binaryfile
        self.version1 = version1
        self.version2 = version2
        self.df1 = Larus2Df(self.file, version=self.version1).get_df()
        self.df2 = Larus2Df(self.file, version=self.version2).get_df()
        self.t = (self.df1.index / 100.0 / 60.0).to_numpy()

    def plot_mag_comparison(self):
        t = (self.df1.index / 100.0 / 60.0).to_numpy()  # 100Hz ticks to minutes for the time axis
        #df = self.df1
        nav_ind_abs_df1 = np.sqrt(
            np.square(self.df1['nav ind mag N']) + np.square(self.df1['nav ind mag E']) + np.square(self.df1['nav ind mag D']))

        nav_ind_abs_df2 = np.sqrt(
            np.square(self.df2['nav ind mag N']) + np.square(self.df2['nav ind mag E']) + np.square(
                self.df2['nav ind mag D']))

        # Plot the data
        figure, axis = plt.subplots()
        title = "Induction in earth system comparison between {} and {}".format(self.version1, self.version2)
        figure.suptitle(title, size="small")
        plt.autoscale(enable=True, axis='y')
        axis.grid()
        axis.set_xlabel('t [minutes]')

        axis.plot(t, nav_ind_abs_df1.to_numpy(), "m", linewidth=1.5)
        axis.plot(t, nav_ind_abs_df2.to_numpy(), "k--", linewidth=0.5)
        axis.legend(["nav ind abs {}".format(self.version1), "nav ind abs {}".format(self.version2)], loc="lower left")

        par1 = axis.twinx()
        par1.plot(t, self.df1['nav ind mag N'].to_numpy(), "r", linewidth=1.5)
        par1.plot(t, self.df2['nav ind mag N'].to_numpy(), "k--", linewidth=0.5)

        par1.plot(t, self.df1['nav ind mag E'].to_numpy(), "g", linewidth=1.5)
        par1.plot(t, self.df2['nav ind mag E'].to_numpy(), "k--", linewidth=0.5)

        par1.plot(t, self.df1['nav ind mag D'].to_numpy(), "b", linewidth=1.5)
        par1.plot(t, self.df2['nav ind mag D'].to_numpy(), "k--", linewidth=0.5)

        minimum = self.df1['nav ind mag N'].min()
        maximum = self.df1['nav ind mag N'].max()
        if self.df1['nav ind mag E'].min() < minimum:
            minimum = self.df1['nav ind mag E'].min()
        if self.df1['nav ind mag E'].max() > maximum:
            maximum = self.df1['nav ind mag E'].max()
        if self.df1['nav ind mag D'].min() < minimum:
            minimum = self.df1['nav ind mag D'].min()
        if self.df1['nav ind mag D'].max() > maximum:
            maximum = self.df1['nav ind mag D'].max()
        par1.set_ylim(minimum - 0.5, maximum + 1.5)
        par1.legend(['nav ind mag N', 'nav ind mag E', 'nav ind mag D'], loc="lower right")
        plt.show()


    def plot_wind_comparison(self):
        figure, axis = plt.subplots(2, 1, sharex=True)
        title = "Wind data comparison between {} and {}".format(self.version1, self.version2)
        figure.suptitle(title, size="small")

        # Wind instantaneous comparison
        axis[0,].plot(self.t, self.df1['wind N'].to_numpy(), "r", linewidth=1.5)
        axis[0,].plot(self.t, self.df2['wind N'].to_numpy(), "k--", linewidth=0.5)
        axis[0,].legend(["df1 wind N", "df2 wind N"], loc="lower left")
        axis[0,].grid()
        par1 = axis[0,].twinx()
        par1.plot(self.t, self.df1['wind E'].to_numpy(), "c", linewidth=1.5)
        par1.plot(self.t, self.df2['wind E'].to_numpy(), "k--", linewidth=0.5)
        par1.legend(['df1 wind E', 'df2 wind E'], loc="lower right")

        # Wind average comparison
        axis[1,].plot(self.t, self.df1['wind avg N'].to_numpy(), "m", linewidth=1.5)
        axis[1,].plot(self.t, self.df2['wind avg N'].to_numpy(), "k--", linewidth=0.5)
        axis[1,].legend(["df1 wind avg N", "df2 wind avg N"], loc="lower left")
        axis[1,].grid()
        par1 = axis[1,].twinx()
        par1.plot(self.t, self.df1['wind avg E'].to_numpy(), "y", linewidth=1.5)
        par1.plot(self.t, self.df2['wind avg E'].to_numpy(), "k--", linewidth=0.5)
        par1.legend(['df1 wind avg E', 'df2 wind avg E'], loc="lower right")
        plt.show()


    def plot_ahrs_comparison(self):
        roll_deg_df1 = self.df1['roll'] / 2 / np.pi * 360
        pitch_deg_df1 = self.df1['pitch'] / 2 / np.pi * 360

        roll_deg_df2 = self.df2['roll'] / 2 / np.pi * 360
        pitch_deg_df2 = self.df2['pitch'] / 2 / np.pi * 360

        # Plot the data
        figure, axis = plt.subplots(2, 1, sharex=True)
        title = "AHRS data comparison between {} and {}".format(self.version1, self.version2)
        figure.suptitle(title, size="small" )
        plt.autoscale(enable=True, axis='y')

        # Plot pitch
        axis[0,].plot(self.t, pitch_deg_df1.to_numpy(), "r", linewidth=1.5)
        axis[0,].plot(self.t, pitch_deg_df2.to_numpy(), "k--", linewidth=0.5)
        axis[0,].legend(["df1 pitch angle", "df2 pitch angle"], loc="lower left")
        axis[0,].grid()

        # Plot roll
        axis[1,].plot(self.t, roll_deg_df1.to_numpy(), "c", linewidth=1.5)
        axis[1,].plot(self.t, roll_deg_df2.to_numpy(), "k--", linewidth=0.5)
        axis[1,].legend(["df1 roll angle", "df2 roll angle"], loc="lower left")
        axis[1,].grid()
        plt.show()

    def plot_vario_comparison(self):
        figure, axis = plt.subplots()
        title = "Vario comparison between {} and {}".format(self.version1, self.version2)
        figure.suptitle(title, size="small")
        plt.autoscale(enable=True, axis='y')
        axis.grid()
        axis.set_xlabel('t [minutes]')

        axis.plot(self.t, self.df1['vario'].to_numpy(), "r", linewidth=1.5)
        axis.plot(self.t, self.df2['vario'].to_numpy(), "k--", linewidth=0.5)
        axis.legend(["{} vario".format(self.version1), "{} vario".format(self.version2)], loc="lower left")
        plt.show()

    def plot_air_density_comparison(self):
        figure, axis = plt.subplots()
        title = "Air Density comparison between {} and {}".format(self.version1, self.version2)
        figure.suptitle(title, size="small")
        plt.autoscale(enable=True, axis='y')
        axis.grid()
        axis.set_ylabel('altitude [m]')

        axis.plot(self.df1['Air Density'].to_numpy(),- self.df1['pos DWN'], "r", linewidth=1.5)
        axis.plot(self.df2['Air Density'].to_numpy(), - self.df2['pos DWN'], "k--", linewidth=0.5)
        axis.legend(["{} Air Density".format(self.version1), "{} Air Density".format(self.version2)], loc="lower left")
        plt.show()


if __name__ == "__main__":
    from larus_data.larus_to_df import Larus2Df
    #file = os.getcwd() + '/240520_091630.f37'
    #file = os.getcwd() + '/230430.f37'
    file = os.getcwd() + '/240830_short.f37'   # Stefly WM Flug

    #available versions 'v0.1.0', 'v0.1.1', 'v0.1.2'
    cmp = LarusLibComparison(file,'v0.1.0', 'v0.1.2' )

    cmp.plot_mag_comparison()
    cmp.plot_wind_comparison()
    cmp.plot_vario_comparison()
    cmp.plot_ahrs_comparison()
    cmp.plot_air_density_comparison()




