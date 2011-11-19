// $Id: reference.h 13199 2007-12-06 14:15:42Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __ZSCHIMMER_REFERENCE_H
#define __ZSCHIMMER_REFERENCE_H


#ifdef REFERER
#   undef REFERER   // sos.h
#endif


namespace zschimmer {

//-----------------------------------------------------------------------------------assert_pointer

void*                           assert_pointer              ( void* );

template< typename TYPE >
TYPE*                           assert_pointer              ( TYPE* p )                             { return static_cast<TYPE*>( assert_pointer( (void*)p ) ); }

//-----------------------------------------------------------------------------------Reference_base
    
//struct Reference_base
//{
//    virtual                    ~Reference_base              ()                                      {}
//    virtual void                on_releasing_referenced_object()                                    = 0;
//};

//--------------------------------------------------------------------------------------reference<>

template< class REFERER, class TO >
struct reference 
{
    reference( REFERER* s ) 
    : 
        _referer( s ), 
        _ptr(NULL) 
    {
    }
    
    
                               ~reference               ()                              { assign( NULL ); }

    reference&                  operator =              ( TO* ptr )                     { assign( ptr );  return *this; }
    bool                        operator !              () const                        { return _ptr == NULL; }
                                operator TO*            () const                        { return _ptr; }
    TO*                         operator +              () const                        { return (TO*)_ptr;  }
    TO&                         operator *              () const                        { return valid_ptr(); }  
    TO*                         operator ->             () const                        { return valid_ptr(); }
    bool                        operator ==             ( TO* s ) const                 { return _ptr == s; }

    TO*                         valid_ptr               () const                        { return assert_pointer( _ptr ); }
    REFERER*                    referer                 () const                        { return _referer; }


    void assign( TO* ptr )
    {
        if( _ptr )  _ptr->unregister_reference( this );
        _ptr = ptr;
        if( _ptr )  _ptr->register_reference( this );
    }

    
    void on_releasing_referenced_object()
    {
        _referer->on_releasing_referenced_object( *this );
        _ptr = NULL;
    }


  private:
    TO*                        _ptr;
    REFERER*                   _referer;
};

//-------------------------------------------------------------------------------------------------
/*
struct Is_referenced
{
    virtual                    ~Is_referenced               ();

    void                        register_reference          ( Reference_base* referer );
    void                        unregister_reference        ( Reference_base* referer );
    void                        release_all_references      ();
    bool                        is_referenced_by            ( Reference_base* referer )              { return _reference_register.find( referer ) != _reference_register.end(); }
    bool                        is_referenced               ()                                      { return !_reference_register.empty(); }
};
*/
//-------------------------------------------------------------------------------is_referenced_by<>

template< class REFERER, class TO >
struct is_referenced_by
{
    typedef reference<REFERER,TO> Reference;


    ~is_referenced_by()
    {
        release_all_references();
    }


    void register_reference( Reference* reference )
    {
        Z_DEBUG_ONLY( assert( _reference_register.find( reference ) == _reference_register.end() ) );
        _reference_register.insert( reference );
    }


    void unregister_reference( Reference* reference )
    {
        Z_DEBUG_ONLY( assert( _reference_register.find( reference ) != _reference_register.end() ) );
        _reference_register.erase( reference );
    }


    void release_all_references()
    {
        for( typename Reference_register::iterator it = _reference_register.begin(); it != _reference_register.end(); )
        {
            typename Reference_register::iterator next_it = it;
            next_it++;

            Reference* reference = *it;
            _reference_register.erase( it );
            reference->on_releasing_referenced_object();

            it = next_it;
        }
    }


    bool is_referenced( REFERER* = NULL )
    { 
        //assert( referer_null_pointer == NULL );
        return !_reference_register.empty(); 
    }
    

    string string_referenced_by() const
    {
        S result;

        Z_FOR_EACH_CONST( typename Reference_register, _reference_register, it )
        {
            Reference* reference = *it;
            if( !result.empty() )  result << ", ";
            result << reference->referer()->obj_name();
        }

        return result;
    }


    typedef stdext::hash_set< Reference* > Reference_register;
    Reference_register                    _reference_register;
};

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer


#endif
