from flight_data import FlightData
from can_bus.can_frames import *
import math

def can_new_protocol(data: FlightData, datagrams: CanFrames):
    
    #########################################################
    # Heartbeat Sensorbox
    datagrams.add(0x520,
                  to_u16(2) +       # Object-ID / Sensorbox
                  to_u16(0) +       # Object-ID Generation
                  to_u32(0x4711))   # Device UID

    # Sensorbox hardware and firmware versions
    datagrams.add(0x521,
                  to_u8(1) +        # Manufacturer Larus
                  to_u8(1) +        # Sensorbox
                  to_u8(1) +        # Sensorbox
                  to_u8(0) +        # Sensorbox
                  to_u8(0) +        # v0.5.3.0
                  to_u8(5) +
                  to_u8(3) +
                  to_u8(0))

    # Roll Angle and Nick Angle (Front-Right-Down System)
    datagrams.add(0x0120,
                  to_f32(data['roll_angle']) +
                  to_f32(data['pitch_angle']))

    # Yaw Angle and Turn Rate
    datagrams.add(0x121,
                  to_f32(data['yaw_angle']) +
                  to_f32(data['turn_rate']))

    # TAS (True Airspeed) and IAS (Indicated Airspeed)
    datagrams.add(0x122,
                  to_f32(data['tas']) +
                  to_f32(data['ias']))
    
    # Vario and Vario Avarage
    datagrams.add(0x123,
                  to_f32(data['vario']) +
                  to_f32(data['vario_average']))

    # WIND calculations
    wind_direction = atan2(- data['wind_east'], - data['wind_north'])
    if wind_direction < 0.0:
        wind_direction += 2 * pi
    wind_speed = sqrt(data['wind_east'] ** 2 + data['wind_north'] ** 2)

    # Wind Direction and Wind Speed
    datagrams.add(0x124,
                  to_f32(wind_direction) + 
                  to_f32(wind_speed))
    
    # average WIND calculations
    av_wind_direction = atan2(- data['wind_average_east'], - data['wind_average_north'])
    if av_wind_direction < 0.0:
        av_wind_direction += 2 * pi
    av_wind_speed = sqrt(data['wind_average_east'] ** 2 + data['wind_average_north'] ** 2)

    # Avarage Wind Direction and Avarage Wind Speed
    datagrams.add(0x125,
                  to_f32(av_wind_direction) + 
                  to_f32(av_wind_speed))

    # Ambient Pressure and Air Density
    datagrams.add(0x126,
                  to_f32(data['static_pressure']) +
                  to_f32(data['air_density']))
    
    # G Force and Vertical G Force
    datagrams.add(0x127,
                  to_f32(data['g_load']) +
                  to_f32(data['effective_acceleration_vertical']))
    
    # Calculated Slip Angle and Pitch Angle
    datagrams.add(0x128,
                  to_f32(data['slip_angle']) +
                  to_f32(data['apparent_pitch_angle']))
    
    # Supply Voltage and Circle Mode
    datagrams.add(0x129,
                  to_f32(data['voltage']) +
                  to_u8(data['circle_mode']))
    
    # System State and Git Tag
    datagrams.add(0x12a,
                    to_u32(0x0000_0011) +
                    to_u32(0x03030001))

    #########################################################
    # Heartbeat GPS Device
    datagrams.add(0x540,
                  to_u16(3) +       # Object-ID / GPS
                  to_u16(0) +       # Object-ID Generation
                  to_u32(0x4711))   # Device UID
    
    # Date and Time
    datagrams.add(0x140,
                  to_u16(data['year'] + 2000) + 
                  to_u8(data['month']) + 
                  to_u8(data['day']) + 
                  to_u8(data['hour']) + 
                  to_u8(data['minute']) + 
                  to_u8(data['second']))

    # Latitude
    datagrams.add(0x141,
                  to_f64(data['latitude']*math.pi/180))
    
    # Longitude
    datagrams.add(0x142,
                  to_f64(data['longitude']*math.pi/180))
    
    # MSL Altitude and Geo Separation
    datagrams.add(0x143,
                  to_f32(-data['position_down']) +
                  to_f32(data["geo_separation"]*0.1))

    # Ground Track and Ground Speed
    datagrams.add(0x144,
                  to_f32(data['track_ground']*math.pi/180) +
                  to_f32(data['speed_ground']))
    
    # Number of Sattelites and Sat Fix Type
    datagrams.add(0x145,
                  to_u8(data['sat_count']) +
                  to_u8(data['sat_fix_type']))
