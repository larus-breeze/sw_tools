/*
 * EEPROM_emulation.h
 *
 *  Created on: May 31, 2022
 *      Author: schaefer
 */

#ifndef EEPROM_EMULATION_H_
#define EEPROM_EMULATION_H_

#include "persistent_data.h"
float configuration( EEPROM_PARAMETER_ID id);

struct config_param_type
{
	unsigned identifier;
	float value;
};

extern config_param_type config_parameters[EEPROM_PARAMETER_ID_END];

#endif /* EEPROM_EMULATION_H_ */
