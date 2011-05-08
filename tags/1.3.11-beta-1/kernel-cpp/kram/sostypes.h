#include <tools.hxx>
#include <string.h>
#include <sos.h>

struct Sos_date : Date 
{
                Sos_date        () : Date(0,0,0);
                Sos_date        ( uint4 date ) : Date(date);
                Sos_date        ( uint day, uint month, uint year ) :
                  Date( day, month, year ) {};

  uint4         date            () const { return GetDate(); };
  void          date            ( uint4 date ) { ChangeDate( date ); };

  uint          year            () const { return GetYear(); };
  uint          month           () const { return GetMonth(); };
  uint          day             () const { return GetDay(); };

  void          year            ( uint y ) { ChangeYear(y); };
  void          month           ( uint m ) { ChangeMonth(m); };
  void          day             ( uint d ) { ChangeDay(d); };

  Bool          valid           () { return IsValid(); };

};


struct Sos_time : Time
{
                Sos_time        () : Time(0,0) {};
                Sos_time        ( uint hour, uint minute ) : Time( hour, minute ) {};

  uint          hour            () { return GetHour(); };
  uint          minute          () { return GetMinute(); };
  void          hour            ( uint h ) { ChangeHour(h); };
  void          minute          ( uint m ) { ChangeMin(m); };

  Bool          valid           ();
};

template <uint size>
struct Sos_fixed_text : Area {
                Sos_fixed_text  ( const char* str = 0 ) : Area(_text,size) { text(str); };          
  operator      const char*     () const { return _text; };
  const char*   text            () const { return _text; };
  void          text            ( const char* str = 0 );
private:
  char _text[size+1];
};

template <uint size>
inline void Sos_fixed_text<size>::Sos_fixed_text( const char* str ) {
  if ( str == 0 ) {
    _text[0] = 0;
    length(0);
    return;
  }
  uint len = min( strlen( str ), size() );
  strncpy( _text, str, len );
  _text[len] = 0;
  length(len);
};

template <uint first, uint last>
struct Sos_number {
                Sos_number  () : _number(first), _nks(0) {}; 
                Sos_number  ( uint4 n, nks = 0 ) : _nks(nks) { number(n); };         
  operator      uint4() const { return _number; };
  uint4         number() { return _number; };
  void          number( uint4 n ) { if (n>=first&&n<=last) _number = n; };
private:
  uint4  _number;
  uint   _nks;
};
