// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com 

#ifndef __SCHEDULER_REGISTER_H
#define __SCHEDULER_REGISTER_H

namespace sos {
namespace scheduler {


struct Register;
template< class REGISTERED_, class IREGISTERED > struct reg;

//---------------------------------------------------------------------------------------Registered

struct Registered : IUnknown,
                    Scheduler_object, 
                    Non_cloneable
{
                                Registered                  ( Register*, IUnknown*, Scheduler_object::Type_code, const string& name = "" );
                               ~Registered                  ();

    virtual void                prepare_to_remove           ( File_based::Remove_flags )           = 0;

    virtual void            set_name                        ( const string& );
    string                      name                        () const                                { return _name; }
    string                      path                        () const                                { return _name; }
    bool                        is_registered               () const;
    virtual void                remove                      ();


    // Scheduler_object
    string                      obj_name                    () const;


    STDMETHODIMP            put_Name                        ( BSTR );     
    STDMETHODIMP            get_Name                        ( BSTR* result )                        { return String_to_bstr( _name, result ); }
    STDMETHODIMP                Remove                      ();

  private:
    Fill_zero                  _zero_;
    string                     _name;
    Register*                  _register;
};

//-------------------------------------------------------------------------------------registered<>
/*
template< class REGISTERED_, class IREGISTERED >
struct registered : Registered
{
                                registered                  ( reg<REGISTERED_,IREGISTERED>* r, const string& name = "" ) : Registered( r, name ) {}
                               ~registered                  ()                                      {}
};

//-----------------------------------------------------------------------------------------Register

struct Register
{
                                Register                    ( Scheduler*, const string& registered_type_name );

    void                        close                       ();
    virtual ptr<Registered>     new_registered_             ()                                      = 0;    //{ return Z_NEW( REGISTERED_( this ); }

  protected:
    friend struct               Registered;

    Registered*                 registered_                 ( const string& name );

    STDMETHODIMP            get_Registered_                 ( BSTR, Registered** );
    STDMETHODIMP            get_Registered_or_null_         ( BSTR, Registered** );
    STDMETHODIMP                Create_registered_          ( Registered** );
    STDMETHODIMP                Add_registered_             ( Registered* );

  private:
    Fill_zero                  _zero_;
    string                     _registered_type_name;

  protected:
    Scheduler*                 _spooler;
};

//--------------------------------------------------------------------------------------------reg<>

template< class REGISTERED_, class IREGISTERED >
struct reg : Register
{
                                reg                         ( Scheduler* scheduler, const string& registered_type_name ) : _zero_(this+1), Register( scheduler, registered_type_name ) {}

    bool                        is_empty                    () const                                { return _registered_map.empty(); }
    REGISTERED_*                registered                  ( const string& name )                  { return static_cast<REGISTERED_*>( registered_( name ) ); }
    virtual ptr<Registered>     new_registered_             ()                                      { ptr<Registered> p = +new_registered(); return p; }
    virtual ptr<REGISTERED_>    new_registered              ()                                      = 0;


    void close()
    {
        for( Registered_map::iterator it = _registered_map.begin(); it != _registered_map.end(); )
        {
            Registered_map::iterator erase_it = it;
            it++;

            erase_it->second->close();
            _registered_map.erase( erase_it );
        }
    }


    REGISTERED_* registered_or_null( const string& name )
    {
        Registered_map::iterator it = _registered_map.find( name );
        return it == _registered_map.end()? NULL : it->second;
    }


    void add_registered( REGISTERED_* registered )
    {
        if( !registered )  z::throw_xc( Z_FUNCTION );

        if( registered->is_registered() )  z::throw_xc( "SCHEDULER-416", registered->obj_name() );
        _spooler->check_name( registered->name() );

        _registered_map[ registered->name() ] = registered;
    }


    void remove_registered( REGISTERED_* registered )
    {
        string        path = registered->path();
        Registered_map::iterator it   = _registered_map.find( path );

        if( it->second != registered )  z::throw_xc( "SCHEDULER-418", registered->obj_name() );

        registered->prepare_to_remove( Remove_flags );
        _registered_map.erase( it );
    }


    STDMETHODIMP            get_Registered                  ( BSTR name, IREGISTERED** r )          { return get_Registered_( name, static_cast<IUnknown**>( r ) ); }
    STDMETHODIMP            get_Registered_or_null          ( BSTR name, IREGISTERED** r )          { return get_Registered_or_null_( name, static_cast<IUnknown**>( r ) ); }
    STDMETHODIMP                Create_registered           ( IREGISTERED** r )                     { return Create_registered_( static_cast<IUnknown**>( r ) ); }
    STDMETHODIMP                Add_registered              ( IREGISTERED* r )                      { return Add_registered_( static_cast<IUnknown*>( r ) ); }


  private:
    Fill_zero                  _zero_;

  public:
    typedef map< string, ptr<REGISTERED_> > Registered_map;
    Registered_map                         _registered_map;
};
*/
//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
