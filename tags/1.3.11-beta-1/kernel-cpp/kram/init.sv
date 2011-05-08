//                                                    (c) SOS GmbH Berlin
//                                                        Jörg Schwiemann
#define SOURCE "init.cpp"

#ifdef _Windows
#include <svwin.h>
#include <sv.hxx>
#undef min
#undef max

#endif

#include <stdlib.h>
#include <xception.h>

//#undef _Windows         ///////////////////  Borland 4.0 / SV Inkompatibilit„t!!!

#include "init.h"

Sos_init_parameters sos_init_parameters;

void init_sos_parameters() {
  sos_init_parameters.init(); xc;
exceptions
};

Sos_init_parameters::Sos_init_parameters()
{
  _init_ok = false;
#ifdef _Windows
  pIniFile = 0;
  char buf[255];
  GetWindowsDirectory( buf, sizeof buf - 10 );
  strcat( buf, "\\sos.ini" );
  Config aSecond( String( buf ) );
  pIniFile = new Config( buf );
#endif
};

Sos_init_parameters::~Sos_init_parameters()
{
    #ifdef _Windows
        delete (Config*) pIniFile;
    #endif
}


void Sos_init_parameters::set_group( const char* aGroup ) {
    #ifdef _Windows
         ((Config *)pIniFile)->SetGroup( String( aGroup ) );
    #endif
};

void Sos_init_parameters::set( const char* item,
                               const char* value ) {
#ifdef _Windows
   ((Config *)pIniFile)->WriteKey( String( item ), String( value ) );
#endif
};    

void Sos_init_parameters::read( char* r, int len, 
                                const char* name,
                                const char* def ) {
  char str [ 256 + 1 ];

  #ifdef _Windows
      String aStr = ((Config *)pIniFile)->ReadKey( name, def );
      strcpy( str, aStr );
   #else
      // Unter DOS momentan nur Default-Wert unterstuetzt
      strcpy( str, def ); 
  #endif
  
  if ( str [0] == '%' ) {
     char* p = strchr( str + 1, '%' );
     if ( p ) {
         *p = 0;
         strupr( str + 1 ); // noetig unter Windows (DOS?), js.
         p = getenv( str + 1 );
         if ( p ) {
           strcpy( str, p );
         } else {
           raise( "INIT", name );
         };
     };
  };
  if ( strlen( str ) > len ) raise( "INIT", name );
  
  strcpy( r, str );

exceptions
};

#define READ(c,n,d) read(c,sizeof c - 1,n,d); xc;

void Sos_init_parameters::init() {
  if ( _init_ok ) return;
  
  // Liste der festverdrahteten Werte, die immer gebraucht werden!
  set_group( "host-connection" );
  READ( _host_connection.userid,           "userid",           "%USER%" );
  READ( _host_connection.account,          "account",          "1" );
  READ( _host_connection.password,         "password",         "" );
  READ( _host_connection.remtiam,          "remtiam",          "do hostware.demo.lib(remote.tiam)"
                                                             /*"exec lib=$jz.tasklib, elem=remote.tiam"*/ );
  READ( _host_connection.logon_parameters, "logon-parameters", "" );

  set_group( "fileserver" );
  READ( _fileserver.name,   "name", "pfs" );

  set_group( "tcp" );
  READ( _tcp.hostname,   "hostname", "%HOSTNAME%" );

  _init_ok = true;

exceptions
};

