#ifndef __FILEBASE_H
#define __FILEBASE_H

#if !defined __SOSSTRNG_H
#   include "../kram/sosstrng.h"
#endif

#include <vector>

#include "../kram/sosfield.h"

namespace sos
{

//----------------------------------------------------------------------------

//struct Record_type;
//struct Field_descr;

typedef uint Record_length;
typedef uint Record_position;

const int file_max_key_length    = 255;
const int max_fs_filename = 200;             // Max. Länge des Dateinamens für Fileserver

struct SOS_CLASS Any_file;
struct SOS_CLASS Any_file_obj;

//---------------------------------------------------------------------------------------exceptions

struct File_not_found_error   : Xc { File_not_found_error()   : Xc( "D140" ) {} };
struct Record_not_found_error : Xc { Record_not_found_error() : Xc( "D311" ) {} };

//----------------------------------------------------------------------------------------Dbms_kind

enum Dbms_kind
{
    dbms_unknown,
    dbms_access,
    dbms_oracle,
    dbms_oracle_thin,           // Oracle Thin-Treiber für JDBC mit seinen Macken
    dbms_sql_server,
    dbms_mysql,
    dbms_sossql,
    dbms_postgresql,
    dbms_db2,
    dbms_firebird,
    dbms_sybase,
    dbms_h2
};  

//--------------------------------------------------------------------------Key

struct Key : Const_area
{
  //typedef Subrange< int, 0, 16 >  Number;
    typedef int                 Number;

                                Key                     ()                      : _number ( 0 ) {}
                                Key                     ( const Const_area&, Number = 0 );
                                Key                     ( const Key& key );  // Ab Borland 4.0 nötig

  //                            operator const void*    () const;
    Number                      number                  () const;

    static int                  max_length              ()  { return file_max_key_length; }

  private:
  //const void* _key_ptr;
    Number                     _number;
};

//------------------------------------------------------------------Record_lock

enum Record_lock {
    no_lock      = 0,
    write_lock   = 1, // mit wait, explizite Freigabe
    single_lock  = 2  // entpricht write_lock
};

//--------------------------------------------------------------------File_base

struct SOS_CLASS File_base
{
    enum Open_mode
    {
        // Folgende Zeilen stammen aus iostream.h, Klasse ios:
        in       = 0x01,        // open for reading
        out      = 0x02,        // open for writing
        inout    = 0x03,        // in | out
        ate      = 0x04,        // seek to eof upon original open
        app      = 0x08,        // append mode: all additions at eof
        trunc    = 0x10,        // truncate file if already exists
        nocreate = 0x20,        // open fails if file doesn't exist
        noreplace= 0x40,        // open fails if file already exists
        binary   = 0x80,        // binary (not text) file
        standard_mask = 0x00FF
        ,
        // Erweiterungen:
        tabs     = 0x0800,      // Tabs sind ausdruecklich erwuenscht!
                                // Provisorisches Flag (s. stdfile) js 24.8.94
        seq      = 0x1000,      // Nur sequentieller Zugriff
        share    = 0x2000,      // Gemeinsamer Zugriff
        blocked  = 0x4000,      // Geblockte Datei (512 Byte?)
        unsafe   = 0x8000,      // Nicht gesichert, dafür schnell

        in_seq   = 0x1001
    };

    static Open_mode norm_open_mode( File_base::Open_mode );

    typedef ::sos::Close_mode Close_mode;  // sos.h

}; // File_base

BITWISE_ENUM( File_base::Open_mode )


inline File_base::Open_mode norm_open_mode( File_base::Open_mode open_mode )
{
    return File_base::norm_open_mode( open_mode );
}

//---------------------------------------------------------------------Key_spec

struct Key_spec
{
    typedef Key_spec&           This_ref;

    enum Attributes
    {
        ka_none            = 0,
    	ka_duplicate       = 0x0001,
    	ka_modifiable      = 0x0002,
    	ka_null            = 0x0004,
    	ka_descending      = 0x0008,
    	ka_segmented       = 0x0010
    };

                                Key_spec                ();

                                Key_spec                ( Record_position, Record_length,
                                                          Attributes = ka_none,
                                                          const Field_descr* = 0 );

  //This_ref                    field_descr_ptr         ( const Field_descr* p )          { _field_descr_ptr = p; return *this; }
    This_ref                    field_descr_ptr         ( const Field_descr* );
    const Field_descr*          field_descr_ptr         () const                          { return _field_descr_ptr; }

    Record_position             key_position            () const  { return _key_position; }
    Record_length               key_length              () const  { return _key_length;   }

    Bool                        duplicate               () const  { return _duplicate;    }
    Bool                        modifiable              () const  { return _modifiable;   }
    Bool                        null                    () const  { return _null;         }
    Bool                        descending              () const  { return _descending;   }
    Bool                        segmented               () const  { return _segmented;    }

//private:
    Record_position            _key_position;          // 0..
    Record_length              _key_length;
    Bool                       _duplicate;
    Bool                       _modifiable;
    Bool                       _null;
    Bool                       _descending;
    Bool                       _segmented;
    //VC2003 Sos_static_ptr<Field_descr> _field_descr_ptr;
    Sos_ptr<Field_descr>       _field_descr_ptr;
}; // struct Key_spec

typedef Key_spec::Attributes Key_attributes;

//----------------------------------------------------------------------------------------Key_specs

struct Key_specs
{
    typedef Key_spec::Attributes Attributes;

    inline                      Key_specs               ();
                                Key_specs               ( const Key_spec& );
                                Key_specs               ( Record_position, Record_length,
                                                          Key_attributes = Key_spec::ka_none );

                                Key_specs               ( const Key_specs& ); // Ab Borland 4.0 nötig

    Key_specs                   operator&               ( const Key_spec&  ) const;
    Key_specs                   operator&               ( const Key_specs& ) const;

    Record_position             key_position            ( Key::Number = 0 ) const;
    Record_length               key_length              ( Key::Number = 0 ) const;

    int                         duplicate               ( Key::Number = 0 ) const;
    int                         modifiable              ( Key::Number = 0 ) const;
    int                         null                    ( Key::Number = 0 ) const;
    int                         descending              ( Key::Number = 0 ) const;
    int                         segmented               ( Key::Number = 0 ) const;

    int                         key_count               ()                  const;

    const Key_spec&             operator []             ( int ) const                  { /*assert( i == 0 );*/ return _key_spec; }

  //private:
    int                        _key_count;  // 0..file_max_keys
    Key_spec                   _key_spec /*[file_max_keys]*/;

}; // struct Key_specs

//----------------------------------------------------------------------------------Secondary_index
/*
struct Secondary_index
{
    struct Segment
    {
                                Segment                 ( int offset, int length ) : _offset(offset), _length(length) {}

        int                    _offset;
        int                    _length;
    };

    string                     _name;                   // Name des Index
    std::vector<Segment>       _segments;               // Das letzte Segment ist der Primärschlüssel
    string                     _filename;               // Dateiname des Index
    Any_file                   _file;
};
*/
//--------------------------------------------------------------------File_spec

struct File_spec
{
    typedef File_spec&          This_ref;

                                File_spec               ();
                                File_spec               ( Record_length    max_record_length,
                                                          const Key_specs&                   = Key_specs(),
                                                          int              pad               = 10,          // Reserve in %
                                                          int              first_allocation  = 0,
                                                          const Record_type*                 = 0 );         // Der erste Platz
                                File_spec               ( const File_spec& );
                               ~File_spec               ();

    File_spec&                  operator =              ( const File_spec& );
    void                        assign                  ( const File_spec& );

    Record_length               max_record_length       ()            const;
    const Key_specs&            key_specs               ()            const;
    Record_position             key_position            ( Key::Number = 0 ) const;
    Record_length               key_length              ( Key::Number = 0 ) const;
    int                         duplicate               ( Key::Number = 0 ) const;
    int                         modifiable              ( Key::Number = 0 ) const;
    int                         null                    ( Key::Number = 0 ) const;
    int                         descending              ( Key::Number = 0 ) const;
    int                         segmented               ( Key::Number = 0 ) const;
    int                         key_count               ()            const;
    int                         pad                     ()            const;
    long                        first_allocation        ()            const;
    This_ref                    field_type_ptr          ( const Record_type* );
    const Record_type*          field_type_ptr          () const                                { return _field_type_ptr; }

//private:
    friend struct               Any_file_obj;

    Record_length              _max_record_length;
    Key_specs                  _key_specs;
    int                        _pad;                    // Padding factor in %
    long                       _first_allocation;       // Anfängliche Größe der Datei in Byte
    Sos_static_ptr<Record_type> _field_type_ptr;        // Feldweise Satzbeschreibung (s. sosfield.h)
    Bool                       _field_type_is_from_caller;  // Feldbeschreibung stammt nicht vom Dateityp (Für Dateityp com. Bei true wird _field_type_ptr nicht berücksichtigt.) jz 18.2.00 für Eichenauer
    Bool                       _need_type;              // open() soll Satzbeschreibung liefern
};

extern const File_spec& std_file_spec;

//----------------------------------------------------------------File_info

const int max_typename = 33;
//const int max_filename = 256;

struct File_info
{
                                File_info               ();
                                File_info               ( const File_info& info );
                               ~File_info               ();

    File_info&                  operator =              ( const File_info& info )   { _assign( info ); return *this; }
    void                       _assign                  ( const File_info& );

    Sos_string                 _typename;
    Sos_string                 _filename;
    Key_specs                  _key_specs;
    Sos_string                 _text;           // Formlos für Ingres (offene Transaktion etc.)
};


//--------------------------------------------------------------------------Key

inline Key::Key( const Const_area& key_area, Number key_number )
:
    Const_area ( key_area )
{
    _number   = key_number;
}

inline Key::Key( const Key& key )
:
    Const_area ( key )
{
    _number   = key._number;
}

inline Key::Number Key::number() const { return _number; }

//---------------------------------------------------------------------Key_spec

inline Key_spec::Key_spec()
:
    _key_position  (0),
    _key_length    (0),
    _duplicate  (false),
    _modifiable (false),
    _null       (false),
    _descending (false),
    _segmented  (false),
    _field_descr_ptr ( 0 )
{
}

//---------------------------------------------------------------------Key_spec

inline Key_specs::Key_specs()
{
    _key_count = 0;
}

//-----------------------------------------------------------------------------

inline Key_specs::Key_specs( const Key_spec& k )
{
    _key_count = 1;
    _key_spec /*[0]*/ = k;
}

//-----------------------------------------------------------------------------

inline Key_specs::Key_specs( const Key_specs& k )
{
    _key_count = k._key_count;
    _key_spec  = k._key_spec;
}

//-----------------------------------------------------------------------------

inline Record_position Key_specs::key_position( Key::Number ) const
{
    return _key_spec/*[0]*/.key_position();
}

//-----------------------------------------------------------------------------

inline Record_length Key_specs::key_length( Key::Number ) const
{
    return _key_spec/*[0]*/.key_length();
}

//-----------------------------------------------------------------------------

inline int Key_specs::duplicate( Key::Number ) const
{
    return _key_spec/*[0]*/.duplicate ();
}

//-----------------------------------------------------------------------------

inline int Key_specs::modifiable( Key::Number ) const
{
    return _key_spec/*[0]*/.modifiable();
}

//-----------------------------------------------------------------------------

inline int Key_specs::null( Key::Number ) const
{
    return _key_spec/*[0]*/.null();
}

//-----------------------------------------------------------------------------

inline int Key_specs::descending( Key::Number ) const
{
    return _key_spec/*[0]*/.descending();
}

//-----------------------------------------------------------------------------

inline int Key_specs::segmented( Key::Number ) const
{
    return _key_spec/*[0]*/.segmented();
}

//-----------------------------------------------------------------------------

inline int Key_specs::key_count() const
{
    return _key_count;
}

//--------------------------------------------------------------------File_spec

inline Record_length File_spec::max_record_length() const
{
    return _max_record_length;
}

//-----------------------------------------------------------------------------

inline const Key_specs& File_spec::key_specs() const
{
    return _key_specs;
}

//-----------------------------------------------------------------------------

inline Record_position File_spec::key_position( Key::Number n ) const
{
    return _key_specs.key_position( n );
}

//-----------------------------------------------------------------------------

inline Record_length File_spec::key_length( Key::Number n ) const
{
    return _key_specs.key_length( n );
}

//-----------------------------------------------------------------------------

inline int File_spec::duplicate( Key::Number n ) const
{
    return _key_specs.duplicate (n);
}

//-----------------------------------------------------------------------------

inline int File_spec::modifiable( Key::Number n ) const
{
    return _key_specs.modifiable(n);
}

//-----------------------------------------------------------------------------

inline int File_spec::null( Key::Number n ) const
{
    return _key_specs.null( n );
}

//-----------------------------------------------------------------------------

inline int File_spec::descending( Key::Number n ) const
{
    return _key_specs.descending(n);
}

//-----------------------------------------------------------------------------

inline int File_spec::segmented( Key::Number n ) const
{
    return _key_specs.segmented (n);
}

//-----------------------------------------------------------------------------

inline int File_spec::key_count() const
{
    return _key_specs.key_count();
}

//-----------------------------------------------------------------------------

inline int File_spec::pad() const
{
    return _pad;
}

//-----------------------------------------------------------------------------

inline long File_spec::first_allocation() const
{
    return _first_allocation;
}

//-----------------------------------------------------------------------------
/*
inline File_info::File_info( const char* fname,
                             const char* type,
                             const Key_specs& kspecs )
{
    strcpy( filename, fname ); // erst mal so ...
    strcpy( typename, type );
    key_specs = kspecs;
};
*/
//-------------------------------------------------------------------------

//inline void Abs_record_file::write( Const_area area )  { Record_stream::write( area ); }
//inline void Abs_record_file::_write_end_of_record()     { Record_stream::_write_end_of_record(); }
//inline void Abs_record_file::flush()                   { Record_stream::flush(); }
//inline void Abs_record_file::read (      Area& area )  { Record_stream::read ( area ); }
//inline int  Abs_record_file::skip_record()             { return Record_stream::skip_record(); }
//inline int  Abs_record_file::get_char ()                   { return Stream::get_char(); }
#if defined USE_STREAM_FILE
    inline void Abs_record_file::put  ( unsigned char c )  { Stream::put( c ); }
#endif

//---------------------------------------------Abs_record_file::Abs_record_file
#if 0
inline Abs_record_file::Abs_record_file()
 :  Record_stream    ( this )
{ }
#endif
//----------------------------------------------Abs_record_file::streambuf_size
#if 0

inline void Abs_record_file::streambuf_size( Record_length buffer_size )
{
    _buffer_size = buffer_size;
}

//-----------------------------------------------Abs_record_file::streambuf_ptr

inline void Abs_record_file::streambuf_ptr( streambuf* s )
{
    _streambuf_ptr = s;
}

//-----------------------------------------------Abs_record_file::streambuf_ptr

inline streambuf* Abs_record_file::streambuf_ptr() const
{
    return _streambuf_ptr;
}

//----------------------------------------------------Abs_record_file::stream

inline Record_stream& Abs_record_file::stream()
{
    if( !_stream_ptr )  return _stream();
                  else  return *_stream_ptr;
}

//--------------------------------------------Abs_record_file::operator stream&

inline Abs_record_file::operator Record_stream& ()
{
    return stream();
}

//---------------------------------------------------------Abs_record_file::get

inline int Abs_record_file::get()
{
    return stream().get();
}

//-------------------------------------------------Abs_record_file::read_until

inline void Abs_record_file::read_until( const Const_area& text )
{
    stream().read_until( text );
}

//-------------------------------------------------Abs_record_file::operator <<

inline Record_stream& Abs_record_file::operator << ( Const_area p )  { write( p ); return *this; }
inline Record_stream& Abs_record_file::operator << ( Record_stream& (*p)(Record_stream&))  { return p( stream() ); }
inline Record_stream& Abs_record_file::operator >> ( Area& p )       { read( p ); return *this; }

DEFINE_OSTREAM_OPERATIONS(Abs_record_file,stream())
DEFINE_ISTREAM_OPERATIONS(Abs_record_file,stream())


inline Record_stream& Streambuf_file::operator << ( Const_area p )  { write( p ); return *this; }
inline Record_stream& Streambuf_file::operator << ( Record_stream& (*p)(Record_stream&))  { return p( stream() ); }
inline Record_stream& Streambuf_file::operator >> ( Area& p )       { read( p ); return *this; }

DEFINE_OSTREAM_OPERATIONS(Streambuf_file,stream())
DEFINE_ISTREAM_OPERATIONS(Streambuf_file,stream())
#endif

} //namespace sos

#endif
