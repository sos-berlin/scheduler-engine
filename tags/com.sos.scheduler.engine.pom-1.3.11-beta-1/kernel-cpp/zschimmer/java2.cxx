// $Id: java.cxx 5971 2009-03-01 17:14:58Z jz $

#include "zschimmer.h"
#include "threads.h"
#include "z_com.h"
#include "java.h"

using namespace std;

namespace zschimmer {
namespace javabridge {

//-------------------------------------------------------------------------------------------static

const raw_parameter_list<0> empty_parameterlist;
const Parameter_list& Parameter_list::empty = empty_parameterlist;

//------------------------------------------------------------Class_register::get_or_register_class

Class* Class_register::get_or_register_class( const Class_factory* class_factory )
{
    Class* result = get( class_factory );
    if( !result )
        result = register_new_class( class_factory );

    return result;
}

//------------------------------------------------------------------------------Class_register::get

Class* Class_register::get( const Class_factory* class_factory )
{
    Class* result = NULL;
    const string& class_name = class_factory->class_name();

    Z_FAST_MUTEX( _mutex ) {    // Nur Lesezugriff
        if( Entry* entry = find_map_entry( _map, class_name ) ) {
            if( entry->_class_factory != class_factory )
                throw_xc( "Z-JAVA-119", class_name, entry->_class->name() );     // Dieselbe Klasse mit zwei Factory definiert
            result = entry->_class;
        }
    }

    return result;
}

//---------------------------------------------------------------Class_register::register_new_class

Class* Class_register::register_new_class( const Class_factory* class_factory )
{
    Z_LOG2(Vm::java_log_category, Z_FUNCTION << " " << class_factory->class_name() << "\n" );

    ptr<Class> clas = class_factory->new_instance();

    Z_FAST_MUTEX( _mutex ) {    // Schreibzugriff
        Entry* entry = &_map[ class_factory->class_name() ];
        entry->_class_factory = class_factory;
        entry->_class = clas;
    }

    return clas;
}

//------------------------------------------------------------------------Local_value::~Local_value

Local_value::~Local_value()
{
    set_simple_type( t_void );
}

//------------------------------------------------------------------------Local_value::assign_value

void Local_value::assign_value( const Local_value& o )
{
    if( o._type == t_object )
        assign_jobject( o.as_jobject() );
    else {
        _type = o._type;
        _jvalue = o._jvalue;
    }
}

//----------------------------------------------------------------------Local_value::assign_jobject

void Local_value::assign_jobject( jobject value )
{
    if( _type != t_object )
        set_simple_type( t_object );

    _jvalue.l = value;
    //Global_value: if( _jvalue.l != value )
    //Global_value:     set_global_ref( value );
}

//-----------------------------------------------------------------------Local_value::assign_string

void Local_value::assign_string( const char* str, int length )
{
    Env env;
    assign_jobject(env.jstring_from_string( str, length ));
    //Global_value: steal_local_ref( env.jstring_from_string( str, length ) );
}

//---------------------------------------------------------------------Local_value::set_simple_type

void Local_value::set_simple_type( Simple_type type )
{
    //Global_value: if( _type == t_object )
    //Global_value:     set_global_ref( NULL );

    _type = type;
    _jvalue.l = NULL;
}

//-------------------------------------------------------------------Local_value::check_simple_type

void Local_value::check_simple_type( const Simple_type& type ) const
{
    if( _type != type )
        throw_xc( "Z-JAVA-114", string( 1, (char)type ) );
}

//--------------------------------------------------------------------------Local_value::check_type

void Local_value::check_type( const Type& type ) const
{
    if( type.simple_type() == t_object )
        if( Class::of_object( _jvalue.l )->name() == type.signature_string() )
            return;

    if( _type == type.simple_type() )
        return;

    throw_xc( "Z-JAVA-114", type.signature_string() );
}

//----------------------------------------------------------------------Local_value::set_global_ref

//Global_value: void Local_value::set_global_ref( jobject jo )
//Global_value: {
//Global_value:     assert( _type == t_object );
//Global_value:
//Global_value:     if( _jvalue.l != jo )
//Global_value:     {
//Global_value:         Env env;
//Global_value:
//Global_value:         jobject new_jobject = NULL;
//Global_value:
//Global_value:         if( jo )
//Global_value:         {
//Global_value:             Z_LOG2( Vm::java_log_category, "NewGlobalRef()\n" );
//Global_value:             new_jobject = env->NewGlobalRef( jo );
//Global_value:             env.check_result(new_jobject, "NewGlobalRef");
//Global_value:         }
//Global_value:
//Global_value:         if( _jvalue.l )
//Global_value:         {
//Global_value:             Z_LOG2( Vm::java_log_category, "DeleteGlobalRef()\n" );
//Global_value:             env->DeleteGlobalRef( _jvalue.l );
//Global_value:             _jvalue.l = NULL;
//Global_value:         }
//Global_value:
//Global_value:         _jvalue.l = new_jobject;
//Global_value:     }
//Global_value: }

//---------------------------------------------------------------------Local_value::steal_local_ref

//Global_value: void Local_value::steal_local_ref( jobject jo )
//Global_value: {
//Global_value:     assign_jobject( jo );
//Global_value:
//Global_value:     if( jo ) {
//Global_value:         Env env;
//Global_value:         assert( env->GetObjectRefType( jo ) == JNILocalRefType );
//Global_value:         Z_LOG2( Vm::java_log_category, "DeleteLocalRef()\n" );
//Global_value:         env->DeleteLocalRef( jo );
//Global_value:     }
//Global_value: }

//-----------------------------------------------------------------------Local_value::set_local_ref

//Global_value: void Local_value::set_local_ref( jobject jo )
//Global_value: {
//Global_value:     assert( _type == t_object );
//Global_value:
//Global_value:     if( _jvalue.l != jo )
//Global_value:     {
//Global_value:         Env env;
//Global_value:
//Global_value:         jobject new_jobject = NULL;
//Global_value:
//Global_value:         if( jo )
//Global_value:         {
//Global_value:             //TODO Soll jo am Ende freigegeben werden? Besser explizite Methode takeFrom()?
//Global_value:             new_jobject = jo;
//Global_value:             //Z_LOG2( Vm::java_log_category, "NewLocalRef()\n" );
//Global_value:
//Global_value:             //new_jobject = env->NewLocalRef( jo );
//Global_value:             //if( !new_jobject || env->ExceptionCheck() )  env.throw_java( "NewLocalRef" );
//Global_value:
//Global_value:             //env->DeleteLocalRef( jo );    // jo wird ungültig!
//Global_value:         }
//Global_value:
//Global_value:         if( _jvalue.l )
//Global_value:         {
//Global_value:             Z_LOG2( Vm::java_log_category, "DeleteLocalRef()\n" );
//Global_value:             env->DeleteLocalRef( _jvalue.l );
//Global_value:             _jvalue.l = NULL;
//Global_value:         }
//Global_value:
//Global_value:         _jvalue.l = new_jobject;
//Global_value:     }
//Global_value: }

//---------------------------------------------------------------------------Local_value::as_string

string Local_value::as_string() const
{
    //TODO Sicherstellen, dass es java.lang.String ist.
    return Env().string_from_jstring( (jstring)as_jobject() );
}

//------------------------------------------------------------------------------replaced_characters

static string replaced_characters( const string& value, char oldChar, char newChar )  //TODO Veröffentlichen
{
    string result = value;

    for( size_t i = 0; i < result.length(); i++ )
        if( result[i] == oldChar )
            result[i] = newChar;

    return result;
}

//--------------------------------------------------------------------Local_value::signature_string

string Local_value::signature_string() const
{
    string result;

    if( _type == t_object ) {
        result = 'L' + java_class_path_of_name( Class::of_object( _jvalue.l )->name() ) + ';';
    } else {
        result += (char)_type;
    }

    return result;
}

//---------------------------------------------------------------------------java_class_path_of_name

string java_class_path_of_name( const string& className )
{
    return replaced_characters( className, '.', '/' );
}

//---------------------------------------------------------------------------------------Type::Type

Type::Type( const string& class_name )
:
    _simple_type( t_object ),
    _class_name(class_name)
{
    check_class_name( _class_name );
}

//---------------------------------------------------------------------------------------Type::Type

Type::Type( const char* class_name )
:
    _simple_type( t_object ),
    _class_name(class_name)
{
    check_class_name( _class_name );
}

//---------------------------------------------------------------------------Type::signature_string

string Type::signature_string() const
{
    return _simple_type == t_object?
        'L' + java_class_path_of_name( _class_name ) + ';'
      : string( 1, (char)_simple_type );
}

//-------------------------------------------------------------------Type::of_type_signature_string

Type Type::of_type_signature_string( const string& signature )
{
    if( signature.length() == 1 ) {
        Signature::check_simple_type_char( signature[0] );
        return Type( (Simple_type)signature[0] );
    }
    else {
        Signature::check_class_signature_string( signature );
        return Type( signature.substr( 1, signature.length() - 2 ) );
    }
}

//---------------------------------------------------------------------------Type::check_class_name

void Type::check_class_name( const string& class_name )
{
    if( !is_class_name( class_name ) )
        throw_xc( "Z-JAVA-117", class_name );
}

//------------------------------------------------------------------------------Type::is_class_name

bool Type::is_class_name( const string& class_name )
{
    if( class_name == "" )
        return false;

    //TODO  Hier vielleicht noch Klassensyntax prüfen: Namen mit Punkten voneinander getrennt.

    return true;
}

//----------------------------------------------------------------Signature::check_simple_type_char

void Signature::check_simple_type_char( char c )
{
    if( !is_simple_type_char( c ) )
        throw_xc( "Z-JAVA-116", c );
}

//-------------------------------------------------------------------Signature::is_simple_type_char

bool Signature::is_simple_type_char( char c )
{
    return
        c == t_void    ||
        c == t_boolean ||
        c == t_byte    ||
        c == t_char    ||
        c == t_short   ||
        c == t_int     ||
        c == t_long    ||
        c == t_float   ||
        c == t_double;
}

//----------------------------------------------------------Signature::check_class_signature_string

void Signature::check_class_signature_string( const string& signature )
{
    if( !is_class_signature_string( signature ) )
        throw_xc( "Z-JAVA-115", signature );
}

//-------------------------------------------------------------Signature::is_class_signature_string

bool Signature::is_class_signature_string( const string& signature )
{
    if( signature.length() <= 3 )
        return false;

    if( signature[0] != 'L' )
        return false;

    if( *signature.rbegin() != ';' )
        return false;

    //TODO Hier vielleicht noch Klassensyntax prüfen: Namen mit Schrägstrichen voneinander getrennt.

    return true;
}

//------------------------------------------------------------------------Simple_signature::of_type

Simple_signature Simple_signature::of_type(const Type& type)
{
    return Simple_signature(type.signature_string());
}

//---------------------------------------------------------------Parameter_list_signature::of_types

Parameter_list_signature Parameter_list_signature::of_types( const Type& result_type )
{
    return Parameter_list_signature( "()" + result_type.signature_string() );
}

//---------------------------------------------------------------Parameter_list_signature::of_types

Parameter_list_signature Parameter_list_signature::of_types( const Type& result_type, const Type& t1 )
{
    return Parameter_list_signature( '(' + t1.signature_string() + ')' + result_type.signature_string() );
}

//---------------------------------------------------------------Parameter_list_signature::of_types

Parameter_list_signature Parameter_list_signature::of_types( const Type& result_type, const Type& t1, const Type& t2 )
{
    return Parameter_list_signature( '(' + t1.signature_string() + t2.signature_string() + ')' + result_type.signature_string() );
}

//---------------------------------------------------------------Parameter_list_signature::of_types

Parameter_list_signature Parameter_list_signature::of_types( const Type& result_type, const Type& t1, const Type& t2, const Type& t3 )
{
    return Parameter_list_signature( '(' + t1.signature_string() + t2.signature_string() + t3.signature_string() + ')' + result_type.signature_string() );
}

//------------------------------------------------------------Parameter_list_signature::return_type

Type Parameter_list_signature::return_type() const
{
    size_t end = _string.rfind( ')' );
    if( end == string::npos )
        throw_xc( Z_FUNCTION, "Ungültige Signatur" );

    return Type::of_type_signature_string( _string.substr( end + 1 ) );
}

//--------------------------------------------------------Parameter_list_signature::parameter_count

int Parameter_list_signature::parameter_count() const
{
    assert( _string.length() >= 3 );
    assert( _string[0] == '(' );

    int result = 0;

    for( const char* p = _string.c_str() + 1; *p && *p != ')'; ) {
        if( *p == (char)t_object ) {       // zB "Ljava/lang/String;"
            p++;
            while( *p  &&  *p != ';' )  p++;
            if( *p )  p++;
            result++;
        }
        else {
            p++;
            result++;
        }
    }

    return result;
}

//-------------------------------------------------Local_variable_parameter_list::signature_string

string Local_variable_parameter_list::signature_string() const
{
    string result = "(";

    for( int i = 0; i < _size; i++ )
        result += _java_values[i].signature_string();
    result += ')';

    return result;
}

//------------------------------------------------------------------------------------Jvalue::check

void Jvalue::check( char type )
{
    if( _type != type )
        throw_xc( "Z-JAVA-107", string( &type, 1 ), string( &_type, 1 ) );
}

//----------------------------------------------------------------Abstract_jobject::steal_local_ref

void Abstract_jobject::steal_local_ref( jobject jo )
{
    if( jo != get_jobject() ) {
        assign_( jo );

        if( jo ) {
            Env env;
            assert( env->GetObjectRefType( jo ) == JNILocalRefType );
            env->DeleteLocalRef( jo );
        }
    }
}

//-------------------------------------------------------------------Abstract_jobject::local_ref_of

jobject Abstract_jobject::local_ref()
{
    jobject jo = get_jobject();
    return jo? Env().new_local_ref(jo) : NULL;
}

//----------------------------------------------------------------------Abstract_jobject::construct

//void Abstract_jobject::construct( const Parameter_list_signature& signature, const Parameter_list& parameter_list )
//{
//    assert( signature.return_simple_type() == t_void );
//    value_call( "<init>", signature, parameter_list );
//}

//----------------------------------------------------------------------Abstract_jobject::construct

//void Abstract_jobject::construct()
//{
//    construct( Parameter_list_signature( "()V" ), Parameter_list::empty );
//}

//---------------------------------------------------------------------Abstract_jobject::value_call

//Local_value Abstract_jobject::value_call( const string& method_name, const Parameter_list_signature& signature, const Parameter_list& parameter_list ) const
//{
//    if( signature.parameter_count() != parameter_list.size() )
//        throw_xc( Z_FUNCTION, method_name, "Parameteranzahl passt nicht" );
//
//    //TODO Außer Anzahl auch die einzelnen Parametertypen vergleichen.
//
//    Method method ( Class::of_object( get_jobject() ), method_name, signature );
//    return method.value_call( get_jobject(), parameter_list );
//}

//----------------------------------------------------------Abstract_global_jobject::assign_jobject

void Abstract_global_jobject::assign_jobject( jobject jo )
{
    jobject this_jobject = get_jobject();

    if( this_jobject != jo )
    {
        Env env = jni_env();

        jobject new_jobject = NULL;

        if( jo )
            new_jobject = env.new_global_ref(jo);

        if( this_jobject )
            env.delete_global_ref(this_jobject);

        set_jobject(new_jobject);
    }
}

//-----------------------------------------------------------------Abstract_global_jobject::assign_

#ifdef Z_HAS_MOVE_CONSTRUCTOR
void Abstract_global_jobject::assign_(Abstract_global_jobject&& o)
{
    jobject jo = get_jobject();
    set_jobject(o.get_jobject());
    o.set_jobject(jo);
}
#endif

//---------------------------------------------------------------Class_based::java_object_allocate_

void Class_based::java_object_allocate_()
{
    assert( !get_jobject() );
    steal_local_ref( java_object_class_()->alloc_object() );
}

//----------------------------------------------------------------------Class_based::assign_jobject

void Class_based::assign_jobject(jobject jo)
{
    if (jo)  java_object_class_()->assert_is_assignable_from(jo);
    Abstract_global_jobject::assign_jobject(jo);
}

//----------------------------------------------------------Class_based::_java_object_is_assignable

//bool Class_based::_java_object_is_assignable(jobject jo)
//{
//    return jo == NULL  ||  0 != Env()->IsInstanceOf(jo, java_object_class_()->get_jclass());
//}

//------------------------------------------------------------------------------Class_factory::clas

Class* Class_factory::clas() const
{
    if (!Vm::static_vm)  throw_xc("Z-JAVA-101", Z_FUNCTION, _class_name);

    return Vm::static_vm->_class_register.get_or_register_class( this );
}

//---------------------------------------------------Jobject_debug_register::Jobject_debug_register

Jobject_debug_register::Jobject_debug_register()
:
    _zero_(this+1)
{}

//----------------------------------------------------------------------Jobject_debug_register::add

void Jobject_debug_register::add(jobject jo)
{
    Z_FAST_MUTEX(_mutex)
        _map[jo]++;
}

//-------------------------------------------------------------------Jobject_debug_register::remove

void Jobject_debug_register::remove(jobject jo)
{
    Z_FAST_MUTEX(_mutex) {
        if (--_map[jo] == 0)
            _map.erase(jo);
    }
}

//-------------------------------------------------Jobject_debug_register::class_object_counter_map

Jobject_debug_register::Class_object_counter_map Jobject_debug_register::class_object_counter_map()
{
    Class_object_counter_map result;
    Env env;

    Z_FAST_MUTEX(_mutex) {
        Z_FOR_EACH(Map, _map, it) result[Class::of_object(it->first)->name()]++;
    }

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace javabridge
} //namespace zschimmer
