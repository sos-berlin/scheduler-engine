// init.h                                               (c) SOS GmbH Berlin

#ifndef __INIT_H
#define __INIT_H

#include "sysdep.h"

struct Sos_init_parameters {
  Sos_init_parameters();
  ~Sos_init_parameters();

  struct Host_connection {
    char userid[8+1];
    char account[8+1];
    char password[19+1];
    char logon_parameters[100+1];
    char remtiam[100+1];
  };

  struct Fileserver {
    char name[8+1];
  };

  struct Tcp {
    char hostname[15+1];
  };

  const char* sos_filename();

  // Methoden
  void init();

  int init_ok() { return _init_ok; };

  const char* getenv( const char* );

  void set_group( const char* group );

  void read( char* r, int len,
             const char* name,
             const char* def = "" );

  unsigned int read_int( const char* name, const unsigned int def );

  int check_flag( const char* name, int def = 0 );

  void set( const char* item, 
            const char* value );

  // Groups
  Host_connection _host_connection;
  Fileserver      _fileserver;
  Tcp             _tcp;


private:
  int _init_ok;
  int _is_demo;
  int _file_exists( const char* );
  char _demo_path[256];

#if defined(SYSTEM_SOLARIS)
  void* pIniFile;
#endif  

#if defined(SYSTEM_WIN)
//StarView      void* pIniFile;
  char group[256];
  char filename[256];
#endif
};

void init_sos_parameters();


extern Sos_init_parameters sos_init_parameters;

#endif

