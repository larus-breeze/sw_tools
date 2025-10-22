from flight_data import FlightData
from can_bus.can_frames import *

def can_legacy_protocol(data: FlightData, datagrams: CanFrames):
    # EULER ANGLES i16, i16, i16 roll nick yaw / 1/1000 rad
    datagrams.add(0x0101,
                    to_i16(data['roll_angle'] * 1000.0) +
                    to_i16(data['pitch_angle'] * 1000.0) +
                    to_i16(data['yaw_angle'] * 1000.0))

    # AIRSPEED tas, ias in km/h
    datagrams.add(0x0102,
                    to_i16(data['tas'] * 3.6) +
                    to_i16(data['ias'] * 3.6))

    # VARIO climb_rate, average_climb_rate in mm/s
    datagrams.add(0x103,
                    to_i16(data['vario'] * 1000.0) +
                    to_i16(data['vario_average'] * 1000.0))

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
                    to_i32(data['latitude'] * 10_000_000.0) +
                    to_i32(data['longitude'] * 10_000_000.0))

    # GPS altitude and geo separation
    datagrams.add(0x106,
                    to_i32(data['position_down'] * -1000.0) +
                    to_i32(data['geo_separation']))

    # GPS track and groundspeed
    datagrams.add(0x107,
                    to_i16((data['track_ground'] * pi / 180)*1000.0) +
                    to_u16(data['speed_ground'] * 3.6))
    
    # GPS number of sats & gps state
    datagrams.add(0x10a,
                    to_u8(data['sat_count']) +
                    to_u8(data['sat_fix_type']))

    # WIND wind 0.001 rad i16 km/h i16 avg wind 0.001 rad i16 km/h i16
    wind_direction = atan2(- data['wind_east'], - data['wind_north'])
    if wind_direction < 0.0:
        wind_direction += 2 * pi
    av_wind_direction = atan2(- data['wind_average_east'], - data['wind_average_north'])
    if av_wind_direction < 0.0:
        av_wind_direction += 2 * pi

    datagrams.add(0x108,
                    to_i16(wind_direction * 1000.0) +
                    to_i16(sqrt(data['wind_east'] ** 2 + data['wind_north'] ** 2) * 3.6) + \
                    to_i16(av_wind_direction * 1000.0) +
                    to_i16(sqrt(data['wind_average_east'] ** 2 + data['wind_average_north'] ** 2) * 3.6))

    # ATHMOSPHERE static pressure in Pa, air_density in g/m³
    datagrams.add(0x109,
                    to_u32(data['static_pressure']) +
                    to_u32(data['air_density'] * 1000))

    # ACCELERATION g_load in mm/s², eff_vert_acc mm/s², vario_uncomp in mm/s,
    #              circle_mode (0 straight, 1 transition, 2 circling)
    datagrams.add(0x10b,
                    to_i16(data['g_load'] * 1000.0) +
                    to_i16(data['effective_acceleration_vertical'] * -1000.0) +
                    to_i16(data['vario_uncompensated'] * -1000.0) +
                    to_u8(data['circle_mode']))

    # TURN_COORD slip_angle in rad, turn_rate in rad/s, nick_angle in rad
    datagrams.add(0x10c,
                    to_i16(data['slip_angle'] * 1000.0) +
                    to_i16(data['turn_rate'] * 1000.0) +
                    to_i16(data['apparent_pitch_angle'] * 1000.0))
    
    # STATUS
    datagrams.add(0x10d,
                    to_i32(0xffffffff) +
                    to_i32(0x03030001))

