#include "stdio.h"
#include "stdint.h"
#include "signal_flight_event.h"

const char *event_name[] =
      { "NO_EVENT",
	"MAG_CALIBRATION_DONE",
	"EXT_MAG_CALIBRATION_DONE",
	"AIR_DENSITY_MODIFIED",
	"EEPROM_CONFIGURATION_CHANGED",
	"CAN_COMMAND_RECEIVED",
	"DEBUGGER_EVENT",
	"ACCELERATION_CALIBRATION_DONE",
	"GNSS_LOG"
      };
const char *communicator_command[] =
  { "NO_COMMAND",
    "MEASURE_CALIB_LEFT",
    "MEASURE_CALIB_RIGHT",
    "MEASURE_CALIB_LEVEL",
    "SET_SENSOR_ROTATION",
    "FINE_TUNE_CALIB",
    "SOME_EEPROM_VALUE_HAS_CHANGED"
  };
const char *debug_point_text[] =
  { "UNSPECIFIED_ERROR",
    "GNSS_TIMEOUT_HALF",
    "GNSS_TIMEOUT_FULL",
    "GNSS_DATA_ERROR"
  };

void
print_flight_event_info (uint32_t debug_data, unsigned time)
{
  switch (debug_data & 0xff)
    {
    case MAG_CALIBRATION_DONE:
    case EXT_MAG_CALIBRATION_DONE:
    case AIR_DENSITY_MODIFIED:
    case EEPROM_CONFIGURATION_CHANGED:
    case ACCELERATION_CALIBRATION_DONE:
      printf ("@ %d: Event: %s\n", time, event_name[debug_data & 0xff]);
      return;

    case CAN_COMMAND_RECEIVED:
      printf ("@ %d: CAN command %s\n", time, communicator_command[debug_data >> 8]);
      return;

    case GNSS_LOG:
      printf ("@ %d: GNSS log: %04x\n", time, debug_data >> 16);
      return;

    case DEBUGGER_DATA:
      {
	unsigned debug_point = (debug_data >> 8) & 0xff;
	if (debug_point <= (sizeof(debug_point_text) / sizeof(char*)))
	  {
	    printf ("@ %d: Debug: %s value: %04x\n", time, debug_point_text[debug_point], debug_data >> 16);
	    return;
	  }
      }
      break;

    case NO_EVENT:
    default:
      break;
    }

  printf ("@ %d: Unknown event: %08x\n", time, debug_data);
}

