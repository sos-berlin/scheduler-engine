// $Id: spooler_common.h,v 1.1 2001/01/25 20:28:38 jz Exp $

#ifndef __SPOOLER_COMMON_H
#define __SPOOLER_COMMON_H

namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------Handle

struct Handle
{
#   ifdef SYSTEM_WIN
                                Handle                      ( HANDLE h = NULL )             : _handle(h) {}
                               ~Handle                      ()                              { close(); }

        void                    operator =                  ( HANDLE h )                    { close(); _handle = h; }
                                operator HANDLE             () const                        { return _handle; }
                                operator !                  () const                        { return _handle == 0; }
        HANDLE*                 operator &                  ()                              { return &_handle; }

        void                    close                       ()                              { if(_handle) { CloseHandle(_handle); _handle=0; } }

        HANDLE                 _handle;
#   endif
};

//--------------------------------------------------------------------------------------------Mutex
// Nur für Typen, die in ein Speicherwort passen, also mit genau einem Maschinenbefehl lesbar sind.

template<typename T>
struct Mutex
{
    typedef sos::Thread_semaphore::Guard Guard;


                                Mutex                       ( const T& t = T() )    : _value(t) {}

    Mutex&                      operator =                  ( const T& t )          { Guard g = &_semaphore; _value = t; return *this; }
                                operator T                  ()                      { return _value; }  // Nicht gesichert
    T                           read_and_reset              ()                      { Guard g = &_semaphore; T v = _value; _value = T(); return v; }
    T                           read_and_set                ( const T& t )          { Guard g = &_semaphore; T v = _value; _value = t; return v; }

    sos::Thread_semaphore      _semaphore;
    volatile T                 _value;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
