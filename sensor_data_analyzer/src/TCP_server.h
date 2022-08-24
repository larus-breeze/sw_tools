#ifndef TCP_SERVER_H_
#define TCP_SERVER_H_

bool open_TCP_port(void);
bool wait_and_accept_TCP_connection( void);
void write_TCP_port( char * data, unsigned length);
void close_TCP_port(void);

#endif /* TCP_SERVER_H_ */
