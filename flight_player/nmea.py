import socket, datetime, math

from flight_data import FlightData

# This class defines some NMEA Strings

class Nmea():
    def __init__(self, nmea_port: int):
        """Initialize the clas with given UDP port"""
        self._port = nmea_port
        self._ip = '127.0.0.1'
        self._socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # Internet, UDP

    def send_data(self, data):
        """Send all NMEA data"""
        self.send_gpgga(data)
        self.send_gprmc(data)

    def send_gpgga(self, data: FlightData):
        """Send a NMEA $GPGGA sentence"""
        self.send(
                "$GPGGA",
                f"{data.time():%H%M%S.%f}"[:-4],
                Nmea.lat_to_str(data['latitude']),
                Nmea.lon_to_str(data['longitude']),
                f"{data['sat_fix_type']:01.0f}",
                f"{data['sat_count']:02.0f}",
                "1.0",
                f"{-data['position_down']:06.1f}",
                "M",
                "00.0",
                "M",
                "",
                "",
        )
        
    def send_gprmc(self, data: FlightData):
        """Send a NMEA $GPRMC sentence"""
        self.send(
                "$GPRMC",
                f"{data.time():%H%M%S.%f}"[:-4],
                "A",
                Nmea.lat_to_str(data['latitude']),
                Nmea.lon_to_str(data['longitude']),
                f"{data['speed_ground']*1.94384:05.1f}",
                f"{data['track_ground']:05.1f}",
                f"{data.date_of_flight():%d%m%y}",
                "",
                "",
                "A"
        )
        
    def send(self, *dps):
        """Sent through selected channel"""
        composed = ','.join(dps)
        cs = 0
        for c in composed[1:]:
            cs ^= ord(c)
        nmea_data =  bytes(f"{composed}*{cs:02X}\r\n", "ascii")

        self._socket.sendto(nmea_data, (self._ip, self._port))

    @staticmethod
    def lat_to_str(coord: float):
        """Convert float coordinate into NMEA ascii string (Latitude)"""
        if coord>0:
            coord = coord
            sign = 'N'
        else:
            coord *= -1.0
            sign = 'S'

        fract, deg = math.modf(coord)
        return f"{deg:0.0f}{fract*60.0:08.5f},{sign}"

    @staticmethod
    def lon_to_str(coord: float):
        """Convert float coordinate into NMEA ascii string (Longitude)"""
        return Nmea.lat_to_str(coord).replace('N', 'E').replace('S', 'W')

