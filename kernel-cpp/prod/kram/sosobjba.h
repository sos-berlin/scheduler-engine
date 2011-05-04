// sosobjba.h                       © 1995 SOS GmbH Berlin


#ifndef __SOSOBJBA_H
#define __SOSOBJBA_H

#include "../kram/sostypcd.h"


namespace sos
{

#define DONT_LOG_NEW

#if defined SOS_INLINE /*defined _MSC_VER */
#   define SOSOBJBA_INLINE
#endif

//------------------------------------------------------------------------------Sos_object_base

struct SOS_CLASS Sos_object_base
{
    friend ::std::ostream&      operator <<             ( ::std::ostream&, const Sos_object_base& );
    void                        obj_print               ( ::std::ostream* s ) const { _obj_print( s ); }
    Bool                        obj_is_type             ( Sos_type_code t ) const   { return _obj_is_type( t ); }
    Sos_object_base*            obj_cast                ( Sos_type_code, const char* type_name, const char* debug_text, int debug_lineno = 0 );
    string                      obj_name                () const                    { return _obj_name(); }

  protected:
    virtual string             _obj_name                () const;
    virtual void               _obj_print               ( ::std::ostream* s ) const;
    virtual Bool               _obj_is_type             ( Sos_type_code t ) const   { return t == tc_Sos_object_base; }
};

//-----------------------------------------------------------------------------DEFINE_OBJ_PRINT
#if defined SYSTEM_RTTI
#   define DEFINE_OBJ_PRINT( TYPE )
# else
#   define DEFINE_OBJ_PRINT( TYPE )  \
       virtual void _obj_print( ::std::ostream* s ) const  { *s << #TYPE; }
#endif
//------------------------------------------------------------------------------DEFINE_OBJ_COPY

#define DEFINE_OBJ_COPY( TYPE )                                                             \
                                                                                            \
   virtual Sos_self_deleting* _obj_copy() const                                             \
   {                                                                                        \
       return new( _obj_new_type_, "" ) TYPE( *this );                                      \
   }

//---------------------------------------------------------------------------DEFINE_OBJ_IS_TYPE

#define DEFINE_OBJ_IS_TYPE( TYPE )                                                          \
   Bool _obj_is_type( Sos_type_code t ) const                                               \
   {                                                                                        \
       return t == tc_##TYPE || Base_class::_obj_is_type( t );                              \
   }

//------------------------------------------------------------------------------------SOS_CAST2

#define SOS_CAST2( TYPE, TYPE_NAME_STRING, PTR ) \
     ( (TYPE*)(PTR)->obj_cast( tc_##TYPE, TYPE_NAME_STRING, MODULE_NAME ", " __FILE__, __LINE__ ) )

//-------------------------------------------------------------------------------------SOS_CAST

#define SOS_CAST( TYPE, PTR )  SOS_CAST2( TYPE, #TYPE, PTR )

//----------------------------------------------------------------------------Sos_self_deleting

struct Sos_self_deleting : Sos_object_base
/*
    Objekte dieses Typs können static oder automatic sein oder mit SOS_NEW()
    angelegt werden.
    operator new() kann nicht verwendet werden.
*/
{
    BASE_CLASS( Sos_object_base )
    DEFINE_OBJ_COPY( Sos_self_deleting )
    DEFINE_OBJ_PRINT( Sos_self_deleting )

    enum                        New_type { _obj_new_type_ };            // zur Polymorphie

                                Sos_self_deleting       ();
                                Sos_self_deleting       ( const Sos_self_deleting& );
    virtual                    ~Sos_self_deleting       ();

    Sos_self_deleting&          operator =              ( const Sos_self_deleting& )            { return *this; }

    void                        obj_const_name          ( const char* const_name )              { _obj_const_name = const_name; }
    void                        obj_add_ref             () const/*mutable*/                     { InterlockedIncrement( &((Sos_self_deleting*)this)->_obj_ref_count ); }
    void                        obj_remove_ref          () const/*mutable*//*???*/              { if( InterlockedDecrement( &((Sos_self_deleting*)this)->_obj_ref_count ) == 0 )  ((Sos_self_deleting*)this)->obj_del(); }
    uint                        obj_ref_count           () const                                { return _obj_ref_count; }

    // Nur für SOS_NEW(), sos_new_ptr():
    void*                       operator new            ( size_t size, const enum New_type, const char* info = "" )      { return operator new( size, info ); }
    void                        operator delete         ( void* );                              // Besser mit DELETE löschen!!
#   ifdef SYSTEM_DELETE_WITH_PARAMS
        void                    operator delete         ( void*, const enum New_type, const char* );  // Für VC++ 6: Sonst warning C4291
#   endif

    void*                       operator new            ( size_t, void* p )                     { return p; }
#   ifdef SYSTEM_DELETE_WITH_PARAMS
        void                    operator delete         ( void*, void* )                        {}						// VC++ 6: C4291
#   endif

    void                        obj_new                 ()                                      { InterlockedDecrement( &_obj_ref_count ); }


  protected:
    Bool                       _obj_is_type             ( Sos_type_code t ) const               { return t == tc_Sos_self_deleting || Base_class::_obj_is_type( t ); }

  private:
    void*                       operator new            ( size_t size, const char* info = "" );
    void                        obj_del                 ();

    zschimmer::long32          _obj_ref_count;
    const char*                _obj_const_name;         // Zeiger auf Konstante, wird nicht freigegeben!
};

//----------------------------------------------------------------------------------Sos_pointer

struct Sos_pointer
{
                                Sos_pointer             ()                          : _ptr ( 0 )   {}
                              //Sos_pointer             ( Sos_self_deleting* ptr );
                                Sos_pointer             ( const Sos_self_deleting* ptr );
                                Sos_pointer             ( const Sos_pointer& src );//  { _ptr = copy( src._ptr ); }
#   if !defined _MSC_VER  &&  !defined SYSTEM_BORLAND
        virtual                 // Damit der Destruktor bei einer Exception aufgerufen wird
#   endif
                               ~Sos_pointer             ();

    int                         operator !              () const                    { return _ptr == 0; }
    void                        operator =              ( const Sos_pointer& src )  { _assign( src._ptr ); }
    void                        operator =              ( Sos_self_deleting* src )  { _assign( src ); }


    Sos_self_deleting&          operator *              () const        { return *_ptr; }
    Sos_self_deleting*          operator +              () const        { return _ptr;  }
    Sos_self_deleting*          operator ->             () const        { return _ptr;  }

    void                        del                     ();

  protected:
                              //Sos_pointer             ( Sos_self_deleting* ptr, Sos_self_deleting::New_type );
                                Sos_pointer             ( const Sos_self_deleting* ptr, Sos_self_deleting::New_type );
    inline void                _assign                  ( Sos_self_deleting* );
    void                      __assign                  ( Sos_self_deleting* );
    Sos_self_deleting*         _ptr;

  private:
    void                        inline_del              ();
    static Sos_self_deleting*   copy                    ( Sos_self_deleting* p )               { if( p ) p->obj_add_ref(); return p; }
};

//-------------------------------------------------------------------------Sos_static_ptr<TYPE>

template< class TYPE >
struct Sos_static_ptr : Sos_pointer
{
                                Sos_static_ptr          ()              {}
                                Sos_static_ptr          ( TYPE* ptr )   { Sos_pointer p; p = sos_self_deleting_ptr( ptr ); *(Sos_pointer*)this = +p; }
                                Sos_static_ptr          ( const Sos_static_ptr<TYPE>& src ) : Sos_pointer( sos_self_deleting_ptr( src ) ) {}

#if defined SYSTEM_SOLARIS  /* anyfile.cxx: Error: Could not find source for Sos_static_ptr<Record_type>::operator=(Record_type*). */
                            // Generiert aber bei Borland viel Code
    void                        operator =              ( TYPE* src )                           { _assign( src ); }
    void                        operator =              ( const Sos_static_ptr<TYPE>& src )     { _assign( src._ptr ); }
#else
    void                        operator =              ( TYPE* src );//                 { _assign( src ); }
    void                        operator =              ( const Sos_static_ptr<TYPE>& src );//                 { _assign( src._ptr ); }
#endif

    TYPE&                       operator *              () const        { return *ptr();    }
                                operator TYPE*          () const        { return ptr();     }
                              //operator TYPE&          () const        { return *ptr();    }
    TYPE*                       operator ->             () const        { return ptr();     }
    TYPE*                       operator +              () const        { return ptr();     }

  protected:
                                Sos_static_ptr          ( TYPE* ptr, Sos_self_deleting::New_type dummy ) : Sos_pointer( sos_self_deleting_ptr( ptr ), dummy ) {}
    TYPE*                       ptr                     () const        { return (TYPE*)_ptr; }

  private:
#   if defined SYSTEM_MICROSOFT || defined SYSTEM_GNU
        static
#   else
        friend
#   endif
    Sos_self_deleting*          sos_self_deleting_ptr   ( TYPE* );      // nicht inline! muß für jeden Typ mit DEFINE_SOS_STATIC_PTR( TYPE ) implementiert werden
};

//----------------------------------------------------------------------------DEFINE_SOS_STATIC
// Muß in einem Modul für jeden Typ gegeben werden:

#if defined SYSTEM_SOLARIS
#   define DEFINE_SOS_STATIC_PTR_ASSIGNMENT( TYPE )
#else
#   define DEFINE_SOS_STATIC_PTR_ASSIGNMENT( TYPE )                                         \
                                                                                            \
        template<>                                                                          \
        void Sos_static_ptr<TYPE>::operator= ( TYPE* src )                                  \
        {                                                                                   \
            _assign( src );                                                                 \
        }                                                                                   \
                                                                                            \
        template<>                                                                          \
        void Sos_static_ptr<TYPE>::operator= ( const Sos_static_ptr<TYPE>& src )            \
        {                                                                                   \
            _assign( src._ptr );                                                            \
        }                                                                                   
#endif

#if defined SYSTEM_MICROSOFT  ||  defined SYSTEM_GNU
    #define DEFINE_SOS_STATIC_PTR( TYPE )                                                   \
                                                                                            \
        template<>                                                                          \
        Sos_self_deleting* Sos_static_ptr<TYPE>::sos_self_deleting_ptr( TYPE* ptr )         \
        {                                                                                   \
            return ptr;                                                                     \
        }                                                                                   \
                                                                                            \
        DEFINE_SOS_STATIC_PTR_ASSIGNMENT( TYPE )

#else
    #define DEFINE_SOS_STATIC_PTR( TYPE )                                                   \
                                                                                            \
        template<>                                                                          \
        Sos_self_deleting* sos_self_deleting_ptr( TYPE* ptr )                               \
        {                                                                                   \
            return ptr;                                                                     \
        }                                                                                   \
                                                                                            \
        DEFINE_SOS_STATIC_PTR_ASSIGNMENT( TYPE )

#endif

//--------------------------------------------------------------------------------Sos_ptr<TYPE>

template< class TYPE >
struct Sos_ptr : Sos_pointer
{
                                Sos_ptr                 ()              {}
                                Sos_ptr                 ( TYPE* ptr )   : Sos_pointer( ptr ) {}
                                Sos_ptr                 ( const Sos_ptr<TYPE>& src ) : Sos_pointer( (const TYPE*)src._ptr ) {}
                                Sos_ptr                 ( const Sos_static_ptr<TYPE>& src ) : Sos_pointer( (const TYPE*)+src ) {}

    void                        operator =              ( const Sos_ptr<TYPE>& src )  { _assign( src._ptr ); }
    void                        operator =              ( TYPE* src )                 { _assign( src ); }

    TYPE&                       operator *              () const        { return *(TYPE*)_ptr; }
                                operator TYPE*          () const        { return (TYPE*)_ptr;  }
                                operator Sos_static_ptr<TYPE>() const   { return (TYPE*)_ptr;  }
    TYPE*                       operator ->             () const        { return (TYPE*)_ptr;  }
    TYPE*                       operator +              () const        { return (TYPE*)_ptr;  }

#if 1//VC2003 defined SYSTEM_GNU
  //protected:
                                Sos_ptr                 ( TYPE* p, Sos_self_deleting::New_type dummy ) : Sos_pointer( p, dummy ) {}
    static inline Sos_ptr<TYPE> sos_new_ptr             ( TYPE* );
    static inline Sos_ptr<TYPE> obj_copy                ( const TYPE& o )  { return Sos_ptr<TYPE>( (TYPE*)o._obj_copy(), Sos_self_deleting::New_type(0) ); }

#else
  protected:
                                Sos_ptr                 ( TYPE* ptr, Sos_self_deleting::New_type dummy ) : Sos_pointer( ptr, dummy ) {}
    friend inline Sos_ptr<TYPE> sos_new_ptr             ( TYPE* );
    friend inline Sos_ptr<TYPE> obj_copy                ( const TYPE& o )  { return Sos_ptr<TYPE>( (TYPE*)o._obj_copy(), Sos_self_deleting::New_type(0) ); }
#endif
};

//-------------------------------------------------------------------------------------OBJ_COPY

#if 1//VC2003 defined SYSTEM_GNU
#   define OBJ_COPY( TYPE, OBJ )  ( Sos_ptr<TYPE>( (TYPE*)(OBJ)._obj_copy(), Sos_self_deleting::New_type(0) ) )
#else
#   define OBJ_COPY( TYPE, OBJ )  obj_copy( OBJ )
#endif

//----------------------------------------------------------------------------------sos_new_ptr

template< class TYPE > inline Sos_ptr<TYPE> sos_new_ptr( TYPE* ptr );

//----------------------------------------------------------------------------------SOS_LOG_NEW

#if defined DONT_LOG_NEW
#   define SOS_LOG_NEW( CONSTRUCTOR_CALL )  ((void)0)
# else
#   define SOS_LOG_NEW( CONSTRUCTOR_CALL )  sos_log( MODULE_NAME ": new " CONSTRUCTOR_CALL "\n" )
#endif

//--------------------------------------------------------------------------------------SOS_NEW

#define SOS_NEW( CONSTRUCTOR_CALL )  \
    sos_new_ptr( ( SOS_LOG_NEW(  #CONSTRUCTOR_CALL ), \
                   new( Sos_self_deleting::_obj_new_type_, #CONSTRUCTOR_CALL ) CONSTRUCTOR_CALL ) )

#define SOS_NEW_PTR(X)  SOS_NEW(X)

//-------------------------------------------------------------------------------TYPED_AUTO_PTR

#define TYPED_AUTO_PTR( TYPE, POINTER_VAR ) \
    if( !POINTER_VAR ) {  \
        Sos_ptr<TYPE> p ( SOS_NEW( TYPE ) ); \
        POINTER_VAR = p; \
    } else {}

//----------------------------------------------------------------------------------Sos_new_ptr

template< class TYPE >
struct Sos_new_ptr : Sos_ptr<TYPE>
{
                                Sos_new_ptr();
};

//-----------------------------------------------------------Sos_auto_new_ptr<TYPE,CREATE_FUNC>
/*
    Legt bei Benutzung, außer operator!(), das Objekt mit seinem Default-Konstruktur an.
*/

//#ifndef SYSTEM_GNU
//
//typedef Sos_self_deleting* Sos_self_deleting_ptr_;
//
//template< class TYPE, Sos_self_deleting_ptr_ (*CREATE_FUNC)() >
//struct Sos_auto_new_ptr : Sos_ptr<TYPE>
//{
//                                operator TYPE*          () const        { return ptr();     }
//                                operator TYPE&          () const        { return *ptr();    }
//    TYPE&                       operator *              ()              { return *ptr();    }
//    TYPE*                       operator ->             ()              { return ptr();     }
//
//  private:
//    TYPE*&                      ptr                     ()              { return  _ptr? _ptr : ( *this = (TYPE*)(*CREATE_FUNC)(), _ptr ) ); }
//};
//
//#endif

//---------------------------------------------------------------------------------auto_new_ptr

template< class TYPE > inline TYPE* auto_new_ptr( Sos_ptr<TYPE>* ptr_ptr );

//-----------------------------------------------------------------------------------sos_delete
// Für DELETE(), s. sos.h

inline void sos_delete( Sos_pointer& ptr )
{
    ptr.del();
}

template< class TYPE >
inline void sos_delete( Sos_ptr<TYPE>& ptr )
{
    ptr.del();
}

//---------------------------------------------------------------------------------AUTO_NEW_PTR

//#define AUTO_NEW_PTR( TYPE, PTRVAR )  ( PTRVAR? PTRVAR : PTRVAR = new TYPE )

//---------------------------------------------------------------------------------auto_new_ptr

template< class TYPE >
inline TYPE*& auto_new_ptr( TYPE** ptr_ptr )
{
    if( !*ptr_ptr )  *ptr_ptr = new TYPE();
    return *ptr_ptr;
}


//------------------------------------------------------------------------------------_obj_copy
//???
Sos_pointer _obj_copy( const Sos_self_deleting& );

//=======================================================================================INLINE

//---------------------------------------------------------------------Sos_pointer::Sos_pointer

inline Sos_pointer::Sos_pointer( const Sos_self_deleting* ptr, Sos_self_deleting::New_type )
{
    // nur für sos_new_ptr()
    //assert( ptr && ptr->obj_ref_count() == 1 );
    _ptr = (Sos_self_deleting*)ptr;
}

//-------------------------------------------------------------------------Sos_pointer::_assign

inline void Sos_pointer::_assign( Sos_self_deleting* src )
{
#   if defined SOSOBJBA_INLINE  ||  defined SOS_INLINE
        if( _ptr )  __assign( src );
              else  _ptr = copy( src );
#   else
        __assign( src );
#   endif
}

//----------------------------------------------------------------------Sos_pointer::inline_del

inline void Sos_pointer::inline_del()
{
    Sos_self_deleting* p = _ptr;
    
    if( p ) 
    {
        _ptr = 0;
        p->obj_remove_ref();
    }
}

//--------------------------------------------------------------------Sos_pointer::~Sos_pointer

inline Sos_pointer::~Sos_pointer()
{
#   if defined SOSOBJBA_INLINE
        inline_del();
#    else
        del();
#   endif
}

//---------------------------------------------------------------------------------auto_new_ptr

template< class AUTO_NEW_PTR_TYPE >
inline AUTO_NEW_PTR_TYPE* auto_new_ptr( Sos_ptr<AUTO_NEW_PTR_TYPE>* ptr_ptr )
{
    if( !*ptr_ptr )  *ptr_ptr = SOS_NEW( AUTO_NEW_PTR_TYPE );
    return *ptr_ptr;
}

//----------------------------------------------------------------------------------sos_new_ptr

template< class TYPE >
inline Sos_ptr<TYPE> sos_new_ptr( TYPE* ptr )
{
    return Sos_ptr<TYPE>( ptr, Sos_self_deleting::New_type(0) );
}

//---------------------------------------------------------------Sos_new_ptr<TYPE>::Sos_new_ptr

template< class Sos_new_ptr_TYPE >      // "Sos_new_ptr_TYPE" wird so mit LOG protokolliert
inline Sos_new_ptr<Sos_new_ptr_TYPE>::Sos_new_ptr()
{
    this->_ptr = SOS_NEW( Sos_new_ptr_TYPE );
}

//---------------------------------------------------------------------------------auto_new_ptr

template< class TYPE >
inline TYPE* auto_new_ptr( Sos_static_ptr<TYPE>* ptr_ptr )
{
    if( !*ptr_ptr ) {
        Sos_pointer p ( SOS_NEW( TYPE ) );
        *ptr_ptr = (TYPE*)+p;
    }
    return *ptr_ptr;
}


} //namespace sos

#endif
