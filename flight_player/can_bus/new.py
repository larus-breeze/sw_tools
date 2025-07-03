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

    # Roll Angle and Nick Angle (Front-Right-Down System)
    datagrams.add(0x0120,
                  to_f32(data['roll']) +
                  to_f32(data['pitch']))

    # Yaw Angle and Turn Rate
    datagrams.add(0x121,
                  to_f32(data['yaw']) +
                  to_f32(data['turn rate']))

    # TAS (True Airspeed) and IAS (Indicated Airspeed)
    datagrams.add(0x122,
                  to_f32(data['TAS']) + 
                  to_f32(data['IAS']))
    
    # Vario and Vario Avarage
    datagrams.add(0x123,
                  to_f32(data['vario']) +
                  to_f32(data['vario integrator']))

    # WIND calculations
    wind_direction = atan2(- data['wind E'], - data['wind N'])
    if wind_direction < 0.0:
        wind_direction += 2 * pi
    wind_speed = sqrt(data['wind E'] ** 2 + data['wind N'] ** 2)

    # Wind Direction and Wind Speed
    datagrams.add(0x124,
                  to_f32(wind_direction) + 
                  to_f32(wind_speed))
    
    # average WIND calculations
    av_wind_direction = atan2(- data['wind avg E'], - data['wind avg N'])
    if av_wind_direction < 0.0:
        av_wind_direction += 2 * pi
    av_wind_speed = sqrt(data['wind avg E'] ** 2 + data['wind avg N'] ** 2)

    # Avarage Wind Direction and Avarage Wind Speed
    datagrams.add(0x125,
                  to_f32(av_wind_direction) + 
                  to_f32(av_wind_speed))

    # Ambient Pressure and Air Density
    datagrams.add(0x126,
                  to_f32(data['static p']) + 
                  to_f32(data['Air Density']))
    
    # G Force and Vertical G Force
    datagrams.add(0x127,
                  to_f32(data['G_load']) +
                  to_f32(data['acc vertical']))
    
    # Calculated Slip Angle and Pitch Angle
    datagrams.add(0x128,
                  to_f32(data['slip angle']) +
                  to_f32(data['pitch angle']))
    
    # Supply Voltage and Circle Mode
    datagrams.add(0x129,
                  to_f32(data['ubatt']) +
                  to_u8(data['circle mode']))
    
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
                  to_f64(data['Lat']*math.pi/180))
    
    # Longitude
    datagrams.add(0x142,
                  to_f64(data['Long']*math.pi/180))
    
    # MSL Altitude and Geo Separation
    datagrams.add(0x143,
                  to_f32(-data['pos DWN']) +
                  to_f32(data["geo separation dm"]*0.1))

    # Ground Track and Ground Speed
    datagrams.add(0x144,
                  to_f32(data['track GNSS']*math.pi/180) +
                  to_f32(data['speed GNSS']))
    
    # Number of Sattelites and Sat Fix Type
    datagrams.add(0x145,
                  to_u8(data['sat number']) + 
                  to_u8(data['sat fix type']))
