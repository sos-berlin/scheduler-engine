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
