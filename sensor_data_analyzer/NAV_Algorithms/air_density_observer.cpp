#include "embedded_math.h"
#include <air_density_observer.h>

air_data_result air_density_observer::feed_metering( float pressure, float MSL_altitude)
{
  air_data_result air_data;
  density_QNH_calculator.add_value( MSL_altitude, pressure);

  if( MSL_altitude > max_altitude)
    max_altitude = MSL_altitude;

  if( MSL_altitude < min_altitude)
    min_altitude = MSL_altitude;

  if( density_QNH_calculator.get_count() <= 5000)
    return air_data;

  if( max_altitude - min_altitude < 100.0f) // ... forget this measurement
    {
    density_QNH_calculator.reset();
    return air_data;
    }
  linear_least_square_result<double> result;
  density_QNH_calculator.evaluate(result);
  if( result.variance_slope < 3e-7)
    {
      air_data.QNH = result.y_offset;
      float density = result.slope / -9.81f;
      float pressure = density_QNH_calculator.get_mean_y();
      float std_density = 1.0496346613e-5f * pressure + 0.1671546011f;
      air_data.density_correction = density / std_density;
      air_data.valid = true;
    }
  else
      return air_data;

  density_QNH_calculator.reset();

  return air_data;
}
