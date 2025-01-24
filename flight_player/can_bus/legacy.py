from flight_data import FlightData
from can_bus.data import *

def can_legacy_protocol(data: FlightData, datagrams: CanData):
    # EULER ANGLES i16, i16, i16 roll nick yaw / 1/1000 rad
    datagrams.add(0x0101,
                    to_i16(data['roll'] * 1000.0) +
                    to_i16(data['pitch'] * 1000.0) +
                    to_i16(data['yaw'] * 1000.0))

    # AIRSPEED tas, ias in km/h
    datagrams.add(0x0102,
                    to_i16(data['TAS'] * 3.6) +
                    to_i16(data['IAS'] * 3.6))

    # VARIO climb_rate, average_climb_rate in mm/s
    datagrams.add(0x103,
                    to_i16(data['vario'] * 1000.0) +
                    to_i16(data['vario integrator'] * 1000.0))

    # GPS date and time
    datagrams.add(0x104,
                    to_u8(data['year']) + 
                    to_u8(data['month']) + 
                    to_u8(data['day']) + 
                    to_u8(data['hour']) + 
                    to_u8(data['minute']) + 
                    to_u8(data['second']))

    # GPS lat, lon
    datagrams.add(0x105,
                    to_i32(data['Lat'] * 10_000_000.0) + 
                    to_i32(data['Long'] * 10_000_000.0))

    # GPS altitude and geo separation
    datagrams.add(0x106,
                    to_i32(data['pos DWN'] * -1000.0) +
                    to_i32(data["geo separation dm"]))

    # GPS track and groundspeed
    datagrams.add(0x107,
                    to_i16((data['track GNSS'] * pi / 180)*1000.0) +
                    to_u16(data['speed GNSS'] * 3.6))
    
    # GPS number of sats & gps state
    datagrams.add(0x10a,
                    to_u8(data['sat number']) + 
                    to_u8(data['sat fix type']))

    # WIND wind 0.001 rad i16 km/h i16 avg wind 0.001 rad i16 km/h i16
    wind_direction = atan2(- data['wind E'], - data['wind N'])
    if wind_direction < 0.0:
        wind_direction += 2 * pi
    av_wind_direction = atan2(- data['wind avg E'], - data['wind avg N'])
    if av_wind_direction < 0.0:
        av_wind_direction += 2 * pi

    datagrams.add(0x108,
                    to_i16(wind_direction * 1000.0) +
                    to_i16(sqrt(data['wind E'] ** 2 + data['wind N'] ** 2) * 3.6) + \
                    to_i16(av_wind_direction * 1000.0) +
                    to_i16(sqrt(data['wind avg E'] ** 2 + data['wind avg N'] ** 2) * 3.6))

    # ATHMOSPHERE static pressure in Pa, air_density in g/m³
    datagrams.add(0x109,
                    to_u32(data['static p']) +
                    to_u32(data['Air Density'] * 1000))

    # ACCELERATION g_load in mm/s², eff_vert_acc mm/s², vario_uncomp in mm/s,
    #              circle_mode (0 straight, 1 transition, 2 circling)
    datagrams.add(0x10b,
                    to_i16(data['G_load'] * 1000.0) +
                    to_i16(data['acc vertical'] * -1000.0) +
                    to_i16(data['vario uncomp'] * -1000.0) +
                    to_u8(data['circle mode']))

    # TURN_COORD slip_angle in rad, turn_rate in rad/s, nick_angle in rad
    datagrams.add(0x10c,
                    to_i16(data['slip angle'] * 1000.0) +
                    to_i16(data['turn rate'] * 1000.0) +
                    to_i16(data['pitch angle'] * 1000.0))
    
    # STATUS
    datagrams.add(0x10d,
                    to_i32(0xffffffff) +
                    to_i32(0x03030001))

