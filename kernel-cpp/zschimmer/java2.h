// $Id: java.h 5531 2008-01-18 13:39:38Z jz $

#ifndef __ZSCHIMMER_JAVA2_H_
#define __ZSCHIMMER_JAVA2_H_


#include <vector>
#include <jni.h>
#include "threads.h"
#include "z_com.h"
#include "log.h"

namespace zschimmer {
namespace javabridge {

struct Class;
struct Method;
struct Local_jobject;

//-------------------------------------------------------------------------------------------------

string                          java_class_path_of_name     ( const string& className );

//--------------------------------------------------------------------------------------Simple_type
// Primitive Typen und Void, Object und Array

enum Simple_type 
{
    t_none    = 0,
    t_void    = 'V',
    t_boolean = 'Z',
    t_byte    = 'B',
    t_char    = 'C',
    t_short   = 'S',
    t_int     = 'I',
    t_long    = 'J',
    t_float   = 'F',
    t_double  = 'D',
    t_object  = 'L',
    t_array   = '['
};

//---------------------------------------------------------------------------------------------Type

struct Type
{
                                Type                        ( Simple_type t )                       : _simple_type( t ) { assert( t ); }
                                Type                        ( const string& class_name );
                                Type                        ( const char* class_name );

    Simple_type                 simple_type                 () const                                { return _simple_type; }
    string                      class_name                  () const                                { return _class_name; }
    string                      signature_string            () const;

    static Type                 of_type_signature_string    ( const string& signature );            // Z.B.: "I", "Ljava/lang/String;"

    static void                 check_class_name            ( const string& );
    static bool                 is_class_name               ( const string& );

  private:
    Simple_type                _simple_type;
    string                     _class_name;
};

//----------------------------------------------------------------------------------------Signature

struct Signature
{
                                Signature                   ( const string& signature_string )      : _string( signature_string ) {}

    const string&               as_string                   () const                                { return _string; }
                                operator const string&      () const                                { return _string; } 
    static void                 check_class_signature_string( const string& signature );
    static bool                 is_class_signature_string   ( const string& signature );            // "Ljava/lang/String;" => true
    static void                 check_simple_type_char      ( char c );
    static bool                 is_simple_type_char         ( char c );                             // 'I' => true

  protected:
    string const               _string;
};

//---------------------------------------------------------------------------------Simple_signature

struct Simple_signature : Signature
{
                                Simple_signature            ( const string& signature_string )      : Signature( signature_string ) {}

    static Simple_signature     of_type                     ( const Type& ); 
};

//-------------------------------------------------------------------------Parameter_list_signature

struct Parameter_list_signature : Signature
{
                                Parameter_list_signature    ( const string& signature_string )      : Signature( signature_string ) {}

    Type                        return_type                 () const;
    Simple_type                 return_simple_type          () const                                { return return_type().simple_type(); }
    int                         parameter_count             () const;

    static Parameter_list_signature of_types                ( const Type& result_type ); 
    static Parameter_list_signature of_types                ( const Type& result_type, const Type& );
    static Parameter_list_signature of_types                ( const Type& result_type, const Type&, const Type& ); 
    static Parameter_list_signature of_types                ( const Type& result_type, const Type&, const Type&, const Type& ); 
};

//---------------------------------------------------------------------------------------Local_value
// Wie Jvalue, mit kleinen Änderungen, um Jvalue erstmal nicht anzurühren. Jvalue sollte durch Local_value ersetzt werden.
// Neben Local_value könnte es noch ein Global_value geben, das jobject als GlobalRef hält und freigibt. Gemeinsame Oberklasse wäre dann ein Simple_value.

struct Local_value : Object
{
                                Local_value                 ()                                      : _type( t_void ) {}
                                //Local_value               ( jboolean v )                          : _type( t_void ) { assign( v ); }
                                Local_value                 ( bool v )                              : _type( t_void ) { assign_jboolean( v ); }
                                //Local_value               ( jbyte v )                             : _type( t_void ) { assign( v ); }
                                //Local_value               ( jchar v )                             : _type( t_void ) { assign( v ); }
                                Local_value                 ( jint v )                              : _type( t_void ) { assign_jint( v ); }
                                Local_value                 ( jlong v )                             : _type( t_void ) { assign_jlong( v ); }
                                Local_value                 ( jfloat v )                            : _type( t_void ) { assign_jfloat( v ); }
                                Local_value                 ( jdouble v )                           : _type( t_void ) { assign_jdouble( v ); }
                                Local_value                 ( jobject v )                           : _type( t_void ) { assign_jobject( v ); }
                                Local_value                 ( const char* v )                       : _type( t_void ) { assign_string( v ); }
                                Local_value                 ( const string& v )                     : _type( t_void ) { assign_string( v ); }
                                Local_value                 ( const Local_value& v )                      : _type( t_void ) { assign_value( v ); }
    virtual                    ~Local_value                 ();

    Local_value&                operator =                  ( bool v )                              { assign_jboolean( v ); return *this; }
    //Local_value&                operator =                  ( jboolean v )                          { assign( v ); return *this; }
    //Local_value&                operator =                  ( jbyte v )                             { assign( v ); return *this; }
    //Local_value&                operator =                  ( jchar v )                             { assign( v ); return *this; }
    Local_value&                operator =                  ( jshort v )                            { assign_jshort( v ); return *this; }
    Local_value&                operator =                  ( jint v )                              { assign_jint( v ); return *this; }
    Local_value&                operator =                  ( jlong v )                             { assign_jlong( v ); return *this; }
    Local_value&                operator =                  ( jfloat v )                            { assign_jfloat( v ); return *this; }
    Local_value&                operator =                  ( jdouble v )                           { assign_jdouble( v ); return *this; }
    Local_value&                operator =                  ( jobject v )                           { assign_jobject( v ); return *this; }
    Local_value&                operator =                  ( const char* v )                       { assign_string( v ); return *this; }
    Local_value&                operator =                  ( const string& v )                     { assign_string( v ); return *this; }
    Local_value&                operator =                  ( const Local_value& v )                      { assign_value( v ); return *this; }

    void                        assign                      ( bool v )                              { assign_jboolean( v ); }
    //void                        assign                      ( jboolean v )                          { assign( v ); }
    //void                        assign                      ( jbyte v )                             { assign( v ); }
    //void                        assign                      ( jchar v )                             { assign( v ); }
    void                        assign                      ( jshort v )                            { assign_jshort( v ); }
    void                        assign                      ( jint v )                              { assign_jint( v ); }
    void                        assign                      ( jlong v )                             { assign_jlong( v ); }
    void                        assign                      ( jfloat v )                            { assign_jfloat( v ); }
    void                        assign                      ( jdouble v )                           { assign_jdouble( v ); }
    void                        assign                      ( jobject v )                           { assign_jobject( v ); }
    void                        assign                      ( const char* v )                       { assign_string( v ); }
    void                        assign                      ( const string& v )                     { assign_string( v ); }

    void                        assign_jboolean             ( jboolean v )                          { set_simple_type( t_boolean );  _jvalue.z = v; }
    void                        assign_jbyte                ( jbyte v )                             { set_simple_type( t_byte    );  _jvalue.b = v; }
    void                        assign_jchar                ( jchar v )                             { set_simple_type( t_char    );  _jvalue.c = v; }
    void                        assign_jshort               ( jshort v )                            { set_simple_type( t_short   );  _jvalue.s = v; }
    void                        assign_jint                 ( jint v )                              { set_simple_type( t_int     );  _jvalue.i = v; }
    void                        assign_jlong                ( jlong v )                             { set_simple_type( t_long    );  _jvalue.j = v; }
    void                        assign_jfloat               ( jfloat v )                            { set_simple_type( t_float   );  _jvalue.f = v; }
    void                        assign_jdouble              ( jdouble v )                           { set_simple_type( t_double  );  _jvalue.d = v; }
    void                        assign_jobject              ( jobject v );
    void                        assign_jstring              ( jstring v )                           { assign_jobject( v ); }
    void                        assign_string               ( const char* v )                       { assign_string( v, (int)strlen( v ) ); }
    void                        assign_string               ( const string& v )                     { assign_string( v.data(), (int)v.length() ); }
    void                        assign_string               ( const char* v, int length );
    void                        assign_value                ( const Local_value& );
    //Global_value: void                        steal_local_ref              ( jobject );

    void                        set_simple_type             ( Simple_type );

    const jvalue&               get_jvalue                  ()                                      { return _jvalue; }

    jobject                     as_jobject                  () const                                { check_simple_type( t_object );  return _jvalue.l; }
    bool                        as_bool                     () const                                { check_simple_type( t_boolean );  return _jvalue.b != 0; }
    string                      as_string                   () const;

    void                        check_simple_type           ( const Simple_type& ) const;
    void                        check_type                  ( const Type& ) const;
    string                      signature_string            () const;

private:
    //Global_value: void                    set_global_ref                  ( jobject o );

private:
    jvalue                     _jvalue;
    Simple_type                _type;
};

//-----------------------------------------------------------------------------------Parameter_list

static const int parameter_list_reserve = 10;       // Einige Parameter extra, falls zu wenig übergeben werden

struct Parameter_list
{
    virtual                    ~Parameter_list              ()                                      {}
    virtual int                 size                        () const                                = 0;
    virtual const jvalue*       jvalue_array                () const                                = 0;

    static const Parameter_list& empty;
};

//-------------------------------------------------------------------------------raw_parameter_list

template<int SIZE>
struct raw_parameter_list : Parameter_list
{
                                raw_parameter_list          ()                                      { memset(_jvalues, 0, sizeof _jvalues); }
    int                         size                        () const                                { return SIZE; }
    const jvalue*               jvalue_array                () const                                { return _jvalues; }

    jvalue                     _jvalues[SIZE + parameter_list_reserve];  
};

//----------------------------------------------------------------------------raw_parameter_list<0>

template<>
struct raw_parameter_list<0> : Parameter_list
{
    int                         size                        () const                                { return 0; }
    const jvalue*               jvalue_array                () const                                { return NULL; }
};

//--------------------------------------------------------------------Local_variable_parameter_list

struct Local_variable_parameter_list : Parameter_list, Non_cloneable
{
    explicit Local_variable_parameter_list( int size ) :
        _size( size ),
        _java_values( size ),
        _jvalues ( new jvalue[ size + parameter_list_reserve ] )
    {
        memset(_jvalues, 0, (size + parameter_list_reserve) * sizeof (jvalue));
    }

    ~Local_variable_parameter_list() {
        delete[] _jvalues;
    }

    int size() const { return _size; }

    template<typename T>
    void set( int index, const T& v ) 
    {
        if( index >= _size )  throw_xc( Z_FUNCTION, index );

        _java_values[index].assign( v );
        _jvalues[index] = _java_values[index].get_jvalue();
    }

    //string signature() 
    //{
    //    string result;
    //    for( uint i = 0; i < _java_values.size(); i++ )
    //        result += _java_values[i].javaType().signature();
    //}

    string signature_string() const;
    const jvalue* jvalue_array() const { return _jvalues; }

private:
    int const                   _size;
    std::vector<Local_value>    _java_values;
    jvalue* const               _jvalues;
};

//---------------------------------------------------------------------------------------Parameters

struct Parameters : Local_variable_parameter_list
{
    Parameters() : Local_variable_parameter_list( 0 ) {}


    template<typename T0>
    explicit Parameters( const T0& p0 ) : Local_variable_parameter_list( 1 ) {
        set( 0, p0 );
    }


    template<typename T0,typename T1>
    Parameters( const T0& p0, const T1& p1 ) : Local_variable_parameter_list( 2 ) {
        set( 0, p0 );
        set( 1, p1 );
    }


    template<typename T0,typename T1,typename T2>
    Parameters( const T0& p0, const T1& p1, const T2& p2 ) : Local_variable_parameter_list( 3 ) {
        set( 0, p0 );
        set( 1, p1 );
        set( 2, p2 );
    }
};

//-------------------------------------------------------------------------------Abstract_jobject
// Neue, vereinfachte Version von Jobject.
// Ohne jobject.

struct Abstract_jobject : Object
{
    virtual                    ~Abstract_jobject            ()                                      {}

                                operator jobject            () const                                { return get_jobject(); }
    bool                        operator !                  () const                                { return get_jobject() == NULL; }
    Abstract_jobject&           operator =                  (jobject jo)                            { assign_(jo); return *this; }

    jobject                     j                           () const                                { return get_jobject(); }
    virtual jobject             get_jobject                 () const                                = 0;
    jobject                     take                        ()                                      { jobject result = get_jobject();  set_jobject(NULL);  return result; }
    void                        assign_                     ( jobject jo )                          { assign_jobject(jo); }
    void                        steal_local_ref             ( jobject jo );                         // lo wird danach ungültig!
    jobject                     local_ref                   (); 

    //Local_value               loval_value_call                  ( const string& method, const Signature&, const Parameter_list& = Parameter_list::empty ) const;

  protected:
    virtual void                assign_jobject              (jobject jo)                            = 0;
    virtual void                set_jobject                 (jobject)                               = 0;
    void                        steal_local_ref             (const Local_jobject&);                 // Falsche Benutzung
};

//--------------------------------------------------------------------------Abstract_global_jobject
// Neue, vereinfachte Version von Global_jobject
// Ohne jobject, assign_jobject() implementiert mit NewGlobalRef() und DeleteGlobalRef.

struct Abstract_global_jobject : Abstract_jobject
{
                                Abstract_jobject::assign_;
#ifdef Z_HAS_MOVE_CONSTRUCTOR
    void                        assign_                     (Abstract_global_jobject&&);
#endif

  protected:
    virtual void                assign_jobject              (jobject jo);
};

//----------------------------------------------------------------------------------Global_jobject2
// Neue, vereinfachte Version von Global_jobject.

struct Global_jobject2 : Abstract_global_jobject
{
                                Global_jobject2             ()                                      : _jobject(NULL) {}
                                Global_jobject2             ( jobject jo )                          : _jobject(NULL) { assign_(jo); }
                                Global_jobject2             ( const Global_jobject2& o )            : _jobject(NULL) { assign_(o._jobject); }

#ifdef Z_HAS_MOVE_CONSTRUCTOR
                                Global_jobject2             (Global_jobject2&& o)                   : _jobject(o._jobject) { o._jobject = NULL; }
#endif

                               ~Global_jobject2             ()                                      { assign_(NULL); }

    virtual jobject             get_jobject                 () const                                { return _jobject; }

  private:
    Global_jobject2&            operator =                  ( const Global_jobject2& );             // Nicht implementiert
    void                        set_jobject                 (jobject jo)                            { _jobject = jo; }

    jobject                    _jobject;
};

//--------------------------------------------------------------------------------------Class_based

struct Class_based : Abstract_global_jobject
{
    void                       java_object_allocate_        ();
    virtual Class*             java_object_class_           ()                                      = 0;

  protected:
    virtual void               assign_jobject               ( jobject );
};

//------------------------------------------------------------------------------------class_based<>

//template<class CLASS>
//struct class_based : Class_based
//{
//                                class_based                 ( CLASS* clas )                         : _class(clas) { java_object_allocate_(); }
//                                class_based                 ( const string& class_name )            : _class( Z_NEW( CLASS( class_name ) ) ) { java_object_allocate_(); }
//                                class_based                 ( const char*   class_name )            : _class( Z_NEW( CLASS( class_name ) ) ) { java_object_allocate_(); }
//
//    CLASS*                      java_object_class_          ()                                      { return _class; }
//
//private:
//    ptr<CLASS> const           _class;
//};

//------------------------------------------------------------------------------------Class_factory

struct Class_factory
{
                                Class_factory               ( const string& class_name )            : _class_name(class_name) {}

    virtual ptr<Class>          new_instance                () const                                = 0;
    const string&               class_name                  () const                                { return _class_name; }
    Class*                      clas                        () const;

private:
    string const               _class_name;
};

//----------------------------------------------------------------------------------class_factory<>

template<class CLASS>
struct class_factory : Class_factory
{
                                class_factory               ( const string& class_name )            : Class_factory(class_name) {}

    ptr<Class>                  new_instance                () const                                { return +Z_NEW( CLASS( class_name() ) ); }
    CLASS*                      clas                        () const                                { return (CLASS*)Class_factory::clas(); }

private:
    const string               _class_name;
};

//------------------------------------------------------------------------------class_factory_based

//template<class CLASS>
//struct class_factory_based : class_based<CLASS>
//{
//                                class_factory_based         ()                                      : class_based<CLASS>( CLASS::class_factory.clas() ) {}
//};

//-----------------------------------------------------------------------------------Class_register

struct Class_register
{
    struct Entry {
        const Class_factory*   _class_factory;              // Für Pointer-Vergleich
        ptr<Class>             _class;
    };


    Class*                      get_or_register_class       ( const Class_factory* );

    template<class CLASS>
    CLASS*                      get_or_register_class       ( const class_factory<CLASS>* f )       { return (CLASS*)get_or_register_class( (Class_factory*)f ); }

  private:
    Class*                      get                         ( const Class_factory* );
    Class*                      register_new_class          ( const Class_factory* );

    Mutex                      _mutex;
    stdext::hash_map<string,Entry> _map;
};

//-------------------------------------------------------------------------------------------------

template<class CLASS>
struct Array : Global_jobject2
{
    //TODO Noch zu implementieren
};

//---------------------------------------------------------------------------Jobject_debug_register

struct Jobject_debug_register : Object
{
    typedef std::map<string,int> Class_object_counter_map;

                                Jobject_debug_register      ();
    void                        add                         (jobject);
    void                        remove                      (jobject);
    Class_object_counter_map    class_object_counter_map    ();

private:
    typedef stdext::hash_map<jobject,int>  Map;

    Fill_zero                  _zero_;
    Mutex                      _mutex;
    Map                        _map;
};

//-------------------------------------------------------------------------------------------------

} //namespace javabridge
} //namespace zschimmer


#endif
