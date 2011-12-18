// $Id: java.h 14135 2010-11-11 15:55:02Z jz $

#ifndef __ZSCHIMMER_JAVA_H_
#define __ZSCHIMMER_JAVA_H_


#include <vector>
#include <jni.h>
#include "threads.h"
#include "z_com.h"
#include "log.h"
#include "java2.h"


namespace zschimmer {
namespace javabridge {

const int                       max_params                  = 10;

struct Vm;
struct Method;
struct Class;
struct Local_jobject;
struct Jobject_debug_register;

//-------------------------------------------------------------------------------------------------

JNIEnv*                         jni_env                     ();
JavaVM*                         javavm_from_jnienv          ( JNIEnv* );
Vm*                             create_vm                   ( bool init = true );
Z_NORETURN void                 throw_java_ret              ( int return_value, const string& method_name, const string& = "" );
string                          expand_class_path           ( const string& class_path_with_wildcards );

//---------------------------------------------------------------------------------Java_thread_data

struct Java_thread_data
{
                                Java_thread_data            ()                                      : _zero_(this+1) {}


    Fill_zero                  _zero_;
    JNIEnv*                    _jni_env;
};

//-----------------------------------------------------------------------------------Java_exception

struct Java_exception : Xc
{
                                Java_exception              ( const char* code )                    : Xc(code) {}
    Z_GNU_ONLY(                ~Java_exception              () throw()                              {}  )               // gcc 3.2.2

    string                      name                        () const                                { return _exception_name; }
    string                      message                     () const                                { return _message; }

    string                     _exception_name;
    string                     _message;
};

//----------------------------------------------------------------------------------------------Env

struct Env 
{
                                Env                         ();
                                Env                         ( JNIEnv* jenv )                        : _jni_env( jenv ) {}
                                Env                         ( Vm* );

                                operator JNIEnv*            ()                                      { return jni_env(); }
    JNIEnv*                     operator ->                 ()                                      { return jni_env(); }

    JNIEnv*                     jni_env                     ()                                      { return _jni_env; }
    JavaVM*                     vm                          ()                                      { return javavm_from_jnienv( _jni_env ); }
    Vm*                         get_vm                      ();

    Z_NORETURN void             throw_java                  ( const string&, const string& = "" );
    void                        log_stack_trace             (jthrowable);

    string                      string_from_jstring         ( const jstring& );
    jstring                     jstring_from_string         ( const char*, size_t length );
    jstring                     jstring_from_string         ( const char* );
    jstring                     jstring_from_string         ( const string& );
    jstring                     jstring_from_string         ( const com::Bstr& );
    jstring                     jstring_from_string         (const OLECHAR*, size_t length);

    string                      get_class_name              ( jclass );
    jobject                     new_local_ref               (jobject);
    jobject                     new_global_ref              (jobject);
    void                        delete_global_ref           (jobject);

    bool                        result_is_ok                (jobject result);
    void                        check_result                (jobject result, const char* static_debug_text);
    void                        set_java_exception          ( const char* what );
    void                        set_java_exception          ( const exception& );
    void                        set_java_exception          ( const _com_error& );
    void                        set_NullPointerException    ( const string& text = "" );

    bool                        is_byte_array_class         ( jobject );
    bool                        is_string_array_class       ( jobject );
    jclass                      get_spooler_idispatch_class_if_is_instance_of( jobject );


    // Einige Java-Methoden mit Fehlerpr�fung:
    jclass                      find_class                  ( const string& name );
    jfieldID                    get_field_id                ( jclass cls, const string& name, const string& signature );
    jmethodID                   get_method_id               ( jclass cls, const string& name, const string& signature ) { return get_method_id( cls, name.c_str(), signature.c_str() ); }
    jmethodID                   get_method_id               ( jclass, const char* name, const char* signature );
    jmethodID                   get_static_method_id        ( jclass cls, const string& name, const string& signature ) { return get_static_method_id( cls, name.c_str(), signature.c_str() ); }
    jmethodID                   get_static_method_id        ( jclass, const char* name, const char* signature );

private:
    JNIEnv*                    _jni_env;
};

//--------------------------------------------------------------------------------------Local_frame

struct Local_frame
{
                                Local_frame                 ( int capacity );
                               ~Local_frame                 ()                                      { _jni_env->PopLocalFrame(NULL); }

    JNIEnv*                    _jni_env;
};

//-------------------------------------------------------------------------------------------Jvalue

struct Jvalue
{
                                Jvalue                      ( jobject value )                       : _type( 'L' ) { _jvalue.l = value; }
                                Jvalue                      ( jboolean value )                      : _type( 'Z' ) { _jvalue.z = value; }
                                Jvalue                      ( bool value )                          : _type( 'Z' ) { _jvalue.z = value; }
                                Jvalue                      ( char value )                          : _type( 'C' ) { _jvalue.c = value; }
                                Jvalue                      ( jshort value )                        : _type( 'S' ) { _jvalue.s = value; }
                                Jvalue                      ( jint value )                          : _type( 'I' ) { _jvalue.i = value; }
                                Jvalue                      ( jlong value )                         : _type( 'j' ) { _jvalue.j = value; }
                                Jvalue                      ( jfloat value )                        : _type( 'F' ) { _jvalue.f = value; }
                                Jvalue                      ( jdouble value )                       : _type( 'D' ) { _jvalue.d = value; }

    const jvalue&               get_jvalue                  ()                                      { return _jvalue; }

    jobject                     as_jobject                  ()                                      { check( 'L' );  return _jvalue.l; }
    bool                        as_bool                     ()                                      { check( 'Z' );  return _jvalue.b != 0; }

    void                        check                       ( char type );

    jvalue                     _jvalue;
    char                       _type;
};

//------------------------------------------------------------------------------------------Jobject

struct Jobject : Jvalue
{
  protected:
                                Jobject                     ()                                      : Jvalue( (jobject)NULL )  {}
                                Jobject                     ( jobject jo )                          : Jvalue( jo ) {}
                                Jobject                     ( const Jobject& )                      : Jvalue( (jobject)NULL )  {}   // Muss von Unterklasse implementiert werden!

    Jobject&                    operator =                  ( const Jobject& o )                    { assign_add_ref( o.get_jobject() );  return *this; } 

  public:
    virtual                    ~Jobject                     ()                                      {}


                                operator jobject            () const                                { return get_jobject(); }
    Jobject&                    operator =                  ( jobject jo )                          { assign( jo );  return *this; }
    bool                        operator !                  () const                                { return get_jobject() == NULL; }

    jobject                     take                        ()                                      { jobject result = _jvalue.l;  _jvalue.l = NULL;  return result; }

    virtual void                assign                      ( jobject )                             = 0;    // jobject muss eine LocalRef sein, und wird danach ung�ltig!
    virtual void                assign_add_ref              ( jobject )                             = 0;    // jobject muss eine LocalRef sein, und wird danach ung�ltig!
    virtual jclass              get_jclass                  ()                                      = 0;

    jobject                     j                           () const                                { return get_jobject(); }
    jobject                     get_jobject                 () const                                { return _jvalue.l; }

    jmethodID                   method_id                   ( const char*   name, const char*   signature );
    jmethodID                   method_id                   ( const string& name, const string& signature ) { return method_id( name.c_str(), signature.c_str() ); }

    jmethodID                   static_method_id            ( const char*   name, const char*   signature );
    jmethodID                   static_method_id            ( const string& name, const string& signature ) { return static_method_id( name.c_str(), signature.c_str() ); }

    void                        call_void_method            ( const char* name, const char* signature, ... );
    jobject                     call_object_method          ( const char* name, const char* signature, ... );
    jint                        call_int_method             ( const char* name, const char* signature, ... );
    jlong                       call_long_method            ( const char* name, const char* signature, ... );
    bool                        call_bool_method            ( const char* name, const char* signature, ... );
    bool                        call_bool_method            ( jmethodID, ... );
    string                      call_string_method          ( const char* name, const char* signature, ... );
    string                      call_string_method          ( jmethodID, ... );

    jobject                     call_static_object_method   ( const char* name, const char* signature, ... );
};

//-----------------------------------------------------------------------------------Global_jobject

struct Global_jobject : Jobject
{
                                Global_jobject              ()                                      : _jclass(NULL)  {}
                                Global_jobject              ( jobject jo )                          : _jclass(NULL)  { assign( jo ); }
                                Global_jobject              ( const Global_jobject& o )             : _jclass(NULL)  { assign( o.get_jobject() ); }
                               ~Global_jobject              ()                                      { assign( NULL ); }

    Global_jobject&             operator =                  ( jobject jo )                          { assign_add_ref( jo );  return *this; }
    bool                        operator !                  () const                                { return get_jobject() == NULL; }

    void                        assign                      ( jobject jo )                          { assign_add_ref( jo ); }
    void                        assign_add_ref              ( jobject );
    jclass                      get_jclass                  ();

  private:
    jclass                     _jclass;
};

//---------------------------------------------------------------------------------global_jobject<>

template< class CLASS >
struct global_jobject : Global_jobject
{
                                global_jobject              ()                                      {}
                                global_jobject              ( CLASS jo )                            { assign( jo ); }

                                operator CLASS              () const                                { return (CLASS)get_jobject(); }
    global_jobject<CLASS>&      operator =                  ( CLASS jo )                            { assign( jo );  return *this; }
    bool                        operator !                  () const                                { return get_jobject() == NULL; }
    bool                        operator ==                 ( CLASS jo ) const                      { return (CLASS)get_jobject() == jo; }
    bool                        operator !=                 ( CLASS jo ) const                      { return (CLASS)get_jobject() != jo; }
    void                        assign                      ( CLASS jo )                            { Global_jobject::assign( jo ); }
};

//------------------------------------------------------------------------------------Local_jobject

struct Local_jobject : Jobject
{
                                Local_jobject               ()                                      : _jclass(NULL), _call_close_at_end(false)  {}
                                Local_jobject               ( jobject jo )                          : _jclass(NULL), _call_close_at_end(false)  { assign( jo ); }
                                Local_jobject               ( const Local_jobject& o )              : _jclass(NULL)  { assign_add_ref( o.get_jobject() ); }
                               ~Local_jobject               ();

    Local_jobject&              operator =                  ( jobject jo )                          { assign( jo );  return *this; }
    bool                        operator !                  () const                                { return get_jobject() == NULL; }

    void                        assign_add_ref              ( jobject );
    void                        assign                      ( jobject );
    jclass                      get_jclass                  ();
    void                        call_close_at_end           ( bool b = true )                       { _call_close_at_end = b; }

  private:
    jclass                     _jclass;
    bool                       _call_close_at_end;
};

//----------------------------------------------------------------------------------local_jobject<>

template< class CLASS >
struct local_jobject : Local_jobject
{
                                local_jobject               ()                                      {}
                                local_jobject               ( CLASS jo )                            { assign( jo ); }

                                operator CLASS              () const                                { return (CLASS)get_jobject(); }
                                operator bool               () const                                { return get_jobject() != NULL; }
    bool                        operator !                  () const                                { return get_jobject() == NULL; }
    local_jobject<CLASS>&       operator =                  ( CLASS jo )                            { assign( jo );  return *this; }
    void                        assign                      ( CLASS jo )                            { Local_jobject::assign( jo ); }
};

//------------------------------------------------------------------------------------Local_jstring

struct Local_jstring : local_jobject<jstring>
{
                                Local_jstring               ()                                      {}
                                Local_jstring               ( jstring jstr )                        { assign( jstr ); }
                                Local_jstring               ( const string& str )                   { assign( str ); }
                                Local_jstring               ( const char* str )                     { assign( str ); }
                                Local_jstring               ( const char* str, size_t length )      { assign( str, length ); }

    Local_jstring&              operator =                  ( jstring jstr )                        { assign( jstr );  return *this; }
    Local_jstring&              operator =                  ( const string& str )                   { assign( str  );  return *this; }
    Local_jstring&              operator =                  ( const char* str )                     { assign( str  );  return *this; }
                                operator jstring            ()                                      { return get_jstring(); }

    void                        assign                      ( jstring jstr )                        { local_jobject<jstring>::assign( jstr ); }
    void                        assign                      ( const string& str )                   { assign( str.data(), str.length() ); }
    void                        assign                      ( const char* str )                     { assign( str, strlen( str ) ); }
    void                        assign                      ( const char* s, size_t l )             { assign( Env().jstring_from_string( s, l ) ); }
    jstring                     get_jstring                 ()                                      { return (jstring)get_jobject(); }
};

//----------------------------------------------------------------------------Read_jstring_critical
// Nur kurzzeitig benutzen! Keine JNI-Aufrufe bis ~Read_jstring_critical!

struct Read_jstring_critical : Non_cloneable
{
                                Read_jstring_critical       ( jstring );
                               ~Read_jstring_critical       ();

    int                         length                      () const                                { return _jni_env->GetStringLength( _jstring ); }
                                operator const jchar*       ()                                      { return _jchars; }
                              //operator const OLECHAR*     ()                                      { return (const OLECHAR*)_jchars; }
    const jchar&                operator []                 ( int i )                               { return _jchars[ i ]; }

    JNIEnv*                    _jni_env;
    jstring                    _jstring;
    const jchar*               _jchars;
};

//--------------------------------------------------------------------------------------------Class

struct Class : Global_jobject, Object
{
                                Class                       ( const char* name = NULL );
                                Class                       ( const string& name );
                                Class                       ( jclass c )                            : Global_jobject( c ) {}

    void                        load                        ( const string& name );

    Class&                      operator =                  ( jclass c )                            { assign( c ); return *this; }
                                operator jclass             () const                                { return get_jclass(); }

    jclass                      get_jclass                  () const                                { return (jclass)get_jobject(); }

    jmethodID                   method_id                   ( const char*   name, const char*   signature ) const;
    jmethodID                   method_id                   ( const string& name, const string& signature ) const { return method_id( name.c_str(), signature.c_str() ); }

    jfieldID                    field_id                    ( const char*   name, const char*   signature ) const;
    jfieldID                    field_id                    ( const string& name, const string& signature ) const { return field_id( name.c_str(), signature.c_str() ); }

    jmethodID                   static_method_id            ( const char*   name, const char*   signature ) const;
    jmethodID                   static_method_id            ( const string& name, const string& signature ) const { return static_method_id( name.c_str(), signature.c_str() ); }

    jobject                     alloc_object                () const;

    jobject                     new_object                  ( const char* signature, ... ) const;

    const string&               name                        ();

    void                        assert_is_assignable_from   (jobject) const;
    bool                        is_assignable_from          (jobject) const;

    static ptr<Class>           of_object                   ( jobject );

    string                     _name;
};

//-------------------------------------------------------------------------------------------Member

struct Member
{
                                Member                      ( const Class* );

    const Class*                clas                        () const                                { return _class; }

protected:
    const Class* const         _class;
};

//----------------------------------------------------------------------------------------Procedure

struct Procedure : Member
{
                                Procedure                   ( const Class*, const string& name, const Parameter_list_signature& );
                                Procedure                   ( const Class*, const char* name, const char* signature );

                                operator jmethodID          ()                                      { return id(); }
    jmethodID                   id                          () const                                { return _id; }
    void                        call                        ( jobject jo, const Parameter_list& = Parameter_list::empty ) const;

protected:
    const jmethodID            _id;
};

//-------------------------------------------------------------------------------------------Method

struct Method : Procedure
{
                                Method                      ( const Class* cls, const string& name, const Parameter_list_signature& signature ) : Procedure( cls, name, signature ) {}
                                Method                      ( const Class* cls, const char* name, const char* signature ) : Procedure( cls, name, signature ) {}

    bool                        bool_call                   ( jobject, const Parameter_list& = Parameter_list::empty ) const;
    jbyte                       byte_call                   ( jobject, const Parameter_list& = Parameter_list::empty ) const;
    wchar_t                     char_call                   ( jobject, const Parameter_list& = Parameter_list::empty ) const;
    short                       short_call                  ( jobject, const Parameter_list& = Parameter_list::empty ) const;
    int                         int_call                    ( jobject, const Parameter_list& = Parameter_list::empty ) const;
    int64                       long_call                   ( jobject, const Parameter_list& = Parameter_list::empty ) const;
    float                       float_call                  ( jobject, const Parameter_list& = Parameter_list::empty ) const;
    double                      double_call                 ( jobject, const Parameter_list& = Parameter_list::empty ) const;
    jobject                     jobject_call                ( jobject, const Parameter_list& = Parameter_list::empty ) const;
    string                      string_call                 ( jobject, const Parameter_list& = Parameter_list::empty ) const;
    //Value                       value_call                  ( jobject, const Parameter_list& = Parameter_list::empty ) const;
    void                        static_call                 ( jclass, const Parameter_list& = Parameter_list::empty ) const;
    bool                        bool_static_call            ( jclass, const Parameter_list& = Parameter_list::empty ) const;
    jbyte                       byte_static_call            ( jclass, const Parameter_list& = Parameter_list::empty ) const;
    wchar_t                     char_static_call            ( jclass, const Parameter_list& = Parameter_list::empty ) const;
    short                       short_static_call           ( jclass, const Parameter_list& = Parameter_list::empty ) const;
    int                         int_static_call             ( jclass, const Parameter_list& = Parameter_list::empty ) const;
    int64                       long_static_call            ( jclass, const Parameter_list& = Parameter_list::empty ) const;
    float                       float_static_call           ( jclass, const Parameter_list& = Parameter_list::empty ) const;
    double                      double_static_call          ( jclass, const Parameter_list& = Parameter_list::empty ) const;
    jobject                     jobject_static_call         ( jclass, const Parameter_list& = Parameter_list::empty ) const;
};

//--------------------------------------------------------------------------------------Constructor

struct Constructor : Procedure
{
                                Constructor                 ( const Class* cls, const Parameter_list_signature& signature ) : Procedure( cls, "<init>", signature ) {}
                                Constructor                 ( const Class* cls, const char* signature ) : Procedure( cls, "<init>", signature ) {}


    void                        construct                   ( jobject jo, const Parameter_list& parameter_list ) const { call( jo, parameter_list ); }
};

//-------------------------------------------------------------------------------------------Method

struct Static_method : Member
{
                                Static_method               ( const Class*, const string& name, const Parameter_list_signature& );
                                Static_method               ( const Class*, const char* name, const char* signature );

    void                        call                        ( jclass, const Parameter_list& = Parameter_list::empty ) const;
    bool                        bool_call                   ( jclass, const Parameter_list& = Parameter_list::empty ) const;
    jbyte                       byte_call                   ( jclass, const Parameter_list& = Parameter_list::empty ) const;
    wchar_t                     char_call                   ( jclass, const Parameter_list& = Parameter_list::empty ) const;
    short                       short_call                  ( jclass, const Parameter_list& = Parameter_list::empty ) const;
    int                         int_call                    ( jclass, const Parameter_list& = Parameter_list::empty ) const;
    int64                       long_call                   ( jclass, const Parameter_list& = Parameter_list::empty ) const;
    float                       float_call                  ( jclass, const Parameter_list& = Parameter_list::empty ) const;
    double                      double_call                 ( jclass, const Parameter_list& = Parameter_list::empty ) const;
    jobject                     jobject_call                ( jclass, const Parameter_list& = Parameter_list::empty ) const;

                                operator jmethodID          ()                                      { return id(); }
    jmethodID                   id                          () const                                { return _id; }

protected:
    const jmethodID            _id;
};

//--------------------------------------------------------------------------------------------Field

struct Field : Member
{
                                Field                       ( const Class*, const string& name, const Simple_signature& );
                                Field                       ( const Class*, const char* name, const char* signature );

                                operator jfieldID           ()                                      { return id(); }
    jfieldID                    id                          () const                                { return _id; }
    void                        set_long                    (jobject jo, jlong) const;


protected:
    //const Simple_signature     _signature;
    const jfieldID             _id;
};

//---------------------------------------------------------------------------------------Statistics

struct Statistics {
    int                        _NewGlobalRef_count;
    int                        _DeleteGlobalRef_count;
};

//-----------------------------------------------------------------------------------------------Vm
// F�r dieselbe JavaVM k�nnen mehrere VM vorhanden sein, f�r jede DLL eine.
// Neben dem Hauptprogram zurzeit hostole.dll, spidermonkey.dll, hostjava.dll.

struct Vm : Object              // Java virtual machine
{
    struct Option
    {
                                Option                      ( const string& option, void* extra = NULL ) : _option(option), _extra(extra) {}

        string                 _option;
        void*                  _extra;
    };


    struct Instance
    {
                                Instance                    ( const string& class_name, jobject jo ) : _class_name(class_name), _jobject(jo) {}

        string                 _class_name;
        jobject                _jobject;
    };


    struct Standard_classes : Object
    {
                                Standard_classes            ()                                      : _zero_(this+1) {}
                               ~Standard_classes            ();

        void                    load                        ( JNIEnv* );

        Fill_zero              _zero_;
        global_jobject<jclass> _java_lang_object_class;
        global_jobject<jclass> _java_lang_string_class;
        global_jobject<jclass> _java_lang_short_class;
        global_jobject<jclass> _java_lang_integer_class;
        global_jobject<jclass> _java_lang_long_class;
        global_jobject<jclass> _java_lang_float_class;
        global_jobject<jclass> _java_lang_double_class;
        global_jobject<jclass> _java_lang_boolean_class;
        global_jobject<jclass> _java_lang_byte_class;
        global_jobject<jclass> _java_lang_runtimeexception_class;
        global_jobject<jclass> _java_lang_nullpointerexception_class;
        global_jobject<jclass> _java_util_date_class;

        jmethodID              _java_lang_short_constructor_id;
        jmethodID              _java_lang_integer_constructor_id;
        jmethodID              _java_lang_long_constructor_id;
        jmethodID              _java_lang_float_constructor_id;
        jmethodID              _java_lang_double_constructor_id;
        jmethodID              _java_lang_boolean_constructor_id;
        jmethodID              _java_lang_byte_constructor_id;
        jmethodID              _java_util_date_constructor_id;
    };

                                Vm                          ( bool do_start = true );
                                Vm                          ( JavaVM* );                            // Vorhandene JVM verwenden (f�r natives Module)
                               ~Vm                          ();


    void                    set_debug                       ( bool d )                              { _debug = d; }
    bool                        debug                       ()                                      { return _debug; }

    bool                        running                     () const                                { return _vm != NULL; }

    void                        new_instances               ( const string& class_names );          // "class:class:..."
    void                        load_standard_classes       ();
    Standard_classes*           standard_classes            ();
    void                    set_log                         ( Has_log* );
    void                    set_filename                    ();
    void                    set_filename                    ( const string& module_filename )       { _filename = module_filename; }
    string                      filename                    () const                                { return _filename; }

    void                    set_class_path                  ( const string& class_path )            { _class_path = class_path; }
    string                      class_path                  ()                                      { expand_class_path(); return _complete_class_path; }
    void                        append_class_path           ( const string& path )                  { _class_path += Z_PATH_SEPARATOR; _class_path += path; }
    void                        prepend_class_path          ( const string& path )                  { _class_path = path + Z_PATH_SEPARATOR + _class_path; }
    void                        expand_class_path           ();

    void                    set_javac_filename              ( const string& filename )              { _javac_filename = filename; }
    string                      javac_filename              () const                                { return _javac_filename; }

    void                    set_work_dir                    ( const string& directory )             { _work_dir = directory; }
    string                      work_dir                    ()                                      { return _work_dir; }

    void                    set_destroy_vm                  ( bool );

    void                    set_sos_initialized             ( bool b = true )                       { _sos_initialized = b; }       // F�r kram/sos_java.h
    bool                        sos_initialized             () const                                { return _sos_initialized; }

    void                        init                        ();
    void                        start                       ();
    void                        close                       ();
    void                        log_version                 ( JNIEnv*, const string& module_filename );
    void                    set_options                     ( const string& );
    string                      options                     () const                                { return _options_string; }

    JNIEnv*                     attach_thread               ( const string& thread_name );
    void                        detach_thread               ();

    JavaVM*                     vm                          ()                                      { return _vm; }

    JNIEnv*                     jni_env                     ();

    void                        check_for_exception         ();

    static bool                 is_active                   ()                                      { return static_vm != NULL; }
    static ptr<Vm>          get_vm                          ( bool start = true );
    static JavaVM*              request_jvm                 ();
    static void                 release_jvm                 ( JavaVM* );

    static Mutex                static_vm_mutex;
    static Vm*                  static_vm;


    Fill_zero                  _zero_;
    //Mutex                      _jni_switch_mutex;
    //Mutex_guard                _jni_switch_mutex_guard;     // Synchronisiert den nebenl�ufigen Aufruf aus Java
    bool                       _foreign;                    // JVM ist von au�en (f�r Hostjava)
    bool                       _vm_requested;
    bool                       _dont_destroy;               // DestroyJavaVM() nicht rufen (h�ngt manchmal)
    bool                       _debug;
    bool                       _debug_options_set;
    bool                       _sos_initialized;
    Delegated_log              _log;
    JavaVMInitArgs             _vm_args;
    std::vector<Option>        _options;
    string                     _options_string;             // JS-540
    JavaVM*                    _vm;
  //Thread_data<Java_thread_data> _thread_data;

    string                     _filename;                   // Dateiname der Java-VM
    string                     _class_path;
    string                     _last_expanded_class_path;   // F�r expand_class_path()
    string                     _complete_class_path;
    string                     _javac_filename;
    string                     _work_dir;
    string                     _new_instances;

    typedef std::vector<Instance> Instances;
    Instances                  _instances;

    ptr<Standard_classes>      _standard_classes;
    Class_register             _class_register;

    Statistics                 _statistics;
    ptr<Jobject_debug_register> _jobject_debug_register;

    static Cached_log_category  java_log_category;
};

//-------------------------------------------------------------------------------------------------

inline Vm*                      Env::get_vm                 ()                                      { return Vm::static_vm; }

//-------------------------------------------------------------------------------------------------

} //namespace javabridge
} //namespace zschimmer


#endif
