// $Id$        © 2000 Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

// §1719


#ifndef __ZSCHIMMER_H
#define __ZSCHIMMER_H

#include "base.h"
#include "Memory_allocator.h"
//#include "com_base.h"


#ifdef Z_WINDOWS
#   define _CRTDBG_MAPALLOC
#   include <crtdbg.h>
#endif

#ifdef Z_UNIX
#   include "com.h"
#endif

#ifdef __GNUC__
#   include <unistd.h>
#endif

#ifdef INTERFACE
#   undef INTERFACE     // Für Windows
#endif

//----------------------------------------------------------------------------------------warnings
#if defined _MSC_VER
#   pragma warning( 3      :4100 )   // Unreferenzierter formaler Parameter
#endif
//-----------------------------------------------------------------------------------namespace std

using ::std::string;
using ::std::exception;

//--------------------------------------------------------------------------------ostream << size_t
#ifdef Z_WINDOWS

//? inline std::ostream&             operator <<                 ( std::ostream& s, size_t value )      { s << (zschimmer::uint64)value; }

#endif
//------------------------------------------------------------------------------namespace zschimmer

namespace zschimmer
{

//----------------------------------------------------------------------------------------exception

inline std::ostream& operator << ( std::ostream& s, const std::exception& x )
{
    return s << x.what();
}

//-------------------------------------------------------------------------------------------static

#ifndef Z_WINDOWS
    extern int                  main_pid;               // Für pthreads (im Spooler). Bei Hostjava etc. kann das die pid des aktuellen Threads sein.
#endif

//Mutex                           zschimmer_mutex;

//---------------------------------------------------------------------------------z_malloc, z_free

void*                z_malloc                ( size_t size, const char* name, int lineno );         // Memory_allocator.cxx
void                 z_free                  ( void* p );                                           // Memory_allocator.cxx

//---------------------------------------------------------------------------------------Z_new_type

enum Z_new_type
{
    z_new_type
};

//---------------------------------------------------------------------------AddRef_Release_counter

struct AddRef_Release_counter
{
                                AddRef_Release_counter  ()                                          : _reference_count(1) {}
                                AddRef_Release_counter  ( const AddRef_Release_counter& )           : _reference_count(1) {}

    ULONG                       call_AddRef             ()                                          { return InterlockedIncrement( &_reference_count ); }
    ULONG                       call_Release            ();

  private:
    long32                     _reference_count;
};

//-------------------------------------------------------------------------------Has_addref_release

struct Has_addref_release
{
    virtual STDMETHODIMP_(ULONG) AddRef                     ()                                      = 0;
    virtual STDMETHODIMP_(ULONG) Release                    ()                                      = 0;
};

//-----------------------------------------------------------------simple_iunknown_implementation<>

template< class IUNKNOWN_INTERFACE >
struct simple_iunknown_implementation : IUNKNOWN_INTERFACE
{
    void*                       operator new            ( size_t size, Z_new_type, const char* name, int lineno )   { return z_malloc( size, name, lineno ); }
    void                        operator delete         ( void* p, Z_new_type, const char*, int )                   { z_free(p); }  // Für VC++ 6: Sonst warning C4291
    void                        operator delete         ( void* p )                                                 { z_free(p); }


    virtual                    ~simple_iunknown_implementation()                                                    {}

    simple_iunknown_implementation& operator=           ( const simple_iunknown_implementation& )                   { return *this; }


    STDMETHODIMP_( ULONG )  AddRef()
    { 
        return _addref_release_counter.call_AddRef(); 
    }


    STDMETHODIMP_( ULONG )  Release()
    { 
        ULONG reference_count = _addref_release_counter.call_Release();  
        
        if( reference_count == 0 )
        {
            delete this;  
        }

        return reference_count; 
    }


    STDMETHODIMP QueryInterface( const IID& iid, void** result )
    {
        *result = NULL;

        if( iid == IID_IUnknown )
        {
            *result = this;
            AddRef();
            return S_OK;
        }
        else
        {
            return E_NOINTERFACE;
        }
    }


  private:
    void*                       operator new            ( size_t );                                                 // gesperrt (Z_NEW verwenden!)
  //void                        operator delete         ( void* );                                                  // gesperrt (Z_NEW verwenden!)

    AddRef_Release_counter     _addref_release_counter;
};

//-------------------------------------------------------------------------------------------Object

struct Object : simple_iunknown_implementation<IUnknown>
/*
    Objekte dieses Typs können static oder automatic sein oder mit Z_NEW() angelegt werden.
    operator new() kann nicht verwendet werden.
*/
{
    // COM
    STDMETHODIMP                QueryInterface          ( const IID&, void** );

    virtual string              obj_name                () const                                    { return "(Object)"; }
    friend ostream&             operator <<             ( ostream& s, const Object& o )             { s << o.obj_name();  return s; }
};

//-------------------------------------------------------------------------------_No_AddRef_Release

template <class T>
struct _No_AddRef_Release : T
{
                                _No_AddRef_Release      ()                                          {}
  private:
    virtual STDMETHODIMP_(ULONG) AddRef                 ()                                          = 0;
    virtual STDMETHODIMP_(ULONG) Release                ()                                          = 0;
};

//--------------------------------------------------------------------------------------------ptr<>

template< class TYPE >
struct ptr
{
    // Ohne virtuelle Methode, damit _ptr möglichst am Anfang (offset 0) steht.


    enum                        New                     { new_object };                 // zur Polymorphie der Konstrukturen

                                ptr                     ()                              : _ptr(NULL) {}
                                ptr                     ( TYPE* p )                     { if(p) p->AddRef();  _ptr = p; }
                                ptr                     ( const ptr<TYPE>& src )        { TYPE* p = src._ptr; if(p) p->AddRef();  _ptr = p; }
    explicit                    ptr                     ( const CLSID& clsid )          : _ptr( NULL ) { create_instance( clsid ); }
    explicit                    ptr                     ( const string& class_name )    : _ptr( NULL ) { create_instance( class_name ); }
                               ~ptr                     ()                              { release(); }

#ifdef Z_HAS_MOVE_CONSTRUCTOR
                                ptr                     (ptr<TYPE>&& o)                 { _ptr = o._ptr, o._ptr = NULL; }
    ptr<TYPE>&                  operator =              (ptr<TYPE>&& o)                 { std::swap(_ptr, o._ptr); return *this; }
#endif

    void                        release                 ()                              { TYPE* p = _ptr; if(p) _ptr=NULL, p->Release(); }

    ptr<TYPE>&                  operator =              ( const ptr<TYPE>& src )        { assign( src._ptr );  return *this; }
    ptr<TYPE>&                  operator =              ( TYPE* src )                   { assign( src );       return *this; }

    bool                        operator !              () const                        { return _ptr == NULL; }
    TYPE&                       operator *              () const                        { return *(TYPE*)_ptr; }  
                                operator TYPE*          () const                        { return (TYPE*)_ptr;  }
    _No_AddRef_Release<TYPE>*   operator ->             () const                        { return (_No_AddRef_Release<TYPE>*)_ptr;  }
    TYPE*                       operator +              () const                        { return (TYPE*)_ptr;  }
    bool                        operator ==             ( TYPE* s ) const               { return _ptr == s; }


    TYPE**                      pp                      ()                              { release();  return (TYPE**)&_ptr; }
    void**                      void_pp                 ()                              { release();  return (void**)&_ptr; }   // gcc 4.1.1:  warning: dereferencing type-punned pointer will break strict-aliasing rules

    void                        assign                  ( TYPE* p )                     { if(p) p->AddRef(); release(); _ptr = p; }

    template< class INTERFACE_PTR >
    void                        assign_qi               ( const INTERFACE_PTR& q )      { TYPE* qi = NULL;  com::com_query_interface( q, &qi );  release();  _ptr = qi; }

    template< class INTERFACE_PTR >
    ptr<TYPE>&              try_assign_qi               ( const INTERFACE_PTR& q )      { TYPE* qi = NULL;  com::com_query_interface_or_null( q, &qi );  release();  _ptr = qi;  return *this; }

    template< class INTERFACE_PTR >
    HRESULT                     Assign_qi               ( const INTERFACE_PTR& of_interface, const IID& iid = __uuidof( TYPE ) )
                                                                                        { this->release(); return com::Com_query_interface( of_interface, &_ptr, iid ); }

    void                        copy_to                 ( TYPE** p ) const              { *p = copy(); }
    HRESULT                     CopyTo                  ( TYPE** p ) const              { copy_to(p); return S_OK; }
    TYPE*                       copy                    () const                        { TYPE* result = _ptr;  if(result) result->AddRef();  return result; }

    void                        move_to                 ( TYPE** p )                    { *p = _ptr;  _ptr = NULL; }
    TYPE*                       take                    ()                              { TYPE* result = _ptr;  _ptr = NULL;  return result; }

    HRESULT                     CoCreateInstance        ( const CLSID&   clsid  , IUnknown* outer = NULL, DWORD context = CLSCTX_ALL )  { release(); void* v = NULL; HRESULT hr = ::CoCreateInstance( clsid, outer, context, __uuidof(TYPE), &v ); _ptr = (TYPE*)v; return hr; }
    HRESULT                     CoCreateInstance        ( const OLECHAR* prog_id, IUnknown* outer = NULL, DWORD context = CLSCTX_ALL )  { release(); CLSID clsid; HRESULT hr = com::Name_to_clsid( prog_id, &clsid ); if(SUCCEEDED(hr)) hr = CoCreateInstance( clsid, outer, context ); return hr; }
    HRESULT                     CoCreateInstance        ( const char*    prog_id, IUnknown* outer = NULL, DWORD context = CLSCTX_ALL )  { release(); CLSID clsid; HRESULT hr = com::Name_to_clsid( prog_id, &clsid ); if(SUCCEEDED(hr)) hr = CoCreateInstance( clsid, outer, context ); return hr; }
    HRESULT                     CoCreateInstance        ( const string&  prog_id, IUnknown* outer = NULL, DWORD context = CLSCTX_ALL )  { return CoCreateInstance( prog_id.c_str(), outer, context ); }

    void                        create_instance         ( const CLSID& clsid )          { release();  _ptr = static_cast<TYPE*>( com::com_create_instance( clsid             , __uuidof( TYPE ) ) ); }
    void                        create_instance         ( const string& class_name )    { release();  _ptr = static_cast<TYPE*>( com::com_create_instance( class_name.c_str(), __uuidof( TYPE ) ) ); }
    
    template<class INTERFACE>
    void                        query_interface         ( INTERFACE** q ) const         { com::com_query_interface( _ptr, q ); }

  public:

    TYPE*                      _ptr;
};

//---------------------------------------------------------------------------------------hash_value

template< class TYPE >
inline size_t                   hash_value              ( const ptr<TYPE>& p )          { return (size_t)+p; }


#ifdef __GNUC__
    } //namespace zschimmer  

    namespace __gnu_cxx
    {                  
        template< class CLASS >
        struct hash< zschimmer::ptr<CLASS> >
        {                                                                                       
            size_t operator()( const zschimmer::ptr<CLASS>& pointer ) const                           
            {                                                                                   
                return zschimmer::hash_value( pointer );
            }                                                                                   
        };                                                                                      
    }

    namespace zschimmer {
#endif

//--------------------------------------------------------------------------------------------z_new

template< class TYPE  >
inline ptr<TYPE> z_new( TYPE* p )
{
    ptr<TYPE> result;
    result._ptr = p;
    return result;
}

//--------------------------------------------------------------------------------------------Z_NEW
// Nur für ptr<TYPE> var = Z_NEW( TYPE )

#ifdef __GNUC__

#   define Z_NEW( CONSTRUCTOR_CALL )  \
        zschimmer::z_new( new( zschimmer::z_new_type, #CONSTRUCTOR_CALL, __LINE__ ) CONSTRUCTOR_CALL )

#else

#   define Z_NEW( CONSTRUCTOR_CALL )  \
        zschimmer::z_new( new( zschimmer::z_new_type, #CONSTRUCTOR_CALL##" "##__FILE__, __LINE__ ) CONSTRUCTOR_CALL )

#endif

//-----------------------------------------------------------------------------------------qi_ptr<>
// ptr<> mit Typprüfung mit QueryInterface()

template< class TYPE >
struct qi_ptr : ptr<TYPE>
{
                                qi_ptr                  ()                                          {}
                                qi_ptr                  ( TYPE* p )                                 : ptr<TYPE>( p ) {}
                                qi_ptr                  ( const qi_ptr<TYPE>& p )                   : ptr<TYPE>( p ) {}
                                qi_ptr                  ( const ptr<TYPE>& p )                      : ptr<TYPE>( p ) {}
                                qi_ptr                  ( IUnknown* p )                             { this->assign_qi(p); }

    ptr<TYPE>&                  operator =              ( TYPE* src )                               { this->assign   ( src      );  return *this; }
    ptr<TYPE>&                  operator =              ( const qi_ptr<TYPE>& src )                 { this->assign   ( src._ptr );  return *this; }
    ptr<TYPE>&                  operator =              ( const ptr<TYPE>& src )                    { this->assign   ( src._ptr );  return *this; }
    ptr<TYPE>&                  operator =              ( const IUnknown* src )                     { this->assign_qi( src      );  return *this; }
};

//-------------------------------------------------------------------------------------try_qi_ptr<>

template< class TYPE >
struct try_qi_ptr : qi_ptr<TYPE>
{
                                try_qi_ptr              ()                                          {}
                                try_qi_ptr              ( TYPE* p )                                 : qi_ptr<TYPE>( p ) {}
                                try_qi_ptr              ( const try_qi_ptr<TYPE>& p )               : qi_ptr<TYPE>( p ) {}
                                try_qi_ptr              ( const ptr<TYPE>& p )                      : qi_ptr<TYPE>( p ) {}
                                try_qi_ptr              ( IUnknown* p )                             { this->try_assign_qi( p ); }


    ptr<TYPE>&                  operator =              ( TYPE* src )                               { this->assign( src      );  return *this; }
    ptr<TYPE>&                  operator =              ( const try_qi_ptr<TYPE>& src )             { this->assign( src._ptr );  return *this; }
    ptr<TYPE>&                  operator =              ( const ptr<TYPE>& src )                    { this->assign( src._ptr );  return *this; }
    ptr<TYPE>&                  operator =              ( const IUnknown* src )                     { this->try_assign_qi( src );  return *this; }
};

//------------------------------------------------------------------------------------String_object

struct String_object : Object
{
    explicit                    String_object           ( const string& str )                       : _string(str) {}

    string                      obj_name                () const                                    { return _string; }

    string                     _string;
};

//---------------------------------------------------------------------------------Zschimmer_static

struct Zschimmer_static
{
                                Zschimmer_static            ();
                               ~Zschimmer_static            (); 

    void                        close();


    bool                       _valid;
};

//--------------------------------------------------------------------------------------Incrementor

template< class COUNTER_TYPE >
struct Incrementor
{
                                Incrementor                 ( COUNTER_TYPE* p )                     : _ptr ( p ) { (*p)++; }
    virtual                    ~Incrementor                 ()                                      { if( _ptr )  (*_ptr)--; }

    void                        release                     ()                                      { if( _ptr )  (*_ptr)--, _ptr = NULL; }

  private:
    COUNTER_TYPE*              _ptr;
};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

//----------------------------------------------------------------------------------------------new

//Reihenfolgeproblem bei Initialisierung statischer Variablen:
//inline void*                    operator new                (size_t size)                           { return zschimmer::z_malloc(size,"new",0); }
//inline void                     operator delete             (void* p)                               { zschimmer::z_free(p); }
//inline void*                    operator new[]              (size_t size)                           { return zschimmer::z_malloc(size,"new[]",0); }
//inline void                     operator delete[]           (void* p)                               { zschimmer::z_free(p); }

//-------------------------------------------------------------------------------------------------

#endif
