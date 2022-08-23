#ifndef AIR_DENSITY_OBSERVER_H_
#define AIR_DENSITY_OBSERVER_H_

#include "Linear_Least_Square_Fit.h"

class air_data_result
{
public:
  air_data_result( void)
    : valid( false)
  {}
  float density_correction;
  float QFF;
  bool valid;
};

class air_density_observer
{
public:
  air_density_observer (void)
  {
    reset();
  }
  air_data_result feed_metering( float pressure, float MSL_altitude);

private:
    void reset(void)
    {
      min_altitude = 10000.0f;
      max_altitude = -1000.0f;
      density_QFF_calculator.reset();
    }

    linear_least_square_fit<double> density_QFF_calculator;
    float min_altitude;
    float max_altitude;
};

#endif /* AIR_DENSITY_OBSERVER_H_ */
