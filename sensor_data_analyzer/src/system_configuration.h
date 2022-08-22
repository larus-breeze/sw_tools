/**
 * @file    system_configuration.h
 * @brief   system-wide tweaks
 * @author  Dr. Klaus Schaefer klaus.schaefer@h-da.de
 */
#ifndef SRC_SYSTEM_CONFIGURATION_H_
#define SRC_SYSTEM_CONFIGURATION_H_

#define UNIX 1

#define WITH_DENSITY_DATA	1
#define PARALLEL_MAGNETIC_AHRS 	1
#define INCLUDING_NANO	 	1

#define GNSS_VERTICAL_SPEED_INVERTED 0

#define MINIMUM_MAG_CALIBRATION_SAMPLES 6000
#define MAG_CALIB_LETHARGY	0.8f // percentage of remaining old calibration info
#define MAG_CALIBRATION_CHANGE_LIMIT 5.0e-4f // variance average of changes: 3 * { offset, scale }

#define CIRCLE_LIMIT 		(10 * 100) //!< 10 * 1/100 s delay into / out of circling state
#define STABLE_CIRCLING_LIMIT	(30 * 100) // seconds @ 100 Hz for MAG auto calibration

#endif /* SRC_SYSTEM_CONFIGURATION_H_ */
