//                                                  (c) SOS GmbH Berlin

#ifndef __FILTER_H
#define __FILTER_H

#include <ctype.h>
#include <string.h>
#include "absfile.h"

namespace sos {


struct Byte_translation {

   // Funktion fÅr put, putkey etc.
   virtual void convert( Area& ) = 0;

   // Funktion fÅr get, getkey etc.
   virtual void trevnoc( Area& ) = 0;
};

static void nop( Area& ) { return; }

struct Any_file;

struct Filter_file : Abs_file
{

  Filter_file( Byte_translation* bc ) : _current_filter( bc ), _next(0), _fname(0) {};
  Filter_file( Byte_translation* bc, const char* next ) :
     _current_filter( bc ), _next( next ), _fname(0) {};
  ~Filter_file() { close(); };

  static const Abs_file_type& static_file_type();

  virtual void close (
      Close_mode close_mode = close_normal
  );

  // Filter_record - spezifischer Teil ...
  virtual void open(
      const char* filename,
      Open_mode         open_mode,
      const File_spec&
  );

protected:
  virtual void put_record ( const Const_area& area );

  virtual void get_record ( Area& area );

private:
    Any_file                   _f;
    const char*                _next;
    Byte_translation*          _current_filter;
    char*                      _fname;
};


// Diverse Beispiel - Filter ...
// nur Filtern ..., d.h. wirksam bei Put-Routinen!
struct Byte_filter : public Byte_translation {

   virtual void convert( Area& ) = 0;

   inline void trevnoc( Area& area ) {
     nop( area );
   };
};

// das gleiche fÅr Get-Routinen ...
struct Byte_conversion : public Byte_translation {

   virtual void trevnoc( Area& ) = 0;

   inline void convert( Area& area ) {
     nop( area );
   };
};

extern Byte a2a_printable[];
extern Byte e2a_printable[];
//extern Byte tbebcasc[];

struct Asc2asc : public Byte_conversion {

   inline void convert( Area& area ) {

     Byte *b = (Byte *) area.ptr();

     for ( int i=0; i < area.length(); i++ ) {
       b[i] = a2a_printable[b[i]];
     };
   };
};


struct Ebcasc : public Byte_translation {

   inline void convert( Area& area ) {
     Byte *b = (Byte *) area.ptr();

     for ( int i=0; i < area.length(); i++ ) {
	 b[i] = tbebcasc[b[i]];
     };
   };

   inline void trevnoc( Area& area ) {
     Byte *b = (Byte *) area.ptr();

     for ( int i=0; i < area.length(); i++ ) {
	 b[i] = tbascebc[b[i]];
     };
   };
};


struct Ebc2asc : public Byte_filter {

   inline void convert( Area& area ) {
     Byte *b = (Byte *) area.ptr();

     for ( int i=0; i < area.length(); i++ ) {
	 b[i] = tbebcasc[b[i]];
     };
   };
};



struct Strupr : public Byte_filter {

   inline void convert( Area& area ) {
     Byte *b = (Byte *) area.ptr();

     for ( int i=0; i < area.length(); i++ ) {
	switch (b[i]) {
	  case 'Ñ': b[i] = 'é'; break;
	  case 'î': b[i] = 'ô'; break;
	  case 'Å': b[i] = 'ö'; break;
	  default : b[i] = toupper((char) b[i]);
	};
     };
   };
};


struct Strlwr : public Byte_filter {

   inline void convert( Area& area ) {
     Byte *b = (Byte *) area.ptr();

     for ( int i=0; i < area.length(); i++ ) {
       switch ((char) b[i]) {
	  case 'é': b[i] = 'Ñ'; break;
	  case 'ô': b[i] = 'î'; break;
	  case 'ö': b[i] = 'Å'; break;
	  default : b[i] = tolower((char) b[i]);
	};
     };
   };
};

/*
struct Increment : public Byte_filter {

   inline void convert( Area& area ) {
     Byte *b = (Byte *) area.ptr();

     for ( int i=0; i < area.length(); i++ ) {
       b[i] = (short int) (b[i] + 1);
     };
   };
};
*/

struct Dachhex : Byte_filter {
  void convert( Area& area );
private:
  int is_hex( const char*, unsigned char* );
};

} //namespace sos

#endif
