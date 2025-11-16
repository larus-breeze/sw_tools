#include "helper.h"
#include "soft_iron_compensator.h"
#include <iostream>

soft_iron_compensator_t soft_iron_compensator;
void trigger_soft_iron_compensator_calculation()
{
  soft_iron_compensator.calculate();
  printf( "soft iron compensation done \n");
}