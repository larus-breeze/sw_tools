#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "time.h"
#include "math.h"
#include <fenv.h>
#include "ascii_support.h"

float random_float(float min, float max);

static float extra[] = { 0.0, 1.0, 2.0, 10.0, -10.0, -1.0, -2.0, -0.0000001, 0.0000001};

void   atof_test(void)
{
  char buffer[100];
  double maxi = 0;
  double mini = 100;

  for( unsigned i = 0; i < sizeof( extra) / sizeof( float); ++i)
    {
      float test = extra[i];
      char * end = my_ftoa( buffer, test);
      double result = my_atof( buffer);

      double factor = test / result;

      printf( "%e -> %s  = %10.10f\n", test, buffer, factor);
    }

  for( unsigned i=0; i< 100000000; ++i)
    {
      float test = random_float( -10, 10);
      if( abs(test) < 0.00000001)
	continue;
      test = expf( test);
      char * end = my_ftoa( buffer, test);
      double result = my_atof( buffer);

      double factor = test / result;

      //      printf( "%e -> %s  = %10.10f\n", test, buffer, factor);
      if( factor > maxi)
	{
	maxi = factor;
	printf( "%e -> %s  = %10.10f\n", test, buffer, factor);
	}

      if( factor < mini)
	mini = factor;
    }

  for( unsigned i=0; i< 1000000; ++i)
    {
      float test = random_float( -1, 1);
      test = expf( test);
      char * end = my_ftoa( buffer, test);
      double result = my_atof( buffer);

      double factor = test / result;

      //      printf( "%e -> %s  = %10.10f\n", test, buffer, factor);
      if( factor > maxi)
	{
	maxi = factor;
	printf( "%e -> %s  = %10.10f\n", test, buffer, factor);
	}

      if( factor < mini)
	mini = factor;
    }

  float test = 0.000001f;
  for( unsigned i=0; i< 1000; ++i)
    {
      char * end = my_ftoa( buffer, test);
      assert(( end - buffer) < 15);

      double result = my_atof( buffer);

      double factor = test / result;
      if( 0 == strcmp( buffer, "0.0"))
	factor = test + 1.0f;

      //      printf( "%e -> %s  = %10.10f\n", test, buffer, factor);
      if( factor > maxi)
	{
	maxi = factor;
	printf( "%e -> %s  = %10.10f\n", test, buffer, factor);
	}

      if( factor < mini)
	{
	mini = factor;
	printf( "%e -> %s  = %10.10f\n", test, buffer, factor);
	}
      test /= 1.99999f;
    }

  printf( "maximal = %10.10f  minimal = %10.10f\n", maxi, mini);
}

