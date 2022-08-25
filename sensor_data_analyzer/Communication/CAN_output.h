/** ***********************************************************************
 * @file		CAN_output.h
 * @brief		format internal data and send to CAN
 * @author		Dr. Klaus Schaefer
 **************************************************************************/
#ifndef SRC_CAN_OUTPUT_H_
#define SRC_CAN_OUTPUT_H_

#include "system_configuration.h"

#include "navigator.h"
#include "flight_observer.h"
#include "NMEA_format.h"

#ifdef UNIX
#include "stdint.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include "stdio.h"
#include "sstream"
#include "stdlib.h"
#include "string.h"

//! basic CAN packet type
typedef struct
{
  uint16_t id; 	//!< identifier
  uint16_t dlc; //!< data length code
  union
  {
		uint8_t  data_b[8]; 	//!< data seen as 8 times uint8_t
		int8_t   data_sb[8]; 	//!< data seen as 8 times int8_t
		uint16_t data_h[4]; 	//!< data seen as 4 times uint16_t
		int16_t  data_sh[4]; 	//!< data seen as 4 times int16_t
		uint32_t data_w[2]; 	//!< data seen as 2 times uint32_t
		int32_t  data_sw[2];	//!< data seen as 2 times int32_t
		float    data_f[2]; 	//!< data seen as 2 times 32-bit floats
		uint64_t data_l;    	//!< data seen as 64-bit integer
  };
} CANpacket;
class CAN_driver_t
{
public:
  bool send( CANpacket p, int dummy)
  {
    std::cout << std::hex << "$CAN " << p.id;

    std::cout << ' ' << std::setw(2) ;

    unsigned i = 0;
    while( p.dlc--)
      std::cout << std::hex <<  (unsigned)(p.data_b[i++]);

    std::cout << std::endl;

    return true;
  }
};

extern CAN_driver_t CAN_driver;
void CAN_output ( const output_data_t &x);

#else
#include "candriver.h"
extern Task CAN_task;

void trigger_CAN(void)
{
  CAN_task.notify_give();
}

#endif

#endif /* SRC_CAN_OUTPUT_H_ */
