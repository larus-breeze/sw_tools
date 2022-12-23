/*
 * old_data_structures.h
 *
 *  Created on: Nov 5, 2022
 *      Author: schaefer
 */

#ifndef OLD_DATA_STRUCTURES_H_
#define OLD_DATA_STRUCTURES_H_

#include "quaternion.h"
#include "GNSS.h"

#pragma pack(push, 1)

typedef struct
{
  float3vector acc;   //XSENSE MTi1 IMU
  float3vector gyro;  //XSENSE MTi1 IMU
  float3vector mag;   //XSENSE MTi1 IMU
  float3vector lowcost_acc;
  float3vector lowcost_gyro;
  float3vector lowcost_mag;
  float pitot_pressure;
  float static_pressure;
  float absolute_pressure;  //this is the second ms5611 on the PCB.
  float static_sensor_temperature;  //log temperature to monitor temperature in enclosure
  float absolute_sensor_temperature;
  float supply_voltage;  //Measuring the supply voltage. Might be related to sensor noise.
} old_measurement_data_t;

typedef struct
{
  float3vector position;  	//!< NED / meters
  float3vector velocity;  	//!< NED / m/s
  float3vector acceleration;  	//!< NED / m/s^2 (from velocity delta)
  float  heading_motion;	// degrees
  float  speed_motion;		// m/s
  float3vector relPosNED;	//
  float relPosHeading;
  float speed_acc;		// speed accuracy m/s
  double latitude;		//!< degrees
  double longitude;		//!< degrees
//  uint32_t time; 		// time of day / ms
//  uint32_t date; 		// calendar date 1000*year + day of year

  uint8_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;

  uint8_t minute;
  uint8_t second;
  int16_t geo_sep_dm;	// (WGS ellipsoid height - elevation MSL) in 0.1m units
} old_coordinates_t;

typedef struct
{
  old_measurement_data_t m;
  old_coordinates_t c;
} old_input_data_t;

#endif /* OLD_DATA_STRUCTURES_H_ */
