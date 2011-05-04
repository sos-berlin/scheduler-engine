//#define MODULE_NAME "filter"
// filter.cpp
// Jörg Schwiemann

#include "precomp.h"
#if 0

#include "../kram/sysdep.h"

#if defined __BORLANDC__
    // Borland-C 4.02 generiert Schrott hinter _f->get( ... ) und _f->put(). ( mov es,ax;  mov es,dx )
    // Ohne Optimierung geht's.   jz 25.3.95
#   pragma option -Od
#endif


#include <stdio.h>
#include "../kram/sosstrng.h"
#include "../kram/sos.h"
#include "../kram/xception.h"
#include <ctype.h>

#include "anyfile.h"
#include "filter.h"

// Filter_file ... Implementation
//-----------------------------------------------------------Filter_file::open

void Filter_file::open( const char* filename, Open_mode  open_mode, const File_spec& )
{
    //char *fname;

    if (_next) {
       _fname = new char[strlen(_next)+strlen(filename)+1];
	      if (!_fname)   throw_no_memory_error(); //raise( "NOMEMORY", "R101" );
       strcpy( _fname, _next); strcat( _fname, ":"),
       strcat( _fname, filename );
    } else {
       _fname = new char[strlen(filename)+1];
	      if (!_fname)   throw_no_memory_error();  //raise( "NOMEMORY", "R101" );
       strcpy( _fname, filename );
    };
    //_f = SOS_NEW_PTR( Any_file );
	//      if (!_f)  throw No_memory_error();
    _f.open( _fname, open_mode ); xc;


exceptions
    delete [] (void*) _fname; _fname = 0;
    //delete (void*) _f; _f = 0;
};

//----------------------------------------------------------Filter_file::close
void Filter_file::close ( Close_mode close_mode ) {

    _f.close( close_mode ); xc;
    delete _current_filter; _current_filter = 0;
    delete [] (void*) _fname; _fname = 0;

exceptions
    delete _current_filter; _current_filter = 0;
    delete [] (void*) _fname; _fname = 0;
}

//------------------------------------------------------------Filter_file::put_record
void Filter_file::put_record (	const Const_area& area )
{
    Byte *buf = new Byte[area.length()];
    Area a( buf, area.length() );
      if (!buf) throw_no_memory_error();  //raise( "NOMEMORY", "R101" );

    memcpy( buf, area.ptr(), area.length() );
    _current_filter->convert( a ); // Length(a) <= Length(area)!
    _f.put( a ); xc;
    delete [] buf;  buf = 0;

exceptions
      delete [] buf;  buf = 0;
};

//------------------------------------------------------------Filter_file::get_record
void Filter_file::get_record ( Area& area) {

    _f.get( area ); xc;
    _current_filter->trevnoc( area );

exceptions
};


// Div. Filter-Implementationen soweit nicht in filter.h
//--------------------------------------------------------------Dachx::convert
void Dachhex::convert ( Area& area ) {
  // ^-Filtern ... Area_length verkleinern.
  Byte *area_ptr = (Byte *) area.ptr();
  int j = 0;

  // funktioniert, da j immer kleiner gleich i ist !!!
  for ( int i = 0; i < area.length() ; i++ ) {
     if ( area_ptr[i] == '^' ) {
       if ( i+1 >= area.length() ) {
	  //area_ptr[j++] == '^';     // Soll bestimmt eine Zuweisung sein! jz
	  area_ptr[j++] = '^';
	  break;
       };

       if ( area_ptr[i+1] == '^' ) {       //  ^^
	 area_ptr[j++] = '^'; i++;
       } else {

	 if ( i+2 >= area.length() ) {
	    area_ptr[j++] = '^';
	    area_ptr[j++] = area_ptr[i+1];
	    i++;
	    break;
	 };

	 unsigned char c;
	 if ( is_hex( (const char*)/*jz*/&area_ptr[i+1], &c ) ) {
	     area_ptr[j++] = c; i++; i++;
	 } else {
	     area_ptr[j++] = '^';
	     area_ptr[j++] = area_ptr[i+1];
	     area_ptr[j++] = area_ptr[i+2];
	     i++; i++;
	 };
       };
     } else { // der Normalfall ...
       area_ptr[j++] = area_ptr[i];
     };
  };

  area.set_length( j );

};

//---------------------------------------------------------------Dachx::is_hex
const char* hex_str = "0123456789ABCDEF";

int Dachhex::is_hex( const char* hex, unsigned char* c_ptr ) {

  const char* p1 = strchr( hex_str, toupper( hex[ 0 ] ));
  const char* p2 = strchr( hex_str, toupper( hex[ 1 ] ));

  if (p1 && p2) {
      *c_ptr = (p1 - hex_str) * 16 + (p2 - hex_str);
      return true;
  } else {
      return false;
  }

  //jz if ( ( strchr( hex_str, hex[0] ) == 0 ) ||
  //jz     ( strchr( hex_str, hex[1] ) == 0 ) ) {
  //jz  return false;
  //jz } else {
  //jz   sscanf( hex, "%x", c );
  //jz   return true;
  //jz };
};

// Verschiedene Filter-Beschreibungen ...

// Ebcasc--Filter
struct Ebcasc_file_type : Abs_file_type
{
    Ebcasc_file_type() : Abs_file_type() {};

    virtual const char* name() const { return "ebcasc"; }

    virtual Bool is_record_file()  const { return true; }

    virtual Sos_ptr<Abs_file> create_base_file() const
    {
          Byte_translation* p = (Byte_translation*) new Ebcasc;
          Sos_ptr<Filter_file> f = SOS_NEW_PTR( Filter_file( p ) );
          return +f;
    }
};

static Ebcasc_file_type _Ebcasc_file_type;

const Abs_file_type& Filter_file::static_file_type () {
  return _Ebcasc_file_type;
};  // Ebcasc als Beispiel-Filter (reicht um das Modul bekannt zu machen!)


// Dachhex_file_type
struct Dachhex_file_type : Abs_file_type
{
    Dachhex_file_type() : Abs_file_type() {};

    virtual const char* name() const { return "dachhex"; }

    virtual Sos_ptr<Abs_file> create_base_file() const
    {
        Sos_ptr<Filter_file> f = SOS_NEW_PTR( Filter_file((Byte_translation *) new Dachhex) );
        return +f;
    };
};

static Dachhex_file_type _Dachhex_file_type;

#endif
