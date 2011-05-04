// $Id$

#include "zschimmer.h"
#include "log_categories.h"
#include "log.h"

using namespace std;


namespace zschimmer {

//-----------------------------------------------------------------------------------Log_categories
// Dieser Code ist ganz unverständlich, weil er direkt auf der Hash-Map arbeitet.
// Ein besserer Algorithmus wäre ein Baum, aus dem nach jeder Änderung die Hash-Map generiert wird.
// Der Baum dient zum Verwalten der durch Punkte getrennten Pfade.
// Die Hash-Map ist daraus gebildet und dient zum schnellen Lesen mit is_set().
// Dann brauchen wir die "derived"-Einträge nicht mehr, der Algorithmus wäre klarer.


/*

    Beispiel der Verwendung: (veraltet)

    Der Benutzer stellt (z.B. in einer .ini-Datei) ein:
    
        log_what = misc.profile misc.java.exceptions objectserver.* objectserver.recv-
    

    Das führt zu den Aufrufen:

        set( "misc.profile"         , true );
        set( "misc.java.exception"  , true );
        set( "objectserver.*"       , true );
        set( "objectserver.recv"    , false );


    Das ergibt die _map:

        "misc.profile"         ._value = true
        "misc.java.exception"  ._value = true 
        "objectserver.*"       ._value = true 
        "objectserver.recv"    ._value = false


    Das Programm setzt Voreinstellungen (vor oder nach den set()-Aufrufen):

        set_default( "misc.profile.get"   , true );

        set_default( "misc.java.call"     , true );
        set_default( "misc.java.exception", true );
        // misc.java => misc.java.call misc.java.exception

      //set_default( "objectserver.send"    , false );
      //set_default( "objectserver.recv"    , false );
        set_default( "objectserver.call"    , true );
        set_default( "objectserver.callback", true );
        // objectserver => objectserver.call objectserver.callback


    Das ergibt die _default_map:

        "misc.profile.get"   , true );
      //"misc.profile.include, false );        // include ausnehmen

        "misc.java.call"     , true );
        "misc.java.exception", true );
        // misc.java => misc.java.call misc.java.exception

      //"objectserver.send"    , false );
      //"objectserver.recv"    , false );
        "objectserver.call"     ._default_value, true );
        "objectserver.callback" ._default_value =  true );


    misc.profile.get
    misc.profile.include
    misc.profile.include misc.profile.get-
    misc.com

    log.programname
    log.virtual_size

    scheduler.wait
    scheduler.sockets

    java.calls
    java.stacktrace
    java.options

    objectserver.call

    -------------------

    static_log_categories.add( "misc.profile !log.programmname java.calls" );
    
    static_log_categories.set_default( "misc.profile.get" );


    if( static_log_categories( "misc.profile.include" ) ) ...;

*/

//-----------------------------------------------------------------Log_categories::Entry::type_name

string Log_categories::Entry::type_name( Type t )
{
    string result;

    switch( t )
    {
        case e_standard:    result = "standard";  break;
        case e_implicit:    result = "implicit";  break;
        case e_explicit:    result = "explicit";  break;
        case e_derived:     result = "derived";   break;
        default:            result = as_string(t);
    }

    return result;
}

//------------------------------------------------------------------------Log_categories::self_test

void Log_categories::self_test()
{
    Log_categories cat;

    Z_LOG2( "zschimmer", Z_FUNCTION << "() " << cat.debug_string() << "\n" );

    Log_categories_content safe;
    cat.save_to( &safe );

    //for( int h = 1; h <= 2; h++ )
    {
        cat.set_default( "default"     , true  );
        cat.set_default( "a.default"   , true  );
        cat.set        ( "a.implied"   , true , Entry::e_implicit );
        cat.set        ( "b.implied"   , true , Entry::e_implicit );
        cat.set        ( "b.suppressed", false, Entry::e_explicit );
        cat.set_default( "c.default"   , true  );
        cat.set        ( "suppressed"  , false, Entry::e_explicit );

        cat.set( "a" );
        cat.set( "b.*" );
        cat.set( "d" );
        
        for( int i = 1; i <= 2; i++ )
        {
            Z_LOG2( "zschimmer", Z_FUNCTION << "() " << cat.debug_string() << "\n" );

            assert(  cat.is_set( "default" ) );
            assert(  cat.is_set( "a" ) );
            assert(  cat.is_set( "a.implied" ) );
            assert(  cat.is_set( "a.default" ) );
            assert( !cat.is_set( "a.not_set" ) );
            assert(  cat.is_set( "b" ) );
            assert(  cat.is_set( "b.implied" ) );
            assert( !cat.is_set( "b.suppressed" ) );
            assert( !cat.is_set( "c" ) );
            assert(  cat.is_set( "c.default" ) );
            assert( !cat.is_set( "c.not_set" ) );
            assert(  cat.is_set( "d" ) );
            assert( !cat.is_set( "not_set" ) );
        }

        cat.set( "all" );

        for( int i = 1; i <= 2; i++ )
        {
            Z_LOG2( "zschimmer", Z_FUNCTION << "() " << cat.debug_string() << "\n" );

            assert(  cat.is_set( "default" ) );
            assert(  cat.is_set( "a" ) );
            assert(  cat.is_set( "a.implied" ) );
            assert(  cat.is_set( "a.default" ) );
            assert(  cat.is_set( "a.not_set" ) );
            assert(  cat.is_set( "b" ) );
            assert(  cat.is_set( "b.implied" ) );
            assert(  cat.is_set( "b.not_set" ) );
            assert( !cat.is_set( "b.suppressed" ) );
            assert(  cat.is_set( "c.not_set" ) );
            assert(  cat.is_set( "not_set" ) );
            assert(  cat.is_set( "not_set.not_set" ) );
            assert( !cat.is_set( "suppressed" ) );
        }    

        cat.set( "all", false );
    }

    cat.restore_from( safe );
    Z_LOG2( "zschimmer", Z_FUNCTION << "() " << cat.debug_string() << "\n" );
}

//-------------------------------------------------------------------Log_categories::Log_categories

Log_categories::Log_categories()
: 
    _zero_(this+1),
    _mutex( "log_categories", Mutex::kind_recursive_dont_log ) 
{  
    _valid = true; 
    _modified_counter = 1;
}

//--------------------------------------------------------------------------Log_categories::save_to
    
void Log_categories::save_to( Log_categories_content* to )
{
    Z_MUTEX( _mutex )
    {
        *to = *this;
    }

    Z_LOG2( "zschimmer", Z_FUNCTION << "() " << debug_string() << "\n" );
}

//---------------------------------------------------------------------Log_categories::restore_from

void Log_categories::restore_from( const Log_categories_content& from )
{
    Z_MUTEX( _mutex )
    {
        Map previous_map = _map;

        *(Log_categories_content*)this = from;

        Z_FOR_EACH( Map, previous_map, e )  
        {
            is_set2( e->first, true );      // Bereits abgefragte Kategorien wieder abfragen, dass sie nicht verloren gehen (z.B. "exception.SOS-1234" );

            if( e->second._used_counter + e->second._denied_counter > 0 )
            {
                if( _map.find( e->first ) == _map.end() )   // Neu?
                {
                    _map[ e->first ]._type = Entry::e_derived;
                }

                _map[ e->first ]._used_counter   = e->second._used_counter;
                _map[ e->first ]._denied_counter = e->second._denied_counter;
            }
        }

        _modified_counter++;
    }

    Z_LOG2( "zschimmer", Z_FUNCTION << "() " << debug_string() << "\n" );
}

//---------------------------------------------------------------------------Log_categories::is_set

bool Log_categories::is_set( const char* name )
{
    return Log_ptr::logging()  &&  ( name == NULL  ||  name[0] == '\0'  ||  is_set( string( name ) ) );
}

//---------------------------------------------------------------------------Log_categories::is_set

bool Log_categories::is_set( const string& name, bool is_derived )
{
    bool result = false;

    if( _really_all )  return true;     // Für set_all(), kann von Signal-Handler (z.B. SIGUSR1) gesetzt werden.
    if( name == ""  )  return true;
    if( !_valid     )  return true;

    Z_MUTEX( _mutex )
    {
        result = name == "all"? is_set2( ""  , is_derived )
                              : is_set2( name, is_derived );
    }

    //Z_DEBUG_ONLY( Z_LOG( Z_FUNCTION << "(\"" << name << "\") => " << result << ",  " << debug_string() << "\n" ) );
    return result;
}

//--------------------------------------------------------------------------Log_categories::is_set2

bool Log_categories::is_set2( const string& name, bool is_derived )
{
    bool result = false;

    //if( name != "" )
    {
        Map::iterator e = _map.find( name );
        
        if( e == _map.end() ) 
        {
            bool value                = false;
            bool parents_children_too = false;

            if( name != "" )
            {
                size_t point                = name.rfind( '.' );  
                string parent_name          = name.substr( 0, point == string::npos? 0 : point );

                if( is_set2( parent_name, true ) )
                {
                    Entry& parent        = _map[ parent_name ]; 
                    parents_children_too = parent._children_too | parent._children_too_derived;
                    value                = parents_children_too  &&  parent._value;   // parent.* ?
                }
            }

            // Eintrag merken, damit's beim nächsten Mal gleich gefunden wird.

            e = _map.insert( Map::value_type( name,  Entry( Entry::e_derived, value ) ) )
                .first;    

            e->second._default_value = value;
            //e->second._children_too = parents_children_too;
            e->second._children_too_derived = parents_children_too;
        }

        result = e->second._value;

        if( !is_derived )
        {
            if( result )  e->second._used_counter++;
                    else  e->second._denied_counter++;
        }
    }

    return result;
}

//------------------------------------------------------------------------------Log_categories::set

string Log_categories::set( const string& name_, bool value, Entry::Type type )
{
    //Deadlock  Z_LOGI2( "zschimmer", Z_FUNCTION << "(\"" << name_ << "\"," << value << "," << Entry::type_name( type ) << ")\n" );

    string name         = name_;
    bool   children_too = false;
    
    assert( type != Entry::e_derived );
    if( name == "" )  return "";

    if( name == "*"  ||  name == "all" )  
    {
        name = "";
        children_too = true;
    }
    else
    if( string_ends_with( name, ".*" ) )  
    {
        name.erase( name.length() - 2 );
        children_too = true;
    }

    Z_MUTEX( _mutex )
    {
        generate_missing_anchestors_of( name );

        Map::iterator e = _map.find( name );
        if( e == _map.end() ) 
        {
            e = _map.insert( Map::value_type( name, Entry( type, value ) ) ) .first;
            //e = _map[ name ] = Entry( type, value );
            e->second._children_too = children_too;
        }
        
        Entry& entry = _map[ name ];

        entry._value                = value;
        if( children_too )  entry._children_too = value;
        entry._children_too_derived = entry._children_too;  //children_too & value;

        //if( e == _map.end()  ||  entry._type == Entry::e_derived )  
        //entry._children_too = children_too & value;
        //entry._children_too_derived = children_too & value;

        if( entry._type == Entry::e_derived   ||
            type        == Entry::e_implicit  ||  
            type        == Entry::e_explicit    )  entry._type = type;

        modify( e, value, children_too );

        _modified_counter++;
    }

    //Deadlock  Z_LOG2( "zschimmer", debug_string() << "\n" );

    return name;
}

//---------------------------------------------------------------------------Log_categories::modify

void Log_categories::modify( const Map::iterator& e, bool value, bool children_too )
{
    //Z_DEBUG_ONLY( Z_LOGI( Z_FUNCTION << "(\"" << name << "\"," << value << ")\n" ) );
    assert( e != _map.end() );

    Entry& entry = e->second;
    
    // Für Kinder übernehmen

    string prefix = e->first;
    if( prefix != "" )  prefix += ".";

    Z_FOR_EACH( Map, _map, c )          // Quadratischer Aufwand
    {
        Entry& child = c->second;

        if( string_begins_with( c->first, prefix )                 &&   // Ist ein Kind "a." ?
            c->first != prefix                                      &&  //Falls prefix == ""
            ( !value ||    // Abschalten? Immer alle Kinder abschalten
              ( children_too? child._type != Entry::e_explicit
                          : child._type == Entry::e_implicit )  ) )
            //( !value  ||                                                    // Abschalten? Gilt auch für alle Kinder
            //  ( entry._children_too | entry._children_too_derived? child._type != Entry::e_explicit
            //                                                     : child._type == Entry::e_implicit ) )  &&
            //c->first.find( '.', prefix.length() ) == string::npos  )  // Kein weiterer Punkt, nur a.b, nicht a.b.c ?
            //c->first != "" )
        {
            child._value                = value;
          //child._children_too         &= value;
            child._children_too_derived = children_too & value;
            //child._children_too_derived = value  &&  ( entry._children_too || entry._children_too_derived );
            //modify( c, value, children_too );
        }
    }
}

//------------------------------------------------------------------Log_categories::reset_derived

//void Log_categories::reset_derived()
//{
//    for( Map::iterator e = _map.begin(); e != _map.end(); )
//    {
//        Map::iterator next_e = e;
//        ++next_e;
//
//        e->second._children_too_derived = false;
//
//        //if( e->second._type == Entry::e_derived )  
//        //{
//        //    if( e->second._has_default )
//        //    {
//        //        e->second._type  = Entry::e_standard;
//        //        e->second._value = e->second._default_value;
//        //    }
//        //    else
//        //    {
//        //        _map.erase( e );
//        //    }
//        //}
//        
//        e = next_e;
//    }
//}

//---------------------------------------------------Log_categories::generate_missing_anchestors_of

void Log_categories::generate_missing_anchestors_of( const string& child_name )
{
    string name = child_name;
    
    while( name != "" )
    {
        size_t point = name.rfind( '.' );  
        name.erase( point == string::npos? 0 : point );

        if( _map.find( name ) != _map.end() )  break;

        is_set2( name, true );  // Legt einen abgeleiteten Eintrag (e_derived) an
    }
}

//---------------------------------------------------------Log_categories::generate_missing_parents

//void Log_categories::generate_missing_parents()
//{
//    stdext::hash_set<string> missing_parent_names;
//
//    Z_FOR_EACH_CONST( Map, _map, e )
//    {
//        string name = e->first;
//
//        while( name != "" )
//        {
//            size_t point = name.rfind( '.' );  
//            name.erase( point == string::npos? 0 : point );
//
//            Map::iterator p = _map.find( name );
//            if( p == _map.end() )  
//            {
//                missing_parent_names.insert( name );
//            }
//        }
//    }
//
//    Z_FOR_EACH( stdext::hash_set<string>, missing_parent_names, p )
//    {
//        is_set2( *p, true );  // Legt einen abgeleiteten Eintrag (e_derived) an
//    }
//}

//--------------------------------------------------------------------Log_categories::is_set_cached

bool Log_categories::is_set_cached( Cached_log_category* c )
{ 
    bool result;

    if( !c->_is_valid )
    {
        result = true;
    }
    else
    {
        if( c->_last_modified_counter != _modified_counter )
        {
            c->_last_modified_counter = _modified_counter;
            c->_is_set                = is_set( c->_name );  
        }

        result = c->_is_set; 
    }

    return result;
}

//------------------------------------------------------------------------------Log_categories::set

void Log_categories::set( const string& name )
{
    if( !_valid )  return;


    if( name.empty() )  return;

    if( *name.rbegin() == '-' )  set( name.substr( 0, name.length() - 1 ), false );
                           else  set( name, true );
}

//------------------------------------------------------------------------------Log_categories::set

void Log_categories::set_multiple( const string& names )
{
    if( names.empty() )  return;

    if( names.find( ' ' ) != string::npos )
    {
        const char* p = names.c_str();
        string      word;

        while( *p )
        {
            while( isspace( (unsigned char)*p ) )  p++;
            word.erase();
            while( *p  &&  !isspace( (unsigned char)*p ) )  word += *p++;
            set( word );
        }
    }
    else
    {
        set( names );
    }
}

//----------------------------------------------------------------------Log_categories::set_default

void Log_categories::set_default( const string& name, bool value )
{
    Z_MUTEX( _mutex )
    {
        string real_name = set( name, value );

        _map[ real_name ]._has_default   = value;
        _map[ real_name ]._default_value = value;

        _modified_counter++;
    }
}

//------------------------------------------------------------------------Log_categories::map_copy

Log_categories::Map Log_categories::map_copy() const
{
    Map result;

    Z_MUTEX( _mutex )  
    {
        Z_FOR_EACH_CONST( Map, _map, e )
        {
            if( e->second._type != Entry::e_derived  ||
                e->second._used_counter              ||
                e->second._denied_counter )
            {
                result.insert( *e );
            }
        }
    }

    return result;
}

//-------------------------------------------------------------Log_categories::category_is_relevant

bool Log_categories::category_is_relevant( const string& name ) const
{ 
    bool result = false;

    //Z_DEBUG_ONLY( assert( name != "com" ) );

    Map::const_iterator e = _map.find( name );
    if( e != _map.end() )
    {
        if( e->second._children_too )
        {
            result = e->second._value;
        }
        else
        if( e->second._has_default )
        {
            result = e->second._value != e->second._default_value;
        }
        else
        if( e->second._type == Entry::e_explicit  ||  name == "" )
        {
            result = e->second._value != e->second._default_value;
        }
        else
        if( e->second._type == Entry::e_implicit )
        {
            size_t point       = name.find( "." );
            string parent_name = name.substr( 0, point == string::npos? 0 : point );

            Map::const_iterator p = _map.find( parent_name );
            result = p == _map.end()  ||
                     e->second._value != p->second._value;
        }
        else
        {
            size_t point       = name.find( "." );
            string parent_name = name.substr( 0, point == string::npos? 0 : point );

            Map::const_iterator p = _map.find( parent_name );
            if( p == _map.end() )
            {
                result = e->second._value; 
            }
            else
            {
                //result = !p->second._children_too? e->second._value
                //                                 : e->second._value != p->second._value;
                result = !p->second._children_too  &&  !p->second._children_too_derived? e->second._value
                                                                                       : e->second._value != p->second._value;
            }
        }
    }

    return result;
}

//------------------------------------------------------------------------Log_categories::to_string

string Log_categories::to_string() const
{
    vector<string> result;
    result.reserve( _map.size() );

    Z_MUTEX( _mutex )
    {
        //if( _really_all )  result << "really_all";

        Z_FOR_EACH_CONST( Map, _map, it )
        {
            const Entry& e    = it->second;
            string       name = it->first;

            if( category_is_relevant( name ) )
            {
                if( e._children_too )  name += name == ""? "*" : ".*";
                if( !e._value       )  name += '-';
                result.push_back( name );
            }
        }
    }

    sort( result.begin(), result.end() );
    return join( " ", result );
}

//---------------------------------------------------------------------Log_categories::debug_string

string Log_categories::debug_string() const
{
    S result;

    vector<string> ordered_names;
    ordered_names.reserve( _map.size() );

    Z_MUTEX( _mutex )
    {
        Z_FOR_EACH_CONST( Map, _map, e )  ordered_names.push_back( e->first );
        sort( ordered_names.begin(), ordered_names.end() );

        for( size_t i = 0; i < ordered_names.size(); i++ )
        {
            Map::const_iterator e = _map.find( ordered_names[ i ] );
            assert( e != _map.end() );

            if( i > 0 )  result << ", ";
            result << e->first;
            if( e->second._children_too         )  result << ".*";
            if( e->second._children_too_derived )  result << ".~*";
            if( !e->second._value               )  result << "-";

            switch( e->second._type )
            {
                case Entry::e_standard: break;
                case Entry::e_implicit: result << "+";  break;
                case Entry::e_explicit: result << "!";  break;
                case Entry::e_derived:  result << "~";  break;
                default: break;
            }

            if( e->second._has_default )  result << " default=" << e->second._default_value;
        }
    }

    result << ",  \"" << to_string() << "\"";

    return result;
}

//---------------------------------------------------------Cached_log_category::Cached_log_category
    
Cached_log_category::Cached_log_category( const string& name )
: 
    _name(name), 
    _last_modified_counter(-1), 
    _is_set(false),
    _is_valid(true)
{
}

//--------------------------------------------------------Cached_log_category::~Cached_log_category
    
Cached_log_category::~Cached_log_category()
{
    _is_valid = false;
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer
