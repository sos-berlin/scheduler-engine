#include "precomp.h"

// $Id: sosdate.cxx 12551 2007-01-20 16:45:09Z jz $
//#define MODULE_NAME "sosdate"
//#define COPYRIGHT   "(c) SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"

#include <stdio.h>          // sprintf

#include <stdlib.h>
#include <time.h>

#include "sosstrng.h"

#include "../kram/sos.h"
#include "../kram/sosfield.h"
#include "../kram/tabucase.h"
#include "../kram/sosctype.h"
#include "../kram/sosdate.h"

#if !defined SYSTEM_INCLUDE_TEMPLATES
//    #include <sosdate.tpl>
#endif

// wg. Sos_optional_date_time_type::input
#include "../kram/log.h"

using namespace std;
namespace sos {

time_t sos_time( time_t *timer );   // sysdep.cxx

//--------------------------------------------------------------------------------------------const

const int max_date_length = 50;

                                                         // Text_format::_date_code
const char* std_date_format_iso      = "yyyy-mm-dd";    // 1
const char* std_date_format_german   = "dd.mm.yyyy";    // 2
const char* std_date_format_ingres   = "dd-mon-yyyy";   // 3
const char* std_date_format_yyyymmdd = "yyyymmdd";      // 4
const char* std_date_format_english  = "mon, dd yyyy";  // 5
const char* std_date_format_odbc     = "{d'yyyy-mm-dd'}";  // 6

// Die folgenden Codes werden auch von hostapi read_field_as_text verwendet:
const char* std_date_format_array [ 16 ] =
{                                   // Text_format::_date_code
    std_date_format_iso,            // 0 default
    std_date_format_iso,            // 1 yyyy-mm-dd
    std_date_format_german,         // 2 dd.mm.yyyy
    std_date_format_ingres,         // 3 dd-mon-yyyy
    std_date_format_yyyymmdd,       // 4 yyyymmdd
    std_date_format_english,        // 5 mon, dd yyyy
    std_date_format_odbc,           // 6 {d'yyyy-mm-dd'}
    "", "", "", "", "", "", "", "", ""  // hostapi setzt _date_code zwischen 0 und 15
};


const char* std_date_time_format_iso        = "yyyy-mm-dd HH:MM:SS";        // 1
const char* std_date_time_format_german     = "dd.mm.yyyy HH:MM:SS";        // 2
const char* std_date_time_format_ingres     = "dd-mon-yyyy HH:MM:SS";       // 3
const char* std_date_time_format_yyyymmddHHMMSS = "yyyymmddHHMMSS";         // 4
const char* std_date_time_format_english    = "mon, dd yyyy HH:MM:SS";      // 5
const char* std_date_time_format_odbc       = "{ts'yyyy-mm-dd HH:MM:SS'}";  // 6

// Die folgenden Codes werden auch von hostapi read_field_as_text verwendet:
const char* std_date_time_format_array [ 16 ] =
{                                   // Text_format::_date_code
    std_date_time_format_iso,            // 0 default
    std_date_time_format_iso,            // 1 yyyy-mm-dd HH:MM:SS
    std_date_time_format_german,         // 2 dd.mm.yyyy HH:MM:SS
    std_date_time_format_ingres,         // 3 dd-mon-yyyy HH:MM:SS
    std_date_time_format_yyyymmddHHMMSS, // 4 yyyymmddHHMMSS
    std_date_time_format_english,        // 5 mon, dd yyyy HH:MM:SS
    std_date_time_format_odbc,           // 6 {d'yyyy-mm-dd HH:MM:SS'}
    "", "", "", "", "", "", "", "", ""  // hostapi setzt _date_code zwischen 0 und 15
};


//                                     Jan Feb M‰r Apr Mai Jun Jul Aug Sep Okt Nov Dez
const uint1 monatstage [ 1+12 ] = { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

const char monthnames3  [ 12 ][4] = { "jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec" };
const char monatsnamen3 [ 12 ][4] = { "jan", "feb", "m‰r", "apr", "mai", "jun", "jul", "aug", "sep", "okt", "nov", "dez" };

const int zero_day = Sos_date( 1970, 1, 1 ).day_count();

//---------------------------------------------------------------------------------------static

Sos_optional_date_time_type sos_optional_date_time_type;
Sos_optional_date_type      sos_optional_date_type;

//---------------------------------------------------------------------------------------------

typedef Sos_date_kind Kind;

//----------------------------------------------------------------------------Text_format::date
// Text_format ist in sosfield.h deklariert.

void Text_format::date( const char* format )
{
	int i;
    for( i = 1; i < NO_OF( std_date_format_array ); i++ ) {
        if( strcmp( format, std_date_format_array[ i ] ) == 0 )  break;
    }

    if( i >= NO_OF( std_date_format_array ) )  throw_xc( "SOS-1205", format );

    _date_code = i;
}

//----------------------------------------------------------------------------Text_format::date
// Text_format ist in sosfield.h deklariert.

const char* Text_format::date() const
{
    return std_date_format_array[ _date_code ];
}

//-----------------------------------------------------------------------Text_format::date_time
// Text_format ist in sosfield.h deklariert.

void Text_format::date_time( const char* format )
{
	int i;
    for( i = 1; i < NO_OF( std_date_time_format_array ); i++ ) {
        if( strcmp( format, std_date_time_format_array[ i ] ) == 0 )  break;
    }

    if( i >= NO_OF( std_date_time_format_array ) )  throw_xc( "SOS-1205", format );

    _date_time_code = i;
}

//-----------------------------------------------------------------------Text_format::date_time
// Text_format ist in sosfield.h deklariert.

const char* Text_format::date_time() const
{
    return std_date_time_format_array[ _date_time_code ];
}

//-----------------------------------------------------------------------------------schaltjahr

Bool schaltjahr( int year )
{
    return year % 4 == 0  &&  ( year % 100 != 0 || year % 400 == 0 );
}

//--------------------------------------------------------------------Sos_optional_date_time::assign

void Sos_optional_date_time::assign( const Sos_string& str, int schwellenjahr )
{
    assign( c_str( str ), schwellenjahr );
}

//--------------------------------------------------------------------Sos_optional_date_time::assign
#ifdef SYSTEM_WIN

void Sos_optional_date_time::assign( const FILETIME& filetime )
{
    SYSTEMTIME systemtime;
    BOOL       ok;

    ok = FileTimeToSystemTime( &filetime, &systemtime );
    if( !ok )  throw_mswin_error( "FileTimeToSystemTime" );

    assign( systemtime );
}

#endif
//--------------------------------------------------------------------Sos_optional_date_time::assign
#ifdef SYSTEM_WIN

void Sos_optional_date_time::assign( const SYSTEMTIME& systemtime )
{
    assign_date( systemtime.wYear, systemtime.wMonth, systemtime.wDay );
    set_time( systemtime.wHour, systemtime.wMinute, systemtime.wSecond );   //systemtime.wMilliseconds; 
}

#endif
//--------------------------------------------------------------------Sos_optional_date_time::assign

void Sos_optional_date_time::_assign_date( int year, int month, int day, int schwellenjahr )
{
    if( year == 0  &&   month == 0  &&  day == 0 ) {
        set_null();   // Sos_date liefert Exception
        return;
    }

    if( year < 100 )  year += year < schwellenjahr? 2000 : 1900;   // default schwellenjahr = 100, also +2000

    if( year  < 1582  ||  year  > 4000
     || month < 1     ||  month > 12
     || day   < 1     ||  day   > 31
     || day   > monatstage[ month ]
     || (   day   == 29             // 29.
        &&  month == 2              // Februar
        &&  !schaltjahr( year ) ) )  //
    {
        char buffer [ 40 ];
        sprintf( buffer, "%02d.%02d.%04d", (int)day, (int)month, (int)year );
        throw_syntax_error( "SOS-1101", buffer );
    }

    _year  = year;
    _month = month;
    _day   = day;
    _time  = 0;
}

//---------------------------------------------------------------Sos_optional_date_time::day_of_year

void Sos_optional_date_time::day_of_year( int doy )
{
    int tag = doy;

    if( tag <= 31 )  { assign_date( year(), 1, tag ); return; }
    tag -= 31;

    int feb = schaltjahr( year() )? 29 : 28;
    if( tag <= feb )  { assign_date( year(), 2, tag ); return; }
    tag -= feb;

    int monat = 3;
    while( monat <= 12 && tag > monatstage[ monat ] )  tag -= monatstage[ monat++ ];

    if( monat > 12 )  throw_xc( "SOS-1180", doy, year() );

    assign_date( year(), monat, tag );
}

//---------------------------------------------------------------Sos_optional_date_time::day_of_year

int Sos_optional_date_time::day_of_year() const
{
    int tag   = day();
    int monat = month();

    if( monat == 1 )  return tag;
    int doy = 31 + tag;

    if( monat == 2 )  return doy;
    doy += schaltjahr( year() )? 29 : 28;

    for( int i = 3; i < monat; i++ )  doy += monatstage[ i ];

    return doy;
}

//-----------------------------------------------------------------Sos_optional_date_time::day_count

int4 Sos_optional_date_time::day_count() const
{
    if( empty() )  throw_null_error( "SOS-1195", "Sos_optional_date_time" );

    int yyyy = year();
    int y    = yyyy - 1900;

    int4 tage = y * 365  +  (y+3) / 4  +  day_of_year() - 1;       // jz 3.1.2001
    if( yyyy > 1900 )  tage--;    // ist kein Schaltjahr
    if( yyyy > 2100 )  tage--;    // ist kein Schaltjahr

    return tage;
}

//-----------------------------------------------------------------Sos_optional_date_time::day_count

void Sos_optional_date_time::day_count( int4 tage )
{
/*
    if( tage >  365                  )  tage++;     // Jahr 1900 ist kein Schaltjahr
    if( tage >= day_count_2101_01_01 )  tage++;     // Jahr 2000 ist kein Schaltjahr

    tage -= ( tage + 3*365 ) / (4*365);
    year( tage / 365 );
    day_of_year( tage % 365 );     int DAS_STIMMT_NICHT;
*/
    if( tage > 300000 )  throw_xc( "SOS-1101" );

    int  y = 1900;
    int4 t = 0;

    while(1) {
        int4 t2 = t + ( schaltjahr( y )? 366 : 365 );
        if( t2 > tage )  break;
        t = t2;
        y++;
    }

    assign_date( y, 1, 1 );
    day_of_year( tage - t + 1 );
}

//------------------------------------------------------------------Sos_optional_date_time::add_days

void Sos_optional_date_time::add_days( int tage )
{
    int new_day = _day + tage;

    if( new_day > 0  &&  new_day <= 28 ) 
    {
        _day = new_day;
    }
    else
    {
        int new_doy = day_of_year() + tage;
    
        if( new_doy > 0  &&  new_doy <= 365 ) 
        {
            day_of_year( new_doy );
        }
        else
        {
            day_count( day_count() + tage );
        }
    }
}

//---------------------------------------------------------Sos_optional_date_time::add_days_and_time

void Sos_optional_date_time::add_days_and_time( double days_and_time )
{
    int tage = (int)days_and_time;
    int time = (int)( ( days_and_time - tage ) * (24*60*60) );

    _time += time;
    
    tage += _time / 24*60*60;
    _time %= 24*60*60;

    add_days( tage );
}

//-----------------------------------------------------------------Sos_optional_date_time::formatted

Sos_string Sos_optional_date_time::formatted( const char* format ) const
{
    Sos_limited_text<100> buffer;
    write_text( &buffer, format );
    return sos::as_string( buffer );
}

//----------------------------------------------------------------Sos_optional_date_time::write_text

void Sos_optional_date_time::write_text( Area* buffer, const char* format ) const
{
    Sos_optional_date_time_type( format ).write_text( (const Byte*)this, buffer );
}

//-----------------------------------------------------------------Sos_optional_date_time::read_text

void Sos_optional_date_time::read_text( const char* text, const char* format )
{
    Sos_optional_date_time_type( format ).read_text( (Byte*)this, text );
}

//-------------------------------------------------------------------Sos_optional_date_time::compare

int Sos_optional_date_time::compare( const Sos_optional_date_time& date ) const
{
    if( null() || date.null() )  throw_null_error( "SOS-1195", "Sos_optional_date_time" );

    if( _year  < date._year  )  return -1;
    if( _year  > date._year  )  return +1;
    if( _month < date._month )  return -1;
    if( _month > date._month )  return +1;
    if( _day   < date._day   )  return -1;
    if( _day   > date._day   )  return +1;

    if( _time < date._time )  return -1;
    if( _time > date._time )  return +1;

    return 0;
}

//----------------------------------------------------------------Sos_optional_date_time::set_null

void Sos_optional_date_time::set_null()
{
    _year  = 0;
    _month = 0;
    _day   = 0;

    _time  = 0;
}

//------------------------------------------------------------Sos_optional_date_time::set_time

void Sos_optional_date_time::set_time( time_t time )
{
    if( _date_only  &&  time != 0 ) {
        throw_xc( "SOS-1223" );
    }

    _time = time;
}

//------------------------------------------------------------Sos_optional_date_time::as_time_t

time_t Sos_optional_date_time::as_time_t() const
{
    if( null() )
    {
        return 0;
    }
    else
    {
        return ( day_count() - zero_day ) * 24*60*60 + _time;
    }
}

//-----------------------------------------------------------Sos_optional_date_time::set_time_t

void Sos_optional_date_time::set_time_t( time_t tim )
{
    if( tim == 0 )
    {
        set_null();
    }
    else
    {
        day_count( zero_day + tim / (24*60*60) );
        set_time( tim % (24*60*60) );
    }
}

//----------------------------------------------------------------------------last_day_of_month

int last_day_of_month( const Sos_date& date )
{
    if( date.month() == 2 )  return schaltjahr( date.year() )? 29 : 28;
    return monatstage[ date.month() ];
}

//---------------------------------------------------------------------------------------------

Sos_optional_date_time ultimo( const Sos_optional_date_time& date )
{
    Sos_optional_date_time d = date;

    if( !d.null() )  d.assign_date( d.year(), d.month(), last_day_of_month( as_date( date ) ) );

    return d;
}

//----------------------------------------------------------------------------------operator <<

ostream& operator << ( ostream& s, const Sos_optional_date_time& date )
{
    Sos_limited_text<50> text;
    date.write_text( &text );
    return s << text;
}

//----------------------------------------------------------------------------------operator >>
/* In MS C++ nicht implementiert
istream& operator >> ( istream& s, Sos_optional_date_time& date )
{
    Sos_string str;
    s >> str;
    date.assign( str );
    return s;
}
*/
//-------------------------------------------------------------Sos_future_date::Sos_future_date
/*
Sos_future_date::Sos_future_date( const Sos_optional_date& date )
{
    if( date < Sos_date::today() ) {
        char buffer [ 40 ];
        sprintf( buffer, "%d.%d.%d", (int)day(), (int)month(), (int)year() );
        throw_syntax_error( "SOS-1102", buffer );
    }

    *(Sos_optional_date_time*)this = date;
}

//---------------------------------------------------------------------Sos_future_date::_assign

void Sos_future_date::_assign( uint2 year, uint1 month, uint1 day, int schwellenjahr )
{
    int future_date_zweistellig_fuer_2000_testen;
    if( year <= 99 ) {
        year += 1900;
        if( year < Sos_date::today().year() - 10 )  year += 100;
    }

    *this = Sos_optional_date( year, month, day );
}

//-----------------------------------------------------------------Sos_past_date::Sos_past_date

Sos_past_date::Sos_past_date( const Sos_optional_date& date )
{
    if( date > Sos_date::today() ) {
        char buffer [ 40 ];
        sprintf( buffer, "%d.%d.%d", (int)day(), (int)month(), (int)year() );
        throw_syntax_error( "SOS-1103", buffer );
    }

    *(Sos_optional_date*)this = (Sos_optional_date)date;
}

//-----------------------------------------------------------------------Sos_past_date::_assign

void Sos_past_date::_assign( uint2 year, uint1 month, uint1 day, int schwellenjahr )
{
    int past_date_zweistellig_fuer_2000_testen;
    if( year <= 99 ) {
        year += 2000;
        while( year > Sos_date::today().year() + 10 )  year -= 100;
    }
    *this = Sos_optional_date( year, month, day );
}
*/
//---------------------------------------------------------------------------Sos_date::Sos_date

Sos_date::Sos_date( const Sos_optional_date_time& date )
{
    if( date.empty() )  throw_null_error( "SOS-1195", "Sos_optional_date_time" );

    assign( date.year(), date.month(), date.day() );
}

//---------------------------------------------------------------------------Sos_date::set_null

void Sos_date::set_null()
{
    throw_null_error( "SOS-1186", "Sos_date" );
}

//------------------------------------------------------------------------------Sos_date::today

Sos_date Sos_date::today()
{
    time_t t;
    sos_time( &t );
    struct tm* tm_ptr = localtime( &t );
    return Sos_date( 1900 + tm_ptr->tm_year, tm_ptr->tm_mon + 1, tm_ptr->tm_mday );
}

//-------------------------------------------------------------------------Sos_date::operator +

Sos_date Sos_date::operator+ ( int4 i ) const
{
    return Sos_date( *this ) += i;
}

//-------------------------------------------------------------------------Sos_date::operator -

Sos_date Sos_date::operator- ( int4 i ) const
{
    return Sos_date( *this ) -= i;
}

//----------------------------------------------------------------------Sos_optional_date_time::now

Sos_optional_date_time Sos_optional_date_time::now()
{
    time_t t;
    sos_time( &t );
    struct tm* tm_ptr = localtime( &t );

    Sos_optional_date_time now;

    now.assign_date( 1900 + tm_ptr->tm_year, tm_ptr->tm_mon + 1, tm_ptr->tm_mday );
    now.set_time( tm_ptr->tm_hour, tm_ptr->tm_min, tm_ptr->tm_sec );

    return now;
}

//-----------------------------------------------------------Sos_optional_date_time_type::_type_info

Listed_type_info Sos_optional_date_time_type::_type_info;

SOS_INIT( date_time )
{
    Sos_optional_date_time_type::_type_info._std_type    = std_type_date_time;      // _std_type
    Sos_optional_date_time_type::_type_info._name        = "Sos_date_time";         // _name
    Sos_optional_date_time_type::_type_info._nullable    = true;                    // _nullable,
    Sos_optional_date_time_type::_type_info.normalize();
};

//-------------------------------------------------------------Sos_optional_date_time_type::destruct

void Sos_optional_date_time_type::destruct( Byte* p ) const
{
    ((Sos_optional_date_time*)p)->~Sos_optional_date_time();
}

//-----------------------------------------------------------Sos_optional_date_time_type::_get_param

void Sos_optional_date_time_type::_get_param( Type_param* param ) const
{
    param->_info_ptr     = &_type_info;
    param->_precision    = length( _format );
    param->_display_size = param->_precision;
}

//-----------------------------------------------------------Sos_optional_date_time_type::write_text

void Sos_optional_date_time_type::write_text( const Byte* p, Area* buffer, const Text_format& text_format ) const
{
    char        text [ 50 ];
    char*       b             = text;
    Bool        time_ok       = false;
    const Sos_optional_date_time& date = *(Sos_optional_date_time*)p;
    const char* f             = date.date_only()? text_format.date_code()      || _format == "" ? text_format.date()
                                                                                                : c_str( _format )
                                                : text_format.date_time_code() || _format == "" ? text_format.date_time()
                                                                                                : c_str( _format )                                      ;

    if( date.null() )  { buffer->set_length( 0 ); return; }

    while( *f ) {
        if( *f == 'y'  &&  *(f+1) == 'y' ) {
            f += 2;
            if( *f == 'y'  &&  *(f+1) == 'y' ) {
                f += 2;
                int n = date.year();
                *b++ = Byte( n / 1000 + '0' );
                *b++ = Byte( n / 100 % 10 + '0' );
                *b++ = Byte( n / 10 % 10 + '0' );
                *b++ = Byte( n % 10 + '0' );
            } else {
                int n = date.year();
                *b++ = Byte( n % 100 / 10 + '0' );
                *b++ = Byte( n % 10 + '0' );
                if( sos_isdigit( f[0] )  &&  sos_isdigit( f[1] ) )  f += 2;     // Schwelle f¸r 1900/2000 wird beim Schreiben ignoriert
            }
        }
        else
        if( *f == 'm' ) {
            f++;
            int n = date.month();
            if( *f == 'm' ) {
                f++;
                *b++ = Byte( n / 10 + '0' );
                *b++ = Byte( n % 10 + '0' );
            }
            else
            if( *f == 'o' && *(f+1) == 'n' ) {
                f += 2;
                if( n >= 1 && n <= 12 ) {
                    memcpy( b, monthnames3[ n - 1 ], 3 );
                    b+= 3;
                } else {
                    *b++ = Byte( n / 10 + '0' );
                    *b++ = Byte( n % 10 + '0' );
                }
            }
        }
        else
        if( *f == 'd'  &&  *(f+1) == 'd' ) {
            f += 2;
            int n = date.day();
            *b++ = Byte( n / 10 + '0' );
            *b++ = Byte( n % 10 + '0' );
        }
        else
        if( f[0] == 'H'  &&  f[1] == 'H' ) {
            f += 2;
            int n = date.hour();
            *b++ = Byte( n / 10 + '0' );
            *b++ = Byte( n % 10 + '0' );
            time_ok = true;
        }
        else
        if( f[0] == 'M'  &&  f[1] == 'M' ) {
            f += 2;
            int n = date.minute();
            *b++ = Byte( n / 10 + '0' );
            *b++ = Byte( n % 10 + '0' );
        }
        else
        if( f[0] == 'S'  &&  f[1] == 'S' ) {
            f += 2;
            int n = date.second();
            *b++ = Byte( n / 10 + '0' );
            *b++ = Byte( n % 10 + '0' );
        }
        else
        {
            *b++ = *f++;
        }
    }

    //jz 17.10.00  if( !date.date_only()  &&  !time_ok ) {
    //jz 17.10.00      sprintf( b, " %02d:%02d:%02d", (int)date.hour(), (int)date.minute(), (int)date.second() );
    //jz 17.10.00      b += 9;
    //jz 17.10.00  }

    buffer->assign( text, b - text );
}

//----------------------------------------------------------Sos_optional_date_time_type::read_text

void Sos_optional_date_time_type::read_text( Byte* p, const char* text, const Text_format& text_format ) const
{
    Sos_optional_date_time*      date_ptr  = (Sos_optional_date_time*)p;
    int          error     = false;
    const char*  f         = date_ptr->date_only()? text_format.date_code()      || _format == "" ? text_format.date()
                                                                                                  : c_str( _format )
                                                  : text_format.date_time_code() || _format == "" ? text_format.date_time()
                                                                                                  : c_str( _format );
    int          schwellenjahr = default_schwellenjahr;
    Bool         yyyy_used = false;
    int          year      = 0;
    int          month     = 0;
    int          day       = 0;
    int          hour      = 0;
    int          minute    = 0;
    int          second    = 0;
  //Bool         year_set  = false;
    Bool         month_set = false;
    Bool         day_set   = false;

    const char* b = text;

    while( *f ) {
        while( *b == ' ' )  b++;
        if( *f == 'y'  &&  *(f+1) == 'y' ) {
            f += 2;
            if( sos_isdigit( *b ) ) {
              //year_set = true;
                year = *b++ - '0';
                if( sos_isdigit( *b ) )  year = year*10 + *b++ - '0';
            }

            if( *f == 'y'  &&  *(f+1) == 'y' )  {
                f += 2;
                yyyy_used = true;
                if( sos_isdigit( *b ) )  year = year*10 + *b++ - '0';
                if( sos_isdigit( *b ) )  year = year*10 + *b++ - '0';
            }
            else 
            if( sos_isdigit( f[0] )  &&  sos_isdigit( f[1] ) ) {           // "yy30" -> yy=29: 2029, yy=30: 1930
                schwellenjahr = ( f[0] - '0' ) * 10  +  f[1] - '0';
                f += 2;
                //year += year < schwellenjahr? 2000 : 1900;
            }

            //if( year_set  &&  !yyyy_used  &&  year <= 99 )  year += 1900;
        }
        else
        if( *f == 'm'  &&  *(f+1) == 'm' ) {
            f += 2;
            if( sos_isdigit( *b ) ) {
                month_set = true;
                month = *b++ - '0';
                if( sos_isdigit( *b ) )  month = month*10 + *b++ - '0';
            }
        }
        else
        if( *f == 'd'  &&  *(f+1) == 'd' ) {
            f += 2;
            if( sos_isdigit( *b ) ) {
                day_set = true;
                day = *b++ - '0';
                if( sos_isdigit( *b ) )  day = day*10 + *b++ - '0';
            }
        }
        else
        if( f[0] == 'H'  &&  f[1] == 'H' ) {
            f += 2;
            if( sos_isdigit( *b ) ) {
                hour = *b++ - '0';
                if( sos_isdigit( *b ) )  hour = hour*10 + *b++ - '0';
            }
        }
        else
        if( f[0] == 'M'  &&  f[1] == 'M' ) {
            f += 2;
            if( sos_isdigit( *b ) ) {
                minute = *b++ - '0';
                if( sos_isdigit( *b ) )  minute = minute*10 + *b++ - '0';
            }
        }
        else
        if( f[0] == 'S'  &&  f[1] == 'S' ) {
            f += 2;
            if( sos_isdigit( *b ) ) {
                second = *b++ - '0';
                if( sos_isdigit( *b ) )  second = second*10 + *b++ - '0';
            }
        }
        else
        {
            if( *f++ != *b++ && !error )  { error = b-text+1; } // Position beginnt ab 1
        }
    }

    while( *b == ' ' )  b++;
    if( *b )  error = true;

    if( !error ) {
        if( ( !yyyy_used  &&  ( year == 1900 || year == 2000 ) ||  year == 0 )
         && month == 0
         && day == 0 )
        {
            // Uhrzeit wird ignoriert!?
            date_ptr->set_null();
        }
        else
        {
            if( !day_set ) {
                day = 1;                     // Ohne Tag: Der Erste des Monats
                if( !month_set )  month = 1; // Ohne Monat: Der 1. Januar des Jahres
            }
            try {
                date_ptr->assign_date( year, month, day, schwellenjahr );
                date_ptr->set_time( hour, minute, second );
            }
            catch( Xc& x ) {
                x.insert( text );
                throw x;
            }
        }
    } else {
        date_ptr->assign( text, schwellenjahr );     // automatische Erkennung versuchen
    }
}

//-----------------------------------------------------------Sos_optional_date_type::_type_info

Listed_type_info Sos_optional_date_type::_type_info;

SOS_INIT( date )
{
    Sos_optional_date_type::_type_info._std_type    = std_type_date;      // _std_type
    Sos_optional_date_type::_type_info._name        = "Sos_date";         // _name
    Sos_optional_date_type::_type_info._nullable    = true;               // _nullable,
    Sos_optional_date_type::_type_info.normalize();
};

//--------------------------------------------------------------------------------append_format
/*
static void append_format( Area* format, const char* text )
{
    if( format )  format += text;
}

//--------------------------------------------------------------------------------append_format

static void append_format( Area* format, char c )
{
    if( format )  format += c;
}
*/
//------------------------------------------------------------------Sos_optional_date_time::get_part

void Sos_optional_date_time::get_part( const char** pp, int* part, Sos_date_kind* kind/*, Area* format*/ )
{
    const char* p = *pp;
    int n;

    while( *p == ' ' )  { /*if( format) append_format( format, *p );*/  p++; }

    if( sos_isalpha( *p ) )   // Monatsname
    {
        const char* pm = p;
        while( sos_isalpha( *p ) )  p++;

        if( p - pm != 3 )  { /*append_format( format, pm );*/  return; }

        int  i;
        char mon [ 3 ];
        xlat( mon, pm, 3, tablcase );

        for( i = 0; i < 12; i++ )  if( memcmp( mon, monthnames3[ i ], 3 ) == 0 )  break;
        if( i < 12  ) {
            //if( format )  append_format( format, "mon" );
        } else {
            for( i = 0; i < 12; i++ )  if( memcmp( mon, monatsnamen3[ i ], 3 ) == 0 )  break;
            //if( i < 12 )  if( format )  append_format( format, "mnt" );
        }

        if( i == 12 )  { /*append_format( format, pm );*/  return; }

        *part = i + 1;
        *kind = kind_month;
    }
    else
    {
        n = 0;
        while( sos_isdigit( *p ) )  n = n*10 + *p++ - '0';
        //jz4.5.98 if( n > 31 )  *kind = kind_year;
        if( n >= 100 ) {
            *kind = kind_year;
        }/* else {
            if( *p == ':' ) {
                _set_time( pp );
            }
        }*/
        *part = n;
    }

    while( *p == ' ' )  p++;
    *pp = p;
}

//--------------------------------------------------------------------Sos_optional_date_time::assign

void Sos_optional_date_time::assign( const char* date, int schwellenjahr )
{
    const char*  p         = date;
    uint4        time      = 0;

    _time = 0;

    //format->length( 0 );

    // yyyymmdd, yymmdd, yydoy:
    while( *p == ' ' )  {
        //if( format )  append_format( *p );
        p++;
    }

    if( !*p ) {
        set_null();
        return;
    }

    if( strncmp( p, "today", 5 ) == 0  &&  ( p += 5 )
     || strncmp( p, "now"  , 3 ) == 0 )
    {
        Sos_optional_date_time td = Sos_date::today();
        _year  = td._year;
        _month = td._month;
        _day   = td._day;
    }
    else
    {
        const char* p2 = p;
        while( sos_isdigit( *p2 ) )  p2++;

        if( *p2 == '\0'  ||  *p2 == ' ' ) 
        {
            if( p2 - p == 8 ) {  // yyyymmdd
                p = p2;
                //if( format )  append_format( format, "yyyymmdd" );
                assign_date( (p[0] - '0') * 1000 + (p[1] - '0') * 100 + (p[2] - '0') * 10 + p[3] - '0',
                             (p[4] - '0') * 10 + p[5] - '0',
                             (p[6] - '0') * 10 + p[7] - '0' );
                goto DATE_OK;
            }
            else
            if( p2 - p == 6 ) {  // yymmdd
                p = p2;
                //if( format )  append_format( format, "yymmdd" );
                assign_date( (p[0] - '0') * 10 + p[1] - '0',
                             (p[2] - '0') * 10 + p[3] - '0',
                             (p[4] - '0') * 10 + p[5] - '0',
                             schwellenjahr );
                goto DATE_OK;
            }
            else
            if( p2 - p == 5 ) {  // yydoy    Tag des Jahres
                p = p2;
                //if( format )  append_format( format, "yydoy" );
                assign_date( (p[0] - '0') * 10 + p[1] - '0', 1, 1, schwellenjahr );
                day_of_year( (p[2] - '0') * 100 + (p[3] - '0')*10 + p[4] - '0' );
                goto DATE_OK;
            }
            if( p2 - p == 7 ) {  // yyyydoy    Tag des Jahres
                p = p2;
                //if( format )  append_format( format, "yyyydoy" );
                assign_date( (p[0] - '0') * 1000 + (p[1] - '0') * 100 + (p[2] - '0') * 10 + p[3] - '0', 1, 1 );
                day_of_year( (p[4] - '0') * 100 + (p[5] - '0')*10 + p[6] - '0' );
                goto DATE_OK;
            }
        }
        else
        {
            Kind         a_kind    = kind_none;
            int          a         = 0;             // 1. Bestandteil
            char         ab        = ' ';           // 1. Trenner
            Kind         b_kind    = kind_none;
            int          b         = 0;             // 2. Bestandteil
          //char         bc        = ' ';           // 2. Trenner
            Kind         c_kind    = kind_none;
            int          c         = 0;             // 3. Bestandteil

            // Bestandteile lesen:
        
            if( sos_isdigit( p[0] )  &&  sos_isdigit( p[1] )  &&  p[2] == ':' )  _set_time( &p );
        
            get_part( &p, &a, &a_kind );
        
            if( *p == '.' || *p == '/' || *p == '-' )  {
                //if( format )  format += *p;
                ab = *p++;
                while( *p == ' ' )  { /*if( format )  append_format( format, *p );*/  p++; }
            }

            if( sos_isdigit( p[0] )  &&  sos_isdigit( p[1] )  &&  p[2] == ':' )  _set_time( &p );
        
            get_part( &p, &b, &b_kind );
        
            if( *p == '.' || *p == '/' || *p == '-' ) {
                //if( format )  format += *p;
                /*bc = **/p++;
                while( *p == ' ' )  { /*if( format )  append_format( format, *p );*/  p++; }
            }

            // Unix: "Mon dd HH:MM:SS yyyy"
            if( sos_isdigit( p[0] )  &&  sos_isdigit( p[1] )  &&  p[2] == ':' )  _set_time( &p );

            get_part( &p, &c, &c_kind );

            time = _time;   // assign_date() setzt _time = 0

            // Syntax:
          //if( ab != bc ) goto error;

            // Interpretieren:
            if( a_kind == kind_year
             && ( b_kind == kind_month || b_kind == kind_none )
             && c_kind == kind_none )
            {
                assign_date( a, b, c, schwellenjahr );                 // jahr monat tag
                goto DATE_OK;
            }
            else
            if( c_kind == kind_year ) {                           // xx xx jahr
                if( b_kind == kind_month )  {
                    assign_date( c, b, a, schwellenjahr );             // tag monat jahr
                    goto DATE_OK;
                }
                else
                if( a_kind == kind_month )  {
                    assign_date( c, a, b, schwellenjahr );             // monat tag jahr
                    goto DATE_OK;
                }
            }

            if( ab == '/' ) {       // monat/tag/jahr
                if( ( a_kind==kind_none || a_kind==kind_month ) && b_kind==kind_none && ( c_kind==kind_none || c_kind==kind_year) ) {
                    assign_date( c, a, b, schwellenjahr );             // monat/tag/jahr
                    goto DATE_OK;
                }
                else
                if( a_kind==kind_none && b_kind==kind_month && c_kind==kind_none ) {
                    assign_date( c, b, a, schwellenjahr );             // tag/monat/jahr
                    goto DATE_OK;
                }
            }
            else
            if( ab == '.' ) {
                if( a_kind==kind_none
                 && ( b_kind==kind_none || b_kind==kind_month )
                 && ( c_kind==kind_none || c_kind==kind_year) )
                {
                    assign_date( c, b, a, schwellenjahr );             // tag.monat.jahr
                    goto DATE_OK;
                }
                else
                if( a_kind==kind_year && b_kind==kind_none && c_kind==kind_none ) {
                    assign_date( a, b, c, schwellenjahr );             // jahr.monat.tag
                    goto DATE_OK;
                }
            }
        }
        goto error;
    }

DATE_OK:
    while( *p == ' ' )  p++;
    if( *p != '\0' )  {
        if( time != 0 )  goto error;
        //jz 28.7.98 00:00:00 soll erlaubt sein bei _date_only.  if ( _date_only ) throw_syntax_error( "SOS-1393", p );
        set_time( p ); 
    } else {
        _time = time;
    }
    return;

error:
    throw_syntax_error ( "SOS-1104", date, p - date );
}

//---------------------------------------------------------------Sos_optional_date_time::set_time

void Sos_optional_date_time::set_time( const char* time )
{
    const char* p = time;

    _set_time( &p );

    while( *p == ' ' )   p++;
    if( *p )  throw_syntax_error ( "SOS-1386", time, p - time );
}

//--------------------------------------------------------------Sos_optional_date_time::_set_time

void Sos_optional_date_time::_set_time( const char** time_ptr )
{
    const char*  p = *time_ptr;
    uint         hh;
    uint         mm;
    uint         ss;

    while( *p == ' ' )   p++;

    if( !*p )  { set_null(); return; }

    if( strncmp( p, "now", 3 ) == 0 )
    {
        p += 3;
        Sos_optional_date_time nw = now();
        set_time( nw._time );
    }
    else
    {
        if( *p == 'T' )  p++;        // XML-Zeit: yyyy-mm-ddT00:00:00

        // Stunde
        hh = *p++ - '0';
        if( hh >= 10 )  goto FEHLER;

        if( sos_isdigit( *p ) ) {
            hh *= 10;
            hh += *p++ - '0';
        }

        if( *p == ':'  ||  *p == '.' )  p++;

        // Minute
        mm = *p++ - '0';
        if( mm >= 10 )  goto FEHLER;

        if( sos_isdigit( *p ) ) {
            mm *= 10;
            mm += *p++ - '0';
        }

    
        // Sekunde
        ss = 0;

        if( *p == ':'  ||  *p == '.'  ||  sos_isdigit( *p ) ) 
        {
            if( *p == ':'  ||  *p == '.' )  p++;

            ss = *p++ - '0';

            if( sos_isdigit( *p ) ) {
                ss *= 10;
                ss += *p++ - '0';
            }
        }

        if( *p == '.' )     // hh.mm.ss.000 tolerieren (liefert Oracle-Thin-JDBC-Treiber, jz 3.4.03)
        {
            p++;
            while( *p == '0' )  p++;
        }

        // PM/AM
        while( *p == ' ' )   p++;

        if( sos_tolower( p[0] ) == 'a'  &&  sos_tolower( p[1] ) == 'm' ) {
            p += 2;
            if( hh == 12 )  hh = 0;          // Ist 12:00:00 AM Mitternacht oder Mittag?
        }
        else
        if( hh <= 12  &&  sos_tolower( p[0] ) == 'p'  &&  sos_tolower( p[1] ) == 'm' ) { 
            p += 2;  
            if( hh < 12 )  hh += 12;         // Ist 12:00:00 PM Mitternacht oder Mittag?
        }

        set_time( hh, mm, ss );
    }

    while( *p == ' ' )   p++;
    *time_ptr = p;
    return;

FEHLER:
    throw_syntax_error ( "SOS-1386", *time_ptr, p - *time_ptr );
}

//------------------------------------------------------------------------------------as_string

Sos_string as_string( const Sos_optional_date_time& date, const char* format )
{
    Sos_limited_text<max_date_length> buffer;
    //Text_format text_format;
    //text_format.string_quote(0);
    Sos_optional_date_time_type( format ).write_text( (const Byte*)&date, &buffer );
    return as_string( buffer );
}

//-------------------------------------------------------------------As_date_time_type::As_date_time_type

As_date_time_type::As_date_time_type( const Sos_ptr<Field_type>& base_type, const char* format )
:
    Field_subtype( &_type_info, base_type ),
    _date_type_with_format( format ),
    _truncate_at_blank( false )
{
    _date_only = false;
    _empty_is_null = true;
    //_empty_format.string_quote( 0 );
    //_empty_format.separator( 0 );

    _type_info._std_type    = std_type_date_time;
    _type_info._name        = "As_date_time_type";
    _type_info._nullable    = true;
    _type_info.normalize();
}

//-----------------------------------------------------------As_date_time_type::~As_date_time_type

As_date_time_type::~As_date_time_type()
{
}

//--------------------------------------------------------------As_date_time_type::_obj_print

void As_date_time_type::_obj_print( ostream* s ) const
{
    *s << *_base_type << ":Date(\"" << _date_type_with_format._format << "\")";
}

//--------------------------------------------------------------As_date_time_type::_get_param

void As_date_time_type::_get_param( Type_param* param ) const
{
    param->_info_ptr     = &_type_info;
  //param->_precision    = length( _date_type_with_format.format() );
    param->_precision    = strlen( std_date_time_format_array[ 0 ] );  // default ISO: "1996-10-18"
    param->_display_size = param->_precision;
}

//--------------------------------------------------------------------As_date_time_type::null

Bool As_date_time_type::null( const Byte* p ) const
{
    return Field_subtype::null( p );
  //return _base_type->empty( p );  // schlieﬂt null() ein
}

//--------------------------------------------------------------As_date_time_type::write_text

void As_date_time_type::write_text( const Byte* p, Area* buffer, const Text_format& f ) const
{
    Sos_limited_text<max_date_length>  text;
    Sos_optional_date_time             date;

    date.date_only( _date_only );

    _base_type->write_text( p, &text );

    if( _truncate_at_blank ) {
        char* p = (char*)memchr( text.char_ptr(), ' ', text.length() );
        if( p ) {
            text.length( p - text.char_ptr() );
        }
    }

    _date_type_with_format.read_text( (Byte*)&date, c_str( text ) );
    sos_optional_date_time_type.write_text( (const Byte*)&date, buffer, f );
}

//--------------------------------------------------------------------As_date_time_type::print

void As_date_time_type::read_text( Byte* p, const char* t, const Text_format& f ) const
{
    Sos_optional_date_time             date;
    Sos_limited_text<max_date_length>  text;

    date.date_only( _date_only );

    sos_optional_date_time_type.read_text( (Byte*)&date, t, f );
    _date_type_with_format.write_text( (const Byte*)&date, &text );
    _base_type->read_text( p, c_str( text ) );
}

//-------------------------------------------------------------------As_date_type::As_date_type

As_date_type::As_date_type( const Sos_ptr<Field_type>& base_type, const char* format )
:
    As_date_time_type( base_type, format )
{
    _empty_is_null = true;
    _date_only = true;
    _truncate_at_blank = false;

    _type_info._std_type    = std_type_date;
    _type_info._name        = "As_date_type";
    _type_info._nullable    = true;
    _type_info.normalize();
}

//-------------------------------------------------------------------As_date_type::_get_param

void As_date_type::_get_param( Type_param* param ) const
{
    param->_info_ptr     = &_type_info;
  //param->_precision    = length( _date_type_with_format.format() );
    param->_precision    = strlen( std_date_format_array[ 0 ] );  // default ISO: "1996-10-18"
    param->_display_size = param->_precision + 9;  //?? " HH:MM:SS"
}

//--------------------------------------------------------------------------------------as_date

Sos_optional_date as_date( const Field_type* t, const Byte* p )
{
    if( t == &sos_optional_date_type      )  return *(Sos_optional_date*)p;

    Sos_limited_text<100> buffer;
    t->write_text( p, &buffer );
    return Sos_optional_date( c_str( buffer ) );
}

//---------------------------------------------------------------------------------as_date_time

Sos_optional_date_time as_date_time( const Field_type* t, const Byte* p )
{
    if( t == &sos_optional_date_time_type )  return *(Sos_optional_date_time*)p;
    if( t == &sos_optional_date_type      )  return *(Sos_optional_date*)p;

    Sos_limited_text<100> buffer;
    t->write_text( p, &buffer );
    return Sos_optional_date_time( c_str( buffer ) );
}

//-----------------------------------------------------------------------------seconds_from_hhmmss

double seconds_from_hhmmss( const string& s )
{
    if( s.find( ':' ) != string::npos )
    {
        Sos_optional_date_time dt;
        dt.set_time( s );
        return dt.time_as_double();
    }
    else
    {
        return as_double( s );
    }
}


} //namespace sos
