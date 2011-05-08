// pointer.h
/*
    Simple_ptr<type>
        Pointer fÅr nicht-structs, ohne operator ->, mit operator []
        und Rechenoperationen.

    Ptr<type>
        Pointer fÅr structs, mit operator ->, ohne operator []
        und ohne Rechennoperatoren. sizeof (type) mu· nicht bekannt sein.

    Auto_ptr<type> : Ptr
        del() ruft delete ohne [] auf und setzt den Pointer auf 0.
        ~Auto_ptr ruft del() auf.

    Array_ptr<type> : Ptr
        Mit operator [] und Rechenoperationen.

    Auto_array_ptr<type> : Array_ptr
        Entsprechend Auto_ptr.

    Const_string0_ptr : Simple_ptr<const char>
        FÅr 0-terminierte Strings.

    String0_ptr : Simple_ptr<char>
        Mit Konvertierung nach Const_string0_ptr.

    Auto_string0_ptr : String0_ptr
        Wie Auto_ptr, aber nicht kompatibel dazu.

    Arrays finden sich in chkarray.h
*/

#ifndef __POINTER_H
#define __POINTER_H

namespace sos
{

//#pragma interface

#ifndef STRING
#define STRING(str) #str
#endif

//---------------------------------------------------------------------possible_invalid_pointer

inline int possible_invalid_pointer( const void* ptr, unsigned int size )
// Siehe auch valid() in pointer.cxx
{
#   if defined SYSTEM_DOS  &&  !defined SYSTEM_WIN
        return ! ( (long) FP_OFF( ptr ) + object_size - 1 <= 0xFFFF
                &&  FP_SEG( ptr ) > 0x0800
                &&  (long) FP_SEG( ptr ) * 16 + FP_OFF( ptr ) < 0xA0000 );
#    else
        return ptr == 0;
#   endif
}

//----------------------------------------------------------------------------checked, checked2

#if 1 //defined NDEBUG
    inline const void* checked( const void* ptr, int = 0, const char* = 0 )
    {
        return ptr;
    }
# else
    const void* checked2( const void*, int object_size = 1, const char* debug_info  = SOURCE );  // Pr¸ft Pointer; in pointer.cpp

    inline const void* checked( const void* ptr, int object_size = 1, const char* debug_info = SOURCE )
    {
        if( possible_invalid_pointer( ptr, object_size ) ) {
            return checked2( ptr, object_size, debug_info );
        }
        return ptr;
    }
#endif

//----------------------------------------------------------------------------------checked_ptr

template<class T_ptr>
inline T_ptr checked_ptr( T_ptr ptr )
{
#   if defined( NDEBUG )
        return ptr;
#    else
        return (T_ptr) checked( (const void*) ptr );
#   endif
}

//-----------------------------------------------------------------------------------Simple_ptr

template< class T >
struct Simple_ptr        // Ohne operator ->  fÅr char, int usw.
{
                    Simple_ptr  ()               : _ptr( 0 )  {}
                    Simple_ptr  ( T* ptr )       : _ptr ( ptr )  {}

                    operator T* ()        const  { return _ptr; }
    Simple_ptr      operator +  ( int i ) const  { return _ptr + i;  }
    Simple_ptr      operator -  ( int i ) const  { return _ptr - i;  }
    Simple_ptr&     operator += ( int i )        { _ptr += i; return *this; }
    Simple_ptr&     operator -= ( int i )        { _ptr -= i; return *this; }
    Simple_ptr&     operator ++ ()               { checked(++_ptr,sizeof(T)); return *this; }
    Simple_ptr      operator ++ ( int   )        { Simple_ptr<T> p = *this; ++*this; return p; }
    T&              operator *  ()        const  { return *(T*) checked(_ptr, sizeof(T)); }
    T&              operator [] ( int i ) const  { return *(T*) checked(_ptr+i,sizeof(T));}

  protected:
    T* _ptr;
};

//------------------------------------------------------------------------------Auto_simple_ptr

template< class T >
struct Auto_simple_ptr : Simple_ptr< T >
{
                                Auto_simple_ptr         ()          {}
                               ~Auto_simple_ptr         ()          { del(); }

    Auto_simple_ptr&            operator =              ( T* ptr )  { del(); _ptr = ptr; return *this; }
    void                        del                     ()          { delete _ptr; _ptr = 0; }

  private:
    // F¸hrt delete bei tempor‰rem Objekt aus:
                                Auto_simple_ptr         ( const Auto_simple_ptr& );
                                Auto_simple_ptr         ( T* ptr ) : Ptr<T> ( ptr )  {}
};

//-----------------------------------------------------------------------------Simple_array_ptr

template<class T>
struct Simple_array_ptr : Simple_ptr<T>     // Mit Rechnenoperationen
{
    Simple_array_ptr()  {}
    Simple_array_ptr( T* ptr ) : Simple_ptr<T>( ptr )  { }

    Simple_array_ptr  operator +  ( int i ) const  { return _ptr + i;  }
    Simple_array_ptr  operator -  ( int i ) const  { return _ptr - i;  }
    Simple_array_ptr& operator += ( int i )        { _ptr += i; return *this; }
    Simple_array_ptr& operator -= ( int i )        { _ptr -= i; return *this; }
    T&       operator [] ( int i ) const  { return *(T*) checked(_ptr+i,sizeof(T));}

    void del()                              { delete [] _ptr; _ptr = 0; }
};

//------------------------------------------------------------------------Auto_simple_array_ptr

template< class T >
struct Auto_simple_array_ptr : Simple_array_ptr<T>
{
                                Auto_simple_array_ptr   () {}
                               ~Auto_simple_array_ptr   () { del(); }

    Auto_simple_array_ptr&      operator =              ( T* ptr )  { del(); _ptr = ptr; return *this; }

    void                        del                     ()          { delete [] _ptr; _ptr = 0; }

  private:
    // F¸Åhrt delete bei tempo‰rÑrem Objekt aus:
                                Auto_simple_array_ptr   ( const Auto_simple_array_ptr& );
                                Auto_simple_array_ptr   ( T* ptr ) : Simple_array_ptr<T> ( ptr )  {}
};

// -----------------------------------------------------------------------------------------Ptr

template< class T >
struct Ptr          // Ohne Rechenoperationen, mit operator ->
{
    Ptr() : _ptr ( 0 ) {}
    Ptr( T* ptr ) : _ptr ( ptr )  {}

       operator T* () const  { return _ptr; }
    T& operator *  () const  { return *(T*) checked(_ptr,1); }
    T* operator -> () const  { return  (T*) checked(_ptr,1); }
    T** operator & ()        { return &_ptr; }  // Vorsicht, vorsicht!

  protected:
    T* _ptr;
};

//-------------------------------------------------------------------------------------Auto_ptr

template< class T >
struct Auto_ptr : Ptr< T >
{
                                Auto_ptr                ()          {}
                               ~Auto_ptr                ()          { del(); }

    Auto_ptr&                   operator =              ( T* ptr )  { del(); _ptr = ptr; return *this; }
    void                        del                     ()          { delete _ptr; _ptr = 0; }

  private:
    // F¸hrt delete bei tempor‰rem Objekt aus:
                                Auto_ptr                ( const Auto_ptr& );
                                Auto_ptr                ( T* ptr ) : Ptr<T> ( ptr )  {}
};

//-----------------------------------------------------------------------------------sos_delete
// F¸r DELETE(), s. sos.h

template< class TYPE >
inline void sos_delete( Auto_ptr<TYPE>& ptr )
{
    ptr.del();
}

//------------------------------------------------------------------------------------Array_ptr

template<class T>
struct Array_ptr : Ptr<T>            // Mit Rechnenoperationen
{
    Array_ptr()  {}
    Array_ptr( T* ptr ) : Ptr<T>( ptr )  { }

    Array_ptr  operator +  ( int i ) const  { return _ptr + i;  }
    Array_ptr  operator -  ( int i ) const  { return _ptr - i;  }
    Array_ptr& operator += ( int i )        { _ptr += i; return *this; }
    Array_ptr& operator -= ( int i )        { _ptr -= i; return *this; }
    T&       operator [] ( int i ) const  { return *(T*) checked(_ptr+i,sizeof(T));}

    void del()                              { delete [] _ptr; _ptr = 0; }
};

//-----------------------------------------------------------------------
/*
template< class T >
struct Auto_array_ptr : Array_ptr<T>
{
    Auto_array_ptr()  {}
    ~Auto_array_ptr()  { del(); }

    Auto_array_ptr& operator = ( T* ptr )  { del(); _ptr = ptr; return *this; }

    void del()  { delete [] _ptr; _ptr = 0; }

  private:
    // F¸hrt delete bei tempor‰rem Objekt aus:
    Auto_array_ptr( const Auto_ptr& );
    Auto_array_ptr( T* ptr ) : Ptr<T> ( ptr )  {}
};
*/


} //namespace sos

#endif
