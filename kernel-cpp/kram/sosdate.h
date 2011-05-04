// sosdate.h                                        (c) SOS GmbH Berlin
// $Id$                                                     Joacim Zschimmer

#ifndef __SOSDATE_H
#define __SOSDATE_H

#if !defined __STORABLE_H
//#   include <storable.h>
#endif

#if !defined __SOSLIMTX_H
#   include "soslimtx.h"       // für Ebcdic_date
#endif

namespace sos
{

extern const char* std_date_format_iso   ;   // = "yyyy-dd-mm";
extern const char* std_date_format_german;   // = "dd.mm.yyyy";
extern const char* std_date_format_ingres;   // = "dd-mon-yyyy";
extern const char* std_date_format_yyyymmdd; // = "yyyymmdd";
extern const char* std_date_format_english;  // = "mon, dd yyyy";
extern const char* std_date_format_odbc;     // = "{d'yyyy-mm-dd'}";

extern const char* std_date_time_format_iso   ;   // = "yyyy-dd-mm HH:MM:SS";
extern const char* std_date_time_format_german;   // = "dd.mm.yyyy HH:MM:SS";
extern const char* std_date_time_format_ingres;   // = "dd-mon-yyyy HH:MM:SS";
extern const char* std_date_time_format_yyyymmdd; // = "yyyymmddHHMMSS";
extern const char* std_date_time_format_english;  // = "mon, dd yyyy HH:MM:SS";
extern const char* std_date_time_format_odbc;     // = "{ts'yyyy-mm-dd HH:MM:SS'}";

const int default_schwellenjahr = 100;      // 0: yy-->19yy,  100: yy-->20yy

Bool schaltjahr( int );


enum Sos_date_kind      // Art der Datumsbestandteile für automatische Erkennung
{
    kind_none  = 0,
    kind_year  = 1,
    kind_month = 2
};

//-----------------------------------------------------------------------Sos_optional_date_time

struct Sos_optional_date_time
{
                                Sos_optional_date_time  ()                                      { _date_only = false; set_null(); }
                                Sos_optional_date_time  ( const char* d )                       { _date_only = false; assign( d ); }
                                Sos_optional_date_time  ( const Sos_string& d )                 { _date_only = false; assign( d ); }
                                Sos_optional_date_time  ( time_t unix_time )                    { _date_only = false; set_time_t( unix_time ); }

    inline void                 assign_date             ( int year, int month, int day, int schwellenjahr = default_schwellenjahr )   { _assign_date( year, month, day, schwellenjahr ); }
    void                        assign                  ( const char* s, int schwellenjahr = default_schwellenjahr );//           { assign( s, length( s ) ); }
  //void                        assign                  ( const char*, int len );
    void                        assign                  ( const Sos_string&, int schwellenjahr = default_schwellenjahr );
    Sos_string                  as_string               () const                                { return formatted(); }
    double                      as_double               () const                                { return (double)day_count() + (double)_time / (24*60*60); }
    double                      time_as_double          () const                                { return (double)_time; }
    int                         time_as_int             () const                                { return _time; }
    Sos_string                  formatted               ( const char* format = "" ) const;
    Sos_string                  formatted               ( const string& format ) const          { return formatted( c_str(format) ); }

    Sos_optional_date_time&     operator =              ( const Sos_string& str )               { assign( str ); return *this; }
    Sos_optional_date_time&     operator =              ( const char* str )                     { assign( str ); return *this; }

#ifdef SYSTEM_WIN
    Sos_optional_date_time&     operator =              ( const FILETIME& filetime )            { assign( filetime ); return *this; }
    void                        assign                  ( const FILETIME& );
    Sos_optional_date_time&     operator =              ( const SYSTEMTIME& systemtime )        { assign( systemtime ); return *this; }
    void                        assign                  ( const SYSTEMTIME& );
#endif

    void                        write_text              ( Area*, const char* format = "" ) const;
    void                        read_text               ( const char*, const char* format = "" );
    void                        read_text               ( const string& str, const char* format = "" ) { read_text( str.c_str(), format ); }

    void                        day_of_year             ( int );    // year() muß gesetzt sein!
    int                         day_of_year             () const;
    int4                        day_count               () const;    // Anzahl der Tage seit day_count( 0 )
    void                        day_count               ( int4 );    // Anzahl der Tage seit day_count( 0 ) setzen

    void                        add_days                ( int days );
    void                        add_days_and_time       ( double days );

  //DECLARE_PUBLIC_MEMBER( uint2, year )
  //DECLARE_PUBLIC_MEMBER( uint1, month )
  //DECLARE_PUBLIC_MEMBER( uint1, day )
    int                         year                    () const                                { return _year;  }
    int                         month                   () const                                { return _month; }
    int                         day                     () const                                { return _day;   }
    int                         hour                    () const                                { return uint4( _time ) / 3600; }
    int                         minute                  () const                                { return uint4( _time ) / 60 % 60; }
    int                         second                  () const                                { return uint4( _time ) / 1 % 60; }
    bool                        time_is_zero            () const                                { return _time == 0; }

    Bool                        date_only               () const                                { return _date_only; }
    void                        date_only               ( Bool b )                              { _date_only = b; if( b )  _time = 0; }
    bool                        has_date                () const                                { return ( _year|_month|_day) != 0; }

    void                        set_time                ( time_t time );
    void                        set_time                ( int hour, int minute, int second = 0 ){ set_time( hour*3600 + minute*60 + second ); }
    void                        set_time                ( const char* );
    void                        set_time                ( const string& t )                     { set_time( t.c_str() ); }
    void                       _set_time                ( const char** );
    time_t                      as_time_t               () const;
    void                        set_time_t              ( time_t );

    Bool                        empty                   () const                                { return null();  }
    void                        make_empty              ()                                      { set_null(); }

    Bool                        null                    () const                                { return _day == 0;  }
    virtual void                set_null                ();

    int                         operator <              ( const Sos_optional_date_time& date ) const  { return compare( date ) <  0; }
    int                         operator <=             ( const Sos_optional_date_time& date ) const  { return compare( date ) <= 0; }
    int                         operator ==             ( const Sos_optional_date_time& date ) const  { return compare( date ) == 0; }
    int                         operator !=             ( const Sos_optional_date_time& date ) const  { return compare( date ) != 0; }
    int                         operator >=             ( const Sos_optional_date_time& date ) const  { return compare( date ) >= 0; }
    int                         operator >              ( const Sos_optional_date_time& date ) const  { return compare( date ) >  0; }
    int                         compare                 ( const Sos_optional_date_time& ) const;

    static Sos_optional_date_time now                   ();

  protected:
    friend struct               Sos_date;
    friend struct               Sos_optional_date;

    void                       _force_null              ();
    virtual void               _assign_date             ( int year, int month, int day, int schwellenjahr = default_schwellenjahr );
    void                        get_part                ( const char**, int*, Sos_date_kind* );
  //virtual void               _set_time                ( time_t );

    Bool                       _date_only;

  private:
    uint2                      _year;
    uint1                      _month;
    uint1                      _day;
    uint4                      _time;
};


//----------------------------------------------------------------------------Sos_optional_date

struct Sos_optional_date : Sos_optional_date_time
{
                                Sos_optional_date       ()                                      { _date_only = true; }
                                Sos_optional_date       ( int year, int month, int day )        { _date_only = true; assign_date( year, month, day ); }
                                Sos_optional_date       ( const char* d )                       { _date_only = true; assign( d ); }
                                Sos_optional_date       ( const Sos_string& d )                 { _date_only = true; assign( d ); }

    void                        assign                  ( int year, int month, int day, int schwellenjahr = default_schwellenjahr )      { assign_date( year, month, day, schwellenjahr ); }
    void                        assign                  ( const char* s, int schwellenjahr = default_schwellenjahr )                        { Sos_optional_date_time::assign( s, schwellenjahr ); }
    void                        assign                  ( const Sos_string& s, int schwellenjahr = default_schwellenjahr )                  { Sos_optional_date_time::assign( s, schwellenjahr ); }

    Sos_optional_date&          operator =              ( const Sos_string& str )               { assign( str ); return *this; }
    Sos_optional_date&          operator =              ( const char* str )                     { assign( str ); return *this; }

  //virtual void               _set_time                ( time_t );
};


inline Bool empty( const Sos_optional_date& date )  { return date.empty();    }
inline void empty( Sos_optional_date* date_ptr   )  { date_ptr->make_empty(); }

#if defined __SOSSTRNG_H
    Sos_string as_string( const Sos_optional_date_time&, const char* format );

    inline Sos_string as_string( const Sos_optional_date_time& date_time, const Sos_string& format )
    {
        return as_string( date_time, c_str( format ) );
    }
#endif

::std::ostream& operator << ( ::std::ostream&, const Sos_optional_date_time& );
::std::istream& operator >> ( ::std::istream&, Sos_optional_date& );

//---------------------------------------------------------------------------Sos_future_date
/*
struct Sos_future_date : Sos_optional_date
{
                                Sos_future_date         ()  {}
                                Sos_future_date         ( const Sos_optional_date& );

    virtual void               _assign                  ( uint2 year, uint1 month, uint1 day, int schwellenjahr = default_schwellenjahr );
};

//-----------------------------------------------------------------------------Sos_past_date

struct Sos_past_date : Sos_optional_date
{
                                Sos_past_date           ()  {}
                                Sos_past_date           ( const Sos_optional_date& );

    virtual void               _assign                  ( uint2 year, uint1 month, uint1 day, int schwellenjahr = default_schwellenjahr );
};
*/
//-----------------------------------------------------------------------------------Sos_date

struct Sos_date : Sos_optional_date
{
                                Sos_date                () {}
                                Sos_date                ( uint2 year, uint1 month, uint1 day )  { assign( year, month, day ); }
                                Sos_date                ( const char* d )                       { assign( d ); }
                                Sos_date                ( const Sos_string& d )                 { assign( d ); }
                                Sos_date                ( const Sos_optional_date_time& );

    virtual void                set_null                ();

    Sos_date                    operator +              ( int4 i ) const;
    Sos_date                    operator -              ( int4 i ) const;
    int4                        operator -              ( const Sos_date& d ) const             { return day_count() - d.day_count(); }
    Sos_date&                   operator +=             ( int4 i )                              { day_count( day_count() + i ); return *this; }
    Sos_date&                   operator -=             ( int4 i )                              { day_count( day_count() - i ); return *this; }

    static Sos_date             today                   ();
};

inline Sos_date                 operator +              ( int4 i, const Sos_date& d )           { return d + i; }
inline Sos_date                 as_date                 ( const Sos_optional_date_time& dt )    { return Sos_date(dt); }
int                             last_day_of_month       ( const Sos_date& );
Sos_optional_date_time          ultimo                  ( const Sos_optional_date_time& );


//-----------------------------------------------------------------------Sos_optional_date_time_type
#if defined __SOSFIELD_H

struct Sos_optional_date_time_type : Field_type
{
                                Sos_optional_date_time_type  ( const char* format = "" ) : Field_type( &_type_info, sizeof (Sos_optional_date_time) ), _format( format ) {}


    /* Format kann enthalten:
       yyyy Jahr
       yy   Jahr ohne Jahrhundert (19xx wird angenommen (???))
       mm   Monat
       dd   Tag
       doy  Tag des Jahres 1..365/366
       //mon  jan feb mar apr may jun jul aug sep oct nov dec
       //MON  JAN FEB MAR APR MAY JUN JUL AUF SEP OCT NOV DEC
       //mnt  Jan Feb Mär Apr Mai Jun Jul Aug Sep Okt Nov Dez
       //Mnt  JAN FEB MÄR APR MAI JUN JUL AUF SEP OKT NOV DEZ
       //month  january february march april may june july august september october november december
       //MONTH  JANUARY FEBRUARY MARCH APIRL MAY JUNE JULY AUGUST SEPTEMBER OCTOBER NOVEMBER DECEMBER
       //monat  Januar Februar März April Mai Juni Juli August September Oktober November Dezember
       //MONAT  JANUAR FEBRUAR MÄRZ APRIL MAI JUNI JULI AUGUST SEPTEMBER OKTOBER NOVEMBER DEZEMBER
       alle anderen Zeichen werden beim Lesen geprüft bzw. geschrieben.
    */

    DECLARE_PUBLIC_MEMBER( Sos_limited_text<30>, format )

    void                        write_text              ( const Byte* p, Area*, const Text_format& f = raw_text_format ) const;
    void                        read_text               ( Byte* p, const char*, const Text_format& f = raw_text_format ) const;

    virtual void                construct               ( Byte* p ) const               { new ( p ) Sos_optional_date_time; }
    virtual void                destruct                ( Byte* ) const;
#   if defined SYSTEM_ALIGNED
    virtual int                 alignment               () const                        { return sizeof (int); }
#   endif

    virtual Bool                nullable                () const                        { return true; }
    virtual Bool                null                    ( const Byte* p ) const         { return ((Sos_optional_date_time*)p)->null(); }
    virtual void                set_null                ( Byte* p ) const               { ((Sos_optional_date_time*)p)->set_null(); }
    virtual Bool                empty_is_null           () const                        { return true; }
    virtual double              as_double               ( const Byte* p ) const         { return ((Sos_optional_date_time*)p)->as_double(); }

    void                        check_type              ( const Sos_optional_date_time* ) {}

    static Listed_type_info    _type_info;

  protected:
                                Sos_optional_date_time_type( const Type_info* info, uint size, const char* format ) : Field_type( info, size ), _format( format ) {}

    void                       _get_param               ( Type_param* ) const;
};

extern Sos_optional_date_time_type sos_optional_date_time_type;
DEFINE_ADD_FIELD( Sos_optional_date_time, sos_optional_date_time_type )

//typedef Sos_optional_date_time_type Sos_optional_date_type;

//-----------------------------------------------------------------Sos_optional_date_type

struct Sos_optional_date_type : Sos_optional_date_time_type
{
                                Sos_optional_date_type  ( const char* format = "" ) : Sos_optional_date_time_type( &_type_info, sizeof (Sos_optional_date), format ) {}

    static Listed_type_info    _type_info;

    virtual void                construct               ( Byte* p ) const               { new ( p ) Sos_optional_date; }
    void                        check_type              ( const Sos_optional_date* ) {}
};

extern Sos_optional_date_type sos_optional_date_type;

DEFINE_ADD_FIELD( Sos_optional_date, sos_optional_date_type )

typedef Sos_optional_date_type Sos_date_type;

//----------------------------------------------------------------------------As_date_time_type

struct As_date_time_type : Field_subtype
{
                                As_date_time_type       ( const Sos_ptr<Field_type>&, const char* );
                               ~As_date_time_type       ();

    Bool                        null                    ( const Byte* ) const;
    void                        write_text              ( const Byte* p, Area*, const Text_format& f ) const;
    void                        read_text               ( Byte* p, const char*, const Text_format& f ) const;
    void                       _obj_print               ( ::std::ostream* ) const;

  protected:
    void                       _get_param               ( Type_param* ) const;

    Sos_optional_date_time_type _date_type_with_format;
    Bool                       _truncate_at_blank;      // Special flag für js odbcfile, um Zeit herauszufiltern. hm!
    Bool                       _date_only;
};

//---------------------------------------------------------------------------------As_date_type

struct As_date_type : As_date_time_type
{
                                As_date_type            ( const Sos_ptr<Field_type>&, const char* );

    void                        truncate_at_blank       ( Bool b )          { _truncate_at_blank = b; }

  protected:
    void                       _get_param               ( Type_param* ) const;
};

typedef As_date_type Text_date_type;  //alt

//----------------------------------------------------------------------------------------------

Sos_optional_date              as_date      ( const Field_type*   , const Byte*   );
inline Sos_optional_date       as_date      ( const Field_descr* f, const Byte* p )  { return as_date( f->type_ptr(), f->const_ptr( p ) ); }

Sos_optional_date_time         as_date_time ( const Field_type*   , const Byte*   );
inline Sos_optional_date_time  as_date_time ( const Field_descr* f, const Byte* p )  { return as_date_time( f->type_ptr(), f->const_ptr( p ) ); }

double                          seconds_from_hhmmss( const string& s );

#endif

} //namespace sos

#endif
