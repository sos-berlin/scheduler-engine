#include "precomp.h"
// sosuser.cxx
// js 12.3.95

#include "../kram/sysdep.h"

#if defined SYSTEM_UNIX

#include "../kram/sosstrng.h"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include <unistd.h>             // getuid()   (jedenfalls in Linux)
#include <pwd.h>
#include "../kram/sos.h"
#include "../kram/log.h"
#include "../kram/sosuser.h"

using namespace std;
namespace sos {

static Bool __name_enabled = true;

void __sos_user_name_enable( Bool b ) { __name_enabled = b; };

Bool __is_sos_user_name_enabled() { return __name_enabled; };


const Sos_string Sos_user::name()
{
/*    char buf[LOGNAME_MAX+1];
    if ( getlogin_r( buf, sizeof buf ) != NULL )
      return Sos_string( buf ); 
    else return( Sos_string( "unknown" ) );
*/
   const struct passwd* passwd_ptr = getpwuid( getuid() );
   //jz 14.3.96 LOG(" Sos_user::name() returns " << ( passwd_ptr? passwd_ptr->pw_name : "" ) << "\"\n" );
   return passwd_ptr? passwd_ptr->pw_name : "";
/*
    #if defined SYSTEM_GNU
        int SOS_USER_NAME_NICHT_IMPLEMENTIERT;
        return Sos_string( "user" );
     #else
        if ( !__name_enabled ) return Sos_string( "" );
        //char buf[L_cuserid];
        //cuserid( buf );
        return Sos_string( getlogin() );
    #endif
*/
}

const Sos_string Sos_user::fullname( const char* username )
{
#if defined SYSTEM_GNU
   const struct passwd* passwd_ptr = *username? getpwnam( username ) : getpwuid( getuid() );
   if ( passwd_ptr != NULL ) return Sos_string( passwd_ptr->pw_name );
   else return Sos_string( "" );
   //int SOS_USER_FULLNAME_NICHT_IMPLEMENTIERT;
   //return Sos_string( "" );
#else
   char uname[LOGNAME_MAX+1];

   if ( strlen( username ) == 0 )
   {
     strcpy( uname, c_str(Sos_user::name()) ); 
   } else {
     strcpy( uname, username );  int LAENGE_VON_USERNAME_UNGEPRUEFT;
   }

   char buf[1024];
   struct passwd* passwd_ptr;
   passwd_ptr = getpwnam_r( uname, passwd_ptr, buf, sizeof buf );

   if ( passwd_ptr != NULL ) return Sos_string( passwd_ptr->pw_gecos ); 
   else return Sos_string( "" );
#endif
}

const unsigned int Sos_user::uid( const char* username )
{
#if defined SYSTEM_GNU
    return getuid();
    //int SOS_USER_UID_NICHT_IMPLEMENTIERT;
    //return -1;
#else
   char uname[LOGNAME_MAX+1];

   if ( strlen( username ) == 0 )
   {
     strcpy( uname, c_str(Sos_user::name()) ); 
   } else {
     strcpy( uname, username );
   }

   char buf[1024];
   struct passwd* passwd_ptr;
   passwd_ptr = getpwnam_r( uname, passwd_ptr, buf, sizeof buf );
   unsigned int erg = -1;

   if ( passwd_ptr != NULL ) erg = passwd_ptr->pw_uid; 
   else return erg;
#endif
}

const unsigned int Sos_user::gid( const char* username )
{
#if defined SYSTEM_GNU
    return getgid();
#else
   char uname[LOGNAME_MAX+1];

   if ( strlen( username ) == 0 )
   {
     strcpy( uname, c_str(Sos_user::name()) ); 
   } else {
     strcpy( uname, username );
   }

   char buf[1024];
   struct passwd* passwd_ptr;
   passwd_ptr = getpwnam_r( uname, passwd_ptr, buf, sizeof buf );
   unsigned int erg = -1;
   
   if ( passwd_ptr != NULL ) erg = passwd_ptr->pw_gid; 
   return erg;
#endif
}

const Sos_string Sos_user::home_dir( const char* username )
{
#if defined SYSTEM_GNU
    //int SOS_USER_HOME_DIR_MIT_GETENV_IMPLEMENTIERT;
    //return Sos_string( getenv( "HOME" ));
   const struct passwd* passwd_ptr = *username? getpwnam( username ) : getpwuid( getuid() );
   //LOG(" Sos_user::home_dir() returns " << ( passwd_ptr? passwd_ptr->pw_dir : "" ) << "\"\n" );
   return passwd_ptr? passwd_ptr->pw_dir : "";
#else
   if( !__is_sos_user_name_enabled() )  return Sos_string( "/etc" );

   char uname[LOGNAME_MAX+1];

   if ( strlen( username ) == 0 )
   {
     strcpy( uname, c_str(Sos_user::name()) );
   } else {
     strcpy( uname, username );
   }

//   char buf[1024];
   const struct passwd* passwd_ptr;
   passwd_ptr = getpwnam/*_r*/( uname /*, passwd_ptr, buf, sizeof buf*/ );
   if ( passwd_ptr != NULL ) return Sos_string( passwd_ptr->pw_dir );
   else return Sos_string( "" );
#endif
}


} //namespace sos

#endif
