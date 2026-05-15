#include "flexible_file_format.h"
#include <flexible_log_file_implementation.h>
#include "CRC16.h"

bool flexible_log_file_implementation_t::open (char *file_name)
{
  outfile.open( file_name, ios::out | ios::binary | ios::ate);
  if ( not outfile.is_open ())
    return false;
  return true;
}

bool flexible_log_file_implementation_t::close( void)
{
  outfile.write( (const char *)flexible_log_file_t::buffer, (flexible_log_file_t::write_pointer - flexible_log_file_t::buffer) * sizeof( uint32_t));
  outfile.close ();
  return true;
}

bool flexible_log_file_implementation_t::write_block (uint32_t *p_data, uint32_t size_words)
{
  if ( not outfile.is_open ())
    return false;

  if( write_pointer + size_words > buffer_end)
    {
      unsigned part_length = buffer_end - write_pointer;
      unsigned remaining_length = size_words - part_length;
      while( part_length --)
	*write_pointer++ = *p_data++;
      outfile.write( (const char *)buffer, (buffer_end - buffer) * sizeof( uint32_t));
      write_pointer = buffer;
      while( remaining_length--)
	*write_pointer++ = *p_data++;
    }
  else
    {
      while( size_words --)
	*write_pointer++ = *p_data++;
    }
  return true;
}
