//#define MODULE_NAME "sysfile"
// sysfile.cpp

#include <precomp.h>

#if 0

#include <process.h>
#include <xception.h>
#include <nullasgn.h>
#include <string.h>

#include "sysfile.h"

extern int errno;

//-------------------------------------------------------System_file_type
struct System_file_type : Abs_file_type
{
    System_file_type() : Abs_file_type() {};

    virtual const char* name() const { return "system"; }

    virtual Bool is_record_file()  const { return true; }

    virtual Abs_file_base* create_base_file() const { return new System_file(); }

    virtual Abs_record_file* create_record_file() const { return new System_file(); }

};

static System_file_type _System_file_type;

const Abs_file_type& System_file::static_file_type () {
  return _System_file_type;
};

//

System_file::System_file( Const_string0_ptr cmd ) {
   open( cmd ); xc;
exceptions
};

void System_file::open( Const_string0_ptr cmd, Open_mode ) {
   dos_command( cmd ); xc;
exceptions
};

void System_file::close( Close_mode ) {
};

void System_file::put_record( Const_area area ) {
   char *buf = new char[ area.length() + 1 ];
      if (!buf) raise( "R101", "NOMEMORY" );
   memcpy( buf, area.char_ptr(), area.length() );
   buf[area.length()] = 0;

   dos_command( buf ); xc;
   delete [] buf;  buf = 0;
exceptions
   delete [] buf;  buf = 0;
};

void System_file::get_record( Area& ) {
};

void System_file::dos_command( Const_string0_ptr cmd ) {
   if ( !check_dos_command( cmd ) ) {
      return;
   };
   if ( system( cmd ) == -1 ) {
      raise_errno( errno );
   };
   null_pointer_assignment.init();
exceptions
};

Bool System_file::check_dos_command( Const_string0_ptr cmd ) {
   Const_string0_ptr ptr = cmd;
   while ( *ptr == ' ' ) ptr++;
   if ( strcspn( ptr, " \r\n\t" ) == 0 ) {
     return false;
   } else {
     return true;
   };
};

#endif

