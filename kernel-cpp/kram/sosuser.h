// sosuser.h
// js 12.3.95


#ifndef __SOSUSER_H
#define __SOSUSER_H

namespace sos {


struct Sos_user
{
     //Sos_user();
     //Sos_user( const char* name );
     //Sos_user( const uid_t user_id );

     // Name des augenblicklichen Users
     static const Sos_string name();

     // weitere zu einem gegebenen User 
     static const Sos_string  home_dir( const char* username = "" );
     static const Sos_string  fullname( const char* username = "" );
     static const unsigned int uid( const char* username = "" );
     static const unsigned int gid( const char* username = "" );

     
     //passwd* passwd_ptr();

//private:
//     passwd* _passwd;
};

}

#endif
