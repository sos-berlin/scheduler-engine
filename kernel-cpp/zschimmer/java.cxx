// $Id: java.cxx 14145 2010-12-02 12:11:47Z jz $

#include "zschimmer.h"
#include "threads.h"
#include "z_com.h"
#include "z_directory.h"
#include "file.h"
#include "Memory_allocator.h"
#include "java.h"

#ifdef Z_WINDOWS
#   include "z_windows_registry.h"          // Der Pfad der jvm.dll steht in der Registry
#endif

#ifdef Z_UNIX
#   include <dlfcn.h>
#endif

#ifdef Z_HPUX
#   include <dl.h>
#endif

#ifdef Z_AIX
#   define JVALUES_CAST(JVALUE) const_cast<jvalue*>(JVALUE)
#else
#   define JVALUES_CAST(JVALUE) (JVALUE)
#endif

using namespace std;
using namespace zschimmer::file;

namespace zschimmer {
namespace javabridge {


//--------------------------------------------------------------------------------------------const

static Message_code_text error_codes[] =
{
    { "Z-JAVA-100", "Java Virtual Machine cannot be loaded" },
    { "Z-JAVA-101", "Java Virtual Machine is not started" },
    { "Z-JAVA-102", "Error '$1' occurred in Java at: $2" },
    { "Z-JAVA-103", "Missing entry [java] vm= in file sos.ini" },
    { "Z-JAVA-104", "COM variant type $1 cannot be converted to any Java type" },
    { "Z-JAVA-105", "Java exception $1(\"$2\"), method=$3" },
    { "Z-JAVA-106", "Java reference is NULL (\"NullPointerException\")" },
    { "Z-JAVA-107", "Type '$1' expected, not '$2'" },
    { "Z-JAVA-109", "set_jvm: A Java Virtual Machine has alreade been started" },
    { "Z-JAVA-110", "Java Virtual Machine is older then version 1.2" },
    { "Z-JAVA-111", "Reference to C++ object is not longer valid" },
    { "Z-JAVA-112", "The $1. parameter is of unknown type" },
    { "Z-JAVA-113", "Name is too long: $1" },
    { "Z-JAVA-114", "Expecting type $1" },
    { "Z-JAVA-115", "No java class signature string: '$1'" },
    { "Z-JAVA-116", "No java type character: '$1'" },
    { "Z-JAVA-117", "No java class name: '$1'" },
    { "Z-JAVA-118", "Invalid java cast to '$1' from '$2'" },
    { "Z-JAVA-119", "(Internal) Same class defined twice with ClassFactory: '$1'" },
    {}
};

//-------------------------------------------------------------------------------------------static

Vm*                             Vm::static_vm         = NULL;
Cached_log_category             Vm::java_log_category ( "java" );
static Thread_data<Java_thread_data> thread_data;

//-------------------------------------------------------------------------------------------Z_INIT

Z_INIT( java )
{
    add_message_code_texts( error_codes );
}

//------------------------------------------------------------------------------------------jni_env

JNIEnv* jni_env()
{
    JNIEnv* jenv = thread_data->_jni_env;
    
    if( !jenv )
    {
        if( !Vm::static_vm )  z::throw_xc( Z_FUNCTION, "No Java, static_vm==NULL" );

        jenv = Vm::static_vm->jni_env();
        thread_data->_jni_env = jenv; 
    }

    return jenv;
}

//---------------------------------------------------------------------------------expand_wildcards

static vector<string> expand_wildcards( const string& pattern )
{
    vector<string> result;

    result.reserve( 100 );


#   ifdef Z_WINDOWS

        string                           directory_path = directory_of_path( pattern );
        windows::Simple_directory_reader dir;

        dir.open( directory_path, windows::Simple_directory_reader::no_flag, filename_of_path( pattern ) );

        while(1)
        {
            string filename = dir.get();
            if( filename == "" )  break;
            if( directory_path != "" )  filename = directory_path + Z_DIR_SEPARATOR + filename;
            result.push_back( filename );
        }

        dir.close();

        sort( result.begin(), result.end() );

        return result;

#    else

        int glob_flags = 0;
#       ifdef Z_LINUX
            glob_flags |= GLOB_BRACE;   // { und } generieren Pfade
#       endif

        posix::Glob posix_glob( pattern, glob_flags );
        
        if( const char* const* f = posix_glob.files() )
        {
            while( *f )  result.push_back( *f++ );
        }
        
        return result;

#   endif
}

//--------------------------------------------------------------------------------expand_class_path

string expand_class_path( const string& class_path_with_wildcards )
{
    vector<string> class_path_list = vector_split( Z_PATH_SEPARATOR, class_path_with_wildcards );
    S              new_class_path;

    Z_FOR_EACH( vector<string>, class_path_list, p )
    {
        string path = *p;

        if( path != "" )
        {
            string filename = filename_of_path( *p );
            
            if( filename.find( '*' ) != string::npos  
             || filename.find( '?' ) != string::npos 
#           ifdef Z_UNIX
             || filename.find( '[' ) != string::npos 
             || filename.find( ']' ) != string::npos 
             || filename.find( '{' ) != string::npos 
             || filename.find( '}' ) != string::npos 
           //|| string_begins_with( filename, "~" )
#           endif
            )
            {
                try
                {
                    vector<string>  name_vector = expand_wildcards( path );

                    Z_FOR_EACH( vector<string>, name_vector, n )  new_class_path << Z_PATH_SEPARATOR << *n;
                }
                catch( exception& x )
                {
                    Z_LOG2( Vm::java_log_category, "classpath " << path << ": " << x.what() << '\n' );
                    //new_class_path << Z_PATH_SEPARATOR << path;  // Wir lassen den Joker stehen.
                }
            }
            else
                new_class_path << Z_PATH_SEPARATOR << path;
        }
        else; // Leere Einträge ignorieren
    }

    string result = new_class_path;
    if( result.length() > 0 )  result.erase( 0, 1 );
    return result;
}

//-------------------------------------------------------------------------------javavm_from_jnienv

JavaVM* javavm_from_jnienv( JNIEnv* jenv )
{
    JavaVM* result = NULL; 
    jenv->GetJavaVM( &result ); 
    return result;
}

//---------------------------------------------------------------------------------------Vm::get_vm

ptr<Vm> Vm::get_vm( bool start )
{
    ptr<Vm> vm;

    Z_MUTEX( static_vm_mutex )
    {
        if( static_vm )
        {
            vm = static_vm;
        }
        else
        {
            vm = Z_NEW( Vm( false ) );
        }

        if( start )  vm->start();
    }

    return vm;
}

//----------------------------------------------------------------------------------Vm::set_options

void Vm::set_options( const string& options )
{
    _options_string = options;  // JS-540

    const char* p = options.c_str();
    while( *p == ' ' )  p++;

    while( *p )
    {
        string option;
        const char* blank = strchr( p, ' ' );
        if( !blank )  blank = p + strlen(p);

        const char* q = strchr( p, '=' );

        if( q )  q++;
           else  q = p + strlen(p);

        if( q < blank )
        {
            // key=value, key="value", key='value'

            option = string( p, q - p );

            if( !*q )  break;

            p = q;
            if( *p == '"' || *p == '\'' )
            {
                char quote = *p++;
                while( *p  &&  *p != quote )
                {
                    if( *p == '\\' )  p++;
                    option += *p++;
                }

                if( *p == quote )  p++;
            }
            else
            {
                q = strchr( p, ' ' );
                if( !q )  q = p + strlen(p);
                option.append( p, q - p );
                p = q;
            }
        }
        else 
        {
            option = string( p, blank - p );
            p = blank;
        }

        while( *p == ' ' )  p++;

        _options.push_back( option );
    }
}

//------------------------------------------------------------------------------------java_vfprintf

static jint JNICALL java_vfprintf( FILE*, const char *format, va_list args )
{
  //static string   line;
    char            buf[1024+1];
    int             ret;

#   ifdef Z_WINDOWS
        ret = _vsnprintf_s( buf, sizeof buf - 1, _TRUNCATE, format, args );
#   else
        memset( buf, 0, sizeof buf );
        ret = _vsnprintf( buf, sizeof buf - 1, format, args );
        buf[ sizeof buf - 1 ] = '\0';
#   endif

    if( Vm::static_vm ) 
    {
        if( strcmp( buf, "\n" ) == 0 )  return ret;     // log macht sowieso einen Zeilenwechsel, das hier gäbe also nur eine Leerzeile

        if( Vm::static_vm->_log.log_level() <= log_debug3 )  Vm::static_vm->_log.debug3( buf );
                                                       else  Z_LOG2( Vm::java_log_category, buf );
    }
    else
    {
        size_t len = strlen( buf );
        if( len > 0  &&  buf[ len - 1 ] != '\n' )  buf[ len ] = '\n', buf[ len+1 ] = '\0';
        Z_LOG2( Vm::java_log_category, buf );
        fputs( buf, stderr );
    }

    return ret;
}

//-------------------------------------------------------------------------------------------Vm::Vm

Vm::Vm( bool do_start )
: 
    _zero_(this+1)
{ 
    init();

    if( do_start )  start(); 
}

//-------------------------------------------------------------------------------------------Vm::Vm

Vm::Vm( JavaVM* jvm )
: 
    _zero_(this+1),
    _foreign(true)
{ 
    init();

    _vm = jvm;
    _foreign = true;

    void* jenv = NULL;
    int ret = _vm->GetEnv( &jenv, JNI_VERSION_1_2 );
    if( ret != JNI_OK )  throw_xc( "Z-JAVA-110" );

    //log_version( jenv, "" );
}

//------------------------------------------------------------------------------------------Vm::~Vm

Vm::~Vm()
{
    _standard_classes = NULL;

    try
    {
        //Java bleibt bei Prozessende hängen  close(); 
    }
    catch( const exception& x ) { Z_LOG( Z_FUNCTION << ": " << x << "\n" ); }

    _options.clear();
    delete _vm_args.options;  _vm_args.options = NULL;
}

//-----------------------------------------------------------------------------------------Vm::init

void Vm::init()
{
    Z_MUTEX( static_vm_mutex )
    {
        if( static_vm )  throw_xc( "Z-JAVA-109" );
        static_vm = this;
    }

    _log.set_prefix( "Java" );
    _log.set_log_level( log_warn );    // vor set_log() geht die Ausgabe nach stderr. Dorthin geben wir nur Warnungen aus.
}

//--------------------------------------------------------------------------------------Vm::set_log

void Vm::set_log( Has_log* log )
{ 
    _log.set_log_level( log_unknown );
    _log.set_log( log ); 
}

//----------------------------------------------------------------------------------Vm::log_version

void Vm::log_version( JNIEnv* jenv, const string& module_filename )
{
    int version = jenv->GetVersion();

    S line;
    line << "Java Native Interface (JNI) " << ( version >> 16 ) << "." << ( version & 0xFFFF ) << ", loaded from " << module_filename;

    _log.debug3( line );
    Z_LOG( line << "\n" );
}

//-------------------------------------------------------------------------------------set_filename

void Vm::set_filename()
{
#   ifdef Z_WINDOWS

        windows::Registry_key hkey;
        windows::Registry_key version_hkey;

        if( hkey.try_open( HKEY_LOCAL_MACHINE, "software\\JavaSoft\\Java Runtime Environment", KEY_QUERY_VALUE ) )
        {
            string current_version = hkey.get_string( "CurrentVersion", "" );
            if( current_version != "" 
             && version_hkey.try_open( hkey, current_version, KEY_QUERY_VALUE ) )
            {
                _filename = version_hkey.get_string( "RuntimeLib", "" );
            }
        }

        if( _filename == "" )  _filename = "jvm.dll";

#   elif defined Z_HPUX_PARISC

        _filename = "libjvm.sl";

#   else

        _filename = "libjvm.so";

#   endif
}

//----------------------------------------------------------------------------------------Vm::start
// Im Haupt-Thread zu rufen.

void Vm::start()
{
    if( _javac_filename == "" )  _javac_filename = "javac";

    Z_MUTEX( static_vm_mutex )
    {
        if( !_vm )
        {
            if( com::static_com_context_ptr )
            {
                _vm = request_jvm();
                _foreign = true;
                _vm_requested = true;
            }
/*
            else
            if( static_foreign_jvm )        // veraltet (noch für hostjava -> hostole)
            {
                _vm = static_foreign_jvm;
                _foreign = true;
            }
*/
        }
    }

    if( _vm )  
    {
        void* jenv = NULL;
        int ret = _vm->GetEnv( &jenv, JNI_VERSION_1_2 );
        if( ret != JNI_OK )  throw_xc( "Z-JAVA-110" );

        return;
    }


    int ret;

    if( _debug )  set_log_category( "java", true );
            else  _debug = log_category_is_set( "java" );

    if (_debug || Memory_allocator::debug_memory())
        _jobject_debug_register = Z_NEW(Jobject_debug_register);

    /// JAVA-MODUL LADEN

    if( _filename == "" )  set_filename();

    string module_filename = _filename;

    typedef int JNICALL JNI_CreateJavaVM_func( JavaVM**, JNIEnv**, JavaVMInitArgs* );
    JNI_CreateJavaVM_func* JNI_CreateJavaVM;

#   ifdef Z_WINDOWS
    {    
        Z_LOG2( Vm::java_log_category, "LoadLibrary " << module_filename << '\n' );
        HINSTANCE vm_module = LoadLibrary( module_filename.c_str() );
        if( !vm_module )  throw_mswin( "LoadLibrary", "Java Virtual Machine " + module_filename );

        //module_filename = filename_of_hinstance( vm_module );
        //Z_LOG2( Vm::java_log_category, "HINSTANCE=" << (void*)vm_module << "  " << module_filename << "  " << file_version_info( module_filename ) << '\n' );

        JNI_CreateJavaVM = (JNI_CreateJavaVM_func*)GetProcAddress( vm_module, "JNI_CreateJavaVM" );
        if( !JNI_CreateJavaVM )  throw_mswin( "GetProcAddress", "JNI_CreateJavaVM" );
    }
#   else
    {
        Z_LOG2( Vm::java_log_category, "LD_LIBRARY_PATH=" << get_environment_variable( "LD_LIBRARY_PATH" ) << "\n" 
#           ifdef Z_HPUX
               "SHLIB_PATH="      << get_environment_variable( "SHLIB_PATH" ) << "\n" 
               "LD_PRELOAD="      << get_environment_variable( "LD_PRELOAD" ) << "\n" 
#           endif
#           ifdef Z_AIX
               "LIBPATH="         << get_environment_variable( "LIBPATH"    ) << "\n" 
#           endif
        );

#       if defined Z_HPUX_IA64
            Z_LOG2( Vm::java_log_category, "dlopen " << module_filename << ",RTLD_LAZY|RTLD_GLOBAL\n" );
            void *vm_module = dlopen( module_filename.c_str(), RTLD_LAZY | RTLD_GLOBAL );
#        else
            Z_LOG2( Vm::java_log_category, "dlopen " << module_filename << ",RTLD_LAZY\n" );
            void *vm_module = dlopen( module_filename.c_str(), RTLD_LAZY );

#           ifdef Z_AIX
                if( !vm_module  &&  string_ends_with( module_filename, ".so" ) )        // Dateinamensendung .a probieren
                {
                    Z_LOG2( Vm::java_log_category, dlerror() << "\n" );

                    module_filename.replace( module_filename.length() - 3, 3, ".a" );
                    Z_LOG2( Vm::java_log_category, "dlopen " << module_filename << ",RTLD_LAZY\n" );

                    vm_module = dlopen( module_filename.c_str(), RTLD_LAZY );
                }
#           endif
#       endif

        if( !vm_module )  throw_xc( "Z-JAVA-100", dlerror(), module_filename );

        JNI_CreateJavaVM = (JNI_CreateJavaVM_func*)dlsym( vm_module, "JNI_CreateJavaVM" );
        if( !JNI_CreateJavaVM )  throw_xc( "Z-JAVA-100", dlerror(), "JNI_CreateJavaVM" );
    }
#   endif



    expand_class_path();


    _vm_args.version = JNI_VERSION_1_4;
    _vm_args.ignoreUnrecognized = true;

    // _options.push_back( "-Xfuture" );   
    // Performs strict class-file format checks.  For purposes of backwards compatibility, the default format checks performed by the Java 2 SDK's virtual machine  are
    // no  stricter  than the checks performed by 1.1.x versions of the JDK software.  The -Xfuture flag turns on stricter class-file format checks that enforce closer
    // conformance to the class-file format specification.  Developers are encouraged to use this flag when developing new code because the stricter checks will become
    // the default in future releases of the Java application launcher.
    

    if( _debug ) 
    {
        _options.push_back( Option( "-Xdebug"               ) );

        bool check_jni_exists = false;
        for( uint i = 0; i < _options.size(); i++ )  check_jni_exists |= string_begins_with( _options[i]._option, "-Xcheck:jni" );
        if( !check_jni_exists )  _options.push_back( Option( "-Xcheck:jni "          ) );    // Nicht unter AIX, damit -Xcheck:jni:pedantic aus sos.ini wirkt

#       ifndef Z_AIX
            _options.push_back( Option( "-showversion"          ) );
            _options.push_back( Option( "-verbose:class,gc,jni" ) );
#       endif
    }

    _options.push_back( Option( "-Xfuture"        ) );  // Perform strict class-file format checks.
    _options.push_back( Option( "-XX:+UseAltSigs" ) );  // Statt SIGUSR1 und SIGUSR2 soll Java andere Signale (welche?) benutzen.
  //_options.push_back( Option( "-Xnocatch"       ) );  // disable the catch all signal handler

//#   ifdef Z_UNIX
    _options.push_back( Option( "-Xrs"            ) );  // signal masks for SIGINT, SIGTERM, SIGHUP, and SIGQUIT are not changed by the JVM, and signal handlers for these signals are not installed
    /*
        -Xrs 
        Reduces use of operating-system signals by the Java virtual machine (JVM). 
        In a previous release, the Shutdown Hooks facility was added to allow orderly shutdown of a Java application.
        The intent was to allow user cleanup code (such as closing database connections) to run at shutdown, even if the JVM terminates abruptly. 

        Sun's JVM catches signals to implement shutdown hooks for abnormal JVM termination. 
        The JVM uses SIGHUP, SIGINT, and SIGTERM to initiate the running of shutdown hooks. 

        The JVM uses a similar mechanism to implement the pre-1.2 feature of dumping thread stacks for debugging purposes. 
        Sun's JVM uses SIGQUIT to perform thread dumps. 

        Applications embedding the JVM frequently need to trap signals like SIGINT or SIGTERM, which can lead to interference 
        with the JVM's own signal handlers. The -Xrs command-line option is available to address this issue. When -Xrs is used on 
        Sun's JVM, the signal masks for SIGINT, SIGTERM, SIGHUP, and SIGQUIT are not changed by the JVM, and signal handlers for
        these signals are not installed. 

        There are two consequences of specifying -Xrs: 

        SIGQUIT thread dumps are not available. 
        User code is responsible for causing shutdown hooks to run, for example by calling System.exit() when the JVM is to be terminated.
    */
//#   endif

    _options.push_back( Option( "vfprintf", (void*)java_vfprintf ) );
    _options.push_back( "-Djava.class.path=" + _complete_class_path );

    _vm_args.nOptions = (int)_options.size();
    delete _vm_args.options;
    _vm_args.options = new JavaVMOption[ _vm_args.nOptions ];

    {
        Log_ptr log ( "java" );
        log << "Java options: ";
        for( int i = 0; i < _vm_args.nOptions; i++ )
        {
            _vm_args.options[i].optionString = (char*)_options[i]._option.c_str();
            _vm_args.options[i].extraInfo    =        _options[i]._extra;
            log << _vm_args.options[i].optionString << "  ";
        }
        log << "\n";
    }

    Z_LOG2( Vm::java_log_category, "JNI_CreateJavaVM()\n" );
    JNIEnv* jenv = NULL;
    ret = JNI_CreateJavaVM( &_vm, &jenv, &_vm_args );

    _options.clear();

    //if( static_vm == this )  set_jvm( _vm );

    Z_LOG2( Vm::java_log_category, "setlocale(LC_ALL,\"C\")\n" );
    const char* java_locale = setlocale( LC_ALL, "C" );
    if( strcmp( java_locale, "C" ) != 0 )  Z_LOG2( Vm::java_log_category, "Javas locale war " << java_locale << "\n" );

    if( ret < 0 )  throw_java_ret( ret, "JNI_CreateJavaVM", module_filename );

    log_version( jenv, module_filename );

    new_instances( _new_instances );

    load_standard_classes();           // Wirkt natürlich nur für dieses Vm. Es kann in einer DLL aber noch eines geben, deshalb standard_classes()
}

//----------------------------------------------------------------------------------------Vm::close

void Vm::close()
{
    _standard_classes = NULL;

    if( _vm )  
    {
        if( _foreign )
        {
            if( _vm_requested )  release_jvm( _vm );
            _vm = NULL;
            Z_MUTEX( static_vm_mutex )  static_vm = NULL;
        }
        else
        {
            if( _dont_destroy  ||  unloading_module )
            {
                Z_LOG2( Vm::java_log_category, "DestroyJavaVM() unterdrückt\n" );
                //_vm = NULL;
            }
            else
            {
                Env env = jni_env();

                Z_FOR_EACH( Instances, _instances, i )  if( i->_jobject )  env.delete_global_ref(i->_jobject);
                _instances.clear();

                Z_LOG2( Vm::java_log_category, "DestroyJavaVM() ...\n" );

                int ret = _vm->DestroyJavaVM();
                Z_MUTEX( static_vm_mutex )  static_vm = NULL;

                Z_LOG2( Vm::java_log_category, "DestroyJavaVM()  OK\n" );            

                if( ret < 0 )  _log.error( "DestroyJavaVM() liefert " + as_string(ret) + ". Java lässt sich nicht entladen" );
                         else  _vm = NULL;
            }

            //if( static_vm == this )  set_jvm( _vm );
        }
    }
}

//-------------------------------------------------------------------------------Vm::set_destroy_vm

void Vm::set_destroy_vm( bool destroy )
{ 
    if( !destroy  &&  !_dont_destroy )  AddRef();       // Nie freigeben, static_vm bleibt erhalten für erneute Verwendung
    if( destroy   &&   _dont_destroy )  Release();      // AddRef() wieder aufheben

    _dont_destroy = !destroy; 
}

//----------------------------------------------------------------------------Vm::expand_class_path

void Vm::expand_class_path()
{
    if( _class_path == _last_expanded_class_path )  return;

    if( _class_path != "" )  _complete_class_path += _class_path + Z_PATH_SEPARATOR;
    //_complete_class_path = replace_regex( _complete_class_path, Z_PATH_SEPARATOR "$", "" );
    //_complete_class_path = replace_regex( _complete_class_path, Z_PATH_SEPARATOR "+", Z_PATH_SEPARATOR );

    _complete_class_path = javabridge::expand_class_path( _complete_class_path );
    _last_expanded_class_path = _class_path;
}

//--------------------------------------------------------------------------------Vm::new_instances

void Vm::new_instances( const string& class_names_str )
{
    if( class_names_str.empty() )  return;

    if( !_vm )
    {
        _new_instances += " ";
        _new_instances += class_names_str;
        return;
    }


    Env            env             = jni_env();
    Class          java_lang_class ( "java/lang/Class" );
    vector<string> class_names     = zschimmer::vector_split( " *[ :;,] *", class_names_str );

    for( vector<string>::iterator i = class_names.begin(); i != class_names.end(); i++ )
    {
        string class_name = *i;

        if( !class_name.empty() )
        {
            bool old = false;
            Z_FOR_EACH( Instances, _instances, j )  if( class_name == j->_class_name )  { old = true; break; }
            if( !old )
            {
                try
                {
                    Z_LOG2( Vm::java_log_category, "Class(\"" << class_name << "\").newInstance()\n" );

                    Class c ( class_name ); 
                    Local_jobject obj ( env->CallObjectMethod( c, java_lang_class.method_id( "newInstance", "()Ljava/lang/Object;" ) ) );
                    if( env->ExceptionCheck() )  env.throw_java( "CallObjectMethod" );
                    if( !obj )  env.throw_java( "newInstance", class_name );

                    _instances.push_back( Instance( class_name, env.new_global_ref(obj) ) );
                }
                catch( const exception& x ) 
                { 
                    _log.warn( "Java class " + class_name + ": " + x.what() );
                }
            }
        }
    }
}

//-----------------------------------------------------------------------Vm::Standard_classes::load

void Vm::Standard_classes::load( JNIEnv* jenv )
{
    Env env ( jenv );
    _java_lang_string_class     = env.find_class( "java/lang/String"  );
    _java_lang_short_class      = env.find_class( "java/lang/Short"   );
    _java_lang_integer_class    = env.find_class( "java/lang/Integer" );
    _java_lang_long_class       = env.find_class( "java/lang/Long"    );
    _java_lang_float_class      = env.find_class( "java/lang/Float"   );
    _java_lang_double_class     = env.find_class( "java/lang/Double"  );
    _java_lang_boolean_class    = env.find_class( "java/lang/Boolean" );
    _java_lang_byte_class       = env.find_class( "java/lang/Byte"    );
    _java_util_date_class       = env.find_class( "java/util/Date"    );

    _java_lang_runtimeexception_class     = env.find_class( "java/lang/RuntimeException" );
    _java_lang_nullpointerexception_class = env.find_class( "java/lang/NullPointerException" );


    _java_lang_short_constructor_id   = env->GetMethodID( _java_lang_short_class  , "<init>", "(S)V" );
    _java_lang_integer_constructor_id = env->GetMethodID( _java_lang_integer_class, "<init>", "(I)V" );
    _java_lang_long_constructor_id    = env->GetMethodID( _java_lang_long_class   , "<init>", "(J)V" );
    _java_lang_float_constructor_id   = env->GetMethodID( _java_lang_float_class  , "<init>", "(F)V" );
    _java_lang_double_constructor_id  = env->GetMethodID( _java_lang_double_class , "<init>", "(D)V" );
    _java_lang_boolean_constructor_id = env->GetMethodID( _java_lang_boolean_class, "<init>", "(Z)V" );
    _java_lang_byte_constructor_id    = env->GetMethodID( _java_lang_byte_class   , "<init>", "(B)V" );
    _java_util_date_constructor_id    = env->GetMethodID( _java_util_date_class   , "<init>", "(J)V" );
}

//-----------------------------------------------------------Vm::Standard_classes::Standard_classes

Vm::Standard_classes::~Standard_classes()
{
    Z_LOG2( "zschimmer", Z_FUNCTION );
}

//------------------------------------------------------------------------Vm::load_standard_classes

void Vm::load_standard_classes()
{
    _standard_classes = Z_NEW( Standard_classes );
    _standard_classes->load( jni_env() );
}

//-----------------------------------------------------------------------------Vm::standard_classes

Vm::Standard_classes* Vm::standard_classes()
{
    if( !_standard_classes )  load_standard_classes();
    return _standard_classes;
}

//--------------------------------------------------------------------------------Vm::attach_thread

JNIEnv* Vm::attach_thread( const string& thread_name )
{
    if( !_vm ) 
        throw_xc( "Z-JAVA-101" );


    JavaVMAttachArgs args;
    void*            jenv = NULL;

    memset( &args, 0, sizeof args );
    args.version = JNI_VERSION_1_2;
    args.name    = (char*)thread_name.c_str();

    Z_LOG2( Vm::java_log_category, "AttachCurrentThread()\n" );

    int ret = _vm->AttachCurrentThread( &jenv, &args ); 
    if( ret < 0 )  throw_java_ret( ret, "AttachCurrentThread" );

    return static_cast<JNIEnv*>( jenv );
}

//--------------------------------------------------------------------------------Vm::detach_thread

void Vm::detach_thread()
{
    void* jenv = NULL;

    if( _vm  &&  _vm->GetEnv( &jenv, JNI_VERSION_1_2 ) == JNI_OK )
    {
        Z_LOG2( Vm::java_log_category, "DetachCurrentThread()\n" );

        int ret = _vm->DetachCurrentThread(); 
        if( ret < 0 )  throw_java_ret( ret, "DetachCurrentThread" );
    }
}

//--------------------------------------------------------------------------------------Vm::jni_env

JNIEnv* Vm::jni_env() 
{
    if( !_vm )  
    {
        throw_xc( "Z-JAVA-101" );   
    }

    void* jenv = NULL;

    if( _vm->GetEnv( &jenv, JNI_VERSION_1_2 ) == JNI_OK )  return static_cast<JNIEnv*>( jenv );
                                                     else  return attach_thread( "" );
}

//-----------------------------------------------------------------------------------------Env::Env

Env::Env( Vm* vm )
:
    _jni_env( vm->jni_env() )
{
}

//-----------------------------------------------------------------------------------------Env::Env
    
Env::Env() 
: 
    _jni_env( javabridge::jni_env() ) 
{
}

//-------------------------------------------------------------------------Env::string_from_jstring

string Env::string_from_jstring( const jstring& jstr )
{
    string result;

    if( jstr )
    {
        JNIEnv* jenv = jni_env();

        int length = jenv->GetStringLength( jstr );

        {
            Read_jstring_critical r ( jstr );     // Ab jetzt kein JNI-Aufruf!
            result = com::string_from_ole( (const OLECHAR*)r._jchars, length );
        }
    }

    return result;
}

//-------------------------------------------------------------------------Env::jstring_from_string

jstring Env::jstring_from_string( const char* str )
{ 
    return jstring_from_string( str, strlen(str) ); 
}

//-------------------------------------------------------------------------Env::jstring_from_string

jstring Env::jstring_from_string( const string& str )  
{ 
    return jstring_from_string( str.data(), str.length() ); 
}

//-------------------------------------------------------------------------Env::jstring_from_string

jstring Env::jstring_from_string( const char* str, size_t length )
{
    JNIEnv*   jenv = jni_env();
    com::Bstr bstr ( str, length );

    return jenv->NewString( (const jchar*)(const OLECHAR*)bstr, bstr.length() );
}

//--------------------------------------------------------------------------------Env::result_is_ok

bool Env::result_is_ok(jobject result) 
{
    bool ok = result != NULL  ||  !jni_env()->ExceptionCheck();
    assert(ok == !jni_env()->ExceptionCheck());
    return ok;
}

//--------------------------------------------------------------------------------Env::check_result

void Env::check_result(jobject result, const char* static_debug_text) 
{
    if (!result_is_ok(result))  throw_java(static_debug_text);
}

//--------------------------------------------------------------------------Env::set_java_exception

void Env::set_java_exception( const char* what )
{
    JNIEnv* jenv = jni_env();

    if( Vm::static_vm )
    {
        int err = jenv->ThrowNew( Vm::static_vm->standard_classes()->_java_lang_runtimeexception_class, what ); 
        if( err == 0 )  return;
    }

    string w = what + string(" (Vm::static_vm==NULL) ");

    jenv->ExceptionDescribe();      // schreibt nach stderr
    jenv->FatalError( w.c_str() );
}

//--------------------------------------------------------------------------Env::set_java_exception

void Env::set_java_exception( const exception& x )
{
    set_java_exception( x.what() );
}

//--------------------------------------------------------------------------Env::set_java_exception

void Env::set_java_exception( const _com_error& x ) 
{
    string what = com::string_from_ole( x.Description() );
    set_java_exception( what.c_str() );
}

//--------------------------------------------------------------------Env::set_NullPointerException

void Env::set_NullPointerException( const string& what )
{
    JNIEnv* jenv = jni_env();

    if( Vm::static_vm )  
    {
        int err = jenv->ThrowNew( Vm::static_vm->standard_classes()->_java_lang_nullpointerexception_class, what.c_str() ); 
        if( err == 0 )  return;
    }

    string w = what + string(" (Vm::static_vm==NULL)");

    jenv->ExceptionDescribe();      // schreibt nach stderr
    jenv->FatalError( w.c_str() );
}

//------------------------------------------------------------------------------Env::get_class_name

string Env::get_class_name( jclass c )
{
    if (!c)  throw_xc("Z-JAVA-106", "get_jclass()");

    jmethodID getClass_method = _jni_env->GetMethodID( c, "getClass", "()Ljava/lang/Class;" );
    if( getClass_method == 0 )  throw_java( "getMethodID()" );

    Local_jobject cls ( _jni_env->CallObjectMethod( c, getClass_method ) );
    check_result(cls, "CallObjectMethod getClass()" );

    Local_jstring result_j = (jstring)_jni_env->CallObjectMethod( c, _jni_env->GetMethodID( cls.get_jclass(), "getName", "()Ljava/lang/String;" ) );
    check_result(result_j, "CallObjectMethod Class.getName()" );
    
    return string_from_jstring( result_j );
}

//-------------------------------------------------------------------------------Env::new_local_ref

jobject Env::new_local_ref(jobject jo) 
{
    jobject result = NULL;
    
    if (jo) {
        result = _jni_env->NewLocalRef(jo);
        if (result == NULL)  throw_java("NewLocalRef");
    }

    return result;
}

//------------------------------------------------------------------------------Env::new_global_ref

jobject Env::new_global_ref(jobject jo)
{
    jobject result = _jni_env->NewGlobalRef( jo );
    check_result(result, "NewGlobalRef");
    if (Vm::static_vm->_jobject_debug_register)  Vm::static_vm->_jobject_debug_register->add(result);
    Vm::static_vm->_statistics._NewGlobalRef_count++;
    Z_LOG2( Vm::java_log_category, "NewGlobalRef() => " << result << "\n" );
    return result;
}

//---------------------------------------------------------------------------Env::delete_global_ref

void Env::delete_global_ref(jobject jo)
{
    Z_LOG2( Vm::java_log_category, "DeleteGlobalRef(" << jo << ")\n" );
    if (Vm::static_vm->_jobject_debug_register)  Vm::static_vm->_jobject_debug_register->remove(jo);
    Vm::static_vm->_statistics._DeleteGlobalRef_count++;
    
    if (_jni_env)  _jni_env->DeleteGlobalRef( jo );
    else Z_LOG("DeleteGlobalRef but _jni_env==NULL\n");
}

//--------------------------------------------------------------------------Env::is_byte_array_class
// Das könnte man optimieren.

bool Env::is_byte_array_class( jobject jo )
{
    JNIEnv* jenv   = jni_env();
    bool    result = false;

    if( jbyteArray jbyte_array = jenv->NewByteArray( 0 ) )
    {
        jclass jbyte_array_jclass = jenv->GetObjectClass( jbyte_array );
        result = jenv->IsInstanceOf( jo, jbyte_array_jclass ) != 0;

        jenv->DeleteLocalRef( jbyte_array_jclass );
        jenv->DeleteLocalRef( jbyte_array );
    }

    return result;
}

//-----------------------------------------------------------------------Env::is_string_array_class
// Das könnte man optimieren.

bool Env::is_string_array_class( jobject jo )
{
    JNIEnv* jenv   = jni_env();
    bool    result = true;

    if( Vm::static_vm )
    {
        if( jobjectArray jstring_array = jenv->NewObjectArray( 0, Vm::static_vm->standard_classes()->_java_lang_string_class, NULL ) )
        {
            jclass jstring_array_jclass = jenv->GetObjectClass( jstring_array );
            result = jenv->IsInstanceOf( jo, jstring_array_jclass ) != 0;

            jenv->DeleteLocalRef( jstring_array );
        }
    }

    return result;
}

//-----------------------------------------------Env::get_spooler_idispatch_class_if_is_instance_of

jclass Env::get_spooler_idispatch_class_if_is_instance_of( jobject jo )
{
    JNIEnv* jenv   = jni_env();
    jclass  result = NULL;

    if( Vm::static_vm )
    {
        jclass cls = jenv->FindClass( "sos/spooler/Idispatch" );

        if( result_is_ok(cls) )
        {
            if( jenv->IsInstanceOf( jo, cls ) )  result = cls;
        }
    }

    return result;
}

//-----------------------------------------------------------------------------------throw_java_ret
                                                                                                   
Z_NORETURN void throw_java_ret( int return_value, const string& text1, const string& text2 )
{
    string ret_text;

    switch( return_value )
    {
        case JNI_OK:        ret_text = "ret=JNI_OK";                                            break;
        case JNI_ERR:       ret_text = "ret=JNI_ERR";                                           break;
        case JNI_EDETACHED: ret_text = "ret=JNI_EDETACHED \"thread detached from the VM\"";     break;
        case JNI_EVERSION:  ret_text = "ret=JNI_EVERSION \"JNI version error\"";                break;

#     ifdef JNI_VERSION_1_4
        case JNI_ENOMEM:    ret_text = "ret=JNI_ENOMEM \"not enough memory\"";                  break;
        case JNI_EEXIST:    ret_text = "ret=JNI_EEXIST \"VM already created\"";                 break;
        case JNI_EINVAL:    ret_text = "ret=JNI_EINVAL \"invalid arguments\"";                  break;
#     endif

        default:            ret_text = "ret=" + as_string(return_value);
    }

    Xc xc ( "Z-JAVA-102" );

    xc.insert( 1, ret_text );
    xc.append_text( text1 );
    xc.append_text( text2 );

    throw_xc( xc );
}

//----------------------------------------------------------------------------------Env::throw_java

void Env::throw_java( const string& text1, const string& text2 )
{
    JNIEnv* jenv = jni_env();

#   if !defined Z_AIX  // VMJNCK028E JNI error in PushLocalFrame: This function cannot be called when an exception is pending
        Local_frame local_frame ( 10 );
#   endif

    Java_exception xc ( "Z-JAVA-105" );
    
    xc._exception_name = "Unbekannt";

    if( jenv )
    {
        jthrowable x = jenv->ExceptionOccurred();
        if( x )
        {
#ifdef Z_DEBUG
            _jni_env->ExceptionDescribe();  // Provisorisch, schreibt nach stderr
#endif

            Z_LOG2( Vm::java_log_category, "Vm::static_vm=" << (void*)Vm::static_vm << "  _debug=" << ( Vm::static_vm? Vm::static_vm->_debug : false ) << "\n" );
            if( Vm::static_vm && Vm::static_vm->_debug )  { Z_LOG2( Vm::java_log_category, "jenv->ExceptionDescribe()\n" ); jenv->ExceptionDescribe(); }     // Ausgabe über java_vfprintf()
            jenv->ExceptionClear();

            jclass c = jenv->GetObjectClass(x);
            if( c ) 
            {
                Local_jobject cls ( jenv->CallObjectMethod( x, jenv->GetMethodID( c, "getClass", "()Ljava/lang/Class;" ) ) );
                if( !jenv->ExceptionCheck() )  
                {
                    Local_jstring name_j ( (jstring)jenv->CallObjectMethod( cls, jenv->GetMethodID( cls.get_jclass(), "getName", "()Ljava/lang/String;" ) ) );
                    if( !jenv->ExceptionCheck() )  
                    {
                        xc._exception_name = string_from_jstring( name_j );

                        while(1)
                        {
                            jthrowable x2 = NULL;
                            jmethodID get_message_id = jenv->GetMethodID( c, "getMessage", "()Ljava/lang/String;" );
                            if( !get_message_id )  break;

                            Local_jstring message_id_j ( (jstring)jenv->CallObjectMethod( x, get_message_id ) );
                            if( !message_id_j || jenv->ExceptionCheck() )  break;
                            
                            xc._message += string_from_jstring( message_id_j );
                          //xc._message += replace_regex_ext( string_from_jstring( Local_jstring( Vm::static_vm, (jstring)jenv->CallObjectMethod( x, get_message_id ) ) ),
                          //                                  "^(.*)\r?\n?$", "\\1" );  // Der Regex greift nicht.

                            jmethodID get_cause_id = jenv->GetMethodID( c, "getCause", "()Ljava/lang/Throwable;" );
                            if( !get_cause_id )  break;

                            x2 = (jthrowable)jenv->CallObjectMethod( x, get_cause_id );
                            if( jenv->ExceptionCheck() )  break;
                            if( !x2 )  break;

                            xc._message += Z_NL;

                            jenv->DeleteLocalRef( x );
                            x = x2;
                        }
                    }
                }

                jenv->DeleteLocalRef( c );
            }

            log_stack_trace(x);
            if( x )  jenv->DeleteLocalRef( x );
        }
    }

    //if( xc._message.empty() )  xc._message = "java";

    xc.insert( 1, xc._exception_name );
    xc.insert( 2, xc._message );
    xc.insert( 3, text1 );
    xc.insert( 4, text2 );

    throw xc;
}

//-----------------------------------------------------------------------------Env::log_stack_trace

void Env::log_stack_trace(jthrowable) 
{
    //StringWriter w = new StringWriter();
    //t.printStackTrace(new PrintWriter(w));
    //String result = w.toString();
    //Z_LOG2(Vm::java_log_category,result);
}

//----------------------------------------------------------------------------------Env::find_class

jclass Env::find_class( const string& name_par )
{
    string name = name_par;

    for( uint i = 0; i < name.length(); i++ )  if( name[i] == '.' )  name[i] = '/';

    Z_LOG2( Vm::java_log_category, "FindClass " << name << '\n' );

    jclass result = _jni_env->FindClass( name.c_str() );
    if( !result_is_ok(result) )  throw_java( "FindClass", name );

    return result;
}

//-------------------------------------------------------------------------------Env::get_method_id

jfieldID Env::get_field_id( jclass cls, const string& name, const string& signature )
{
    jfieldID result;
    
    Z_LOG2( Vm::java_log_category, "GetFieldID(\"" << name << "\")\n" );

    result = _jni_env->GetFieldID( cls, name.c_str(), signature.c_str() );
    if( !result )  throw_java( "GetFieldID", (name + signature).c_str() );

    return result;
}

//-------------------------------------------------------------------------------Env::get_method_id

jmethodID Env::get_method_id( jclass cls, const char* name, const char* signature )
{
    jmethodID result;
    
    Z_LOG2( Vm::java_log_category, "GetMethodID(\"" << name << "\")\n" );

    result = _jni_env->GetMethodID( cls, name, signature );
    if( !result )  throw_java( "GetMethodID", string(name) + signature );

    return result;
}

//-------------------------------------------------------------------------------Env::get_method_id

jmethodID Env::get_static_method_id( jclass cls, const char* name, const char* signature )
{
    jmethodID result;
    
    Z_LOG2( Vm::java_log_category, "GetStaticMethodID(\"" << name << "\")\n" );

    result = _jni_env->GetStaticMethodID( cls, name, signature );
    if( !result )  throw_java( "GetStaticMethodID", string(name) + signature );

    return result;
}

//----------------------------------------------------------------------------Env::get_object_class

//jclass Env::get_object_class( jobject o )
//{
//    jclass result = _jni_env->GetObjectClass( o );
//    if( !result )  throw_java( "GetObjectClass" );
//
//    return result;
//}

//-------------------------------------------------------------------------Local_frame::Local_frame

Local_frame::Local_frame( int capacity ) 
:
    _jni_env( jni_env() )
{ 
    int ret = _jni_env->PushLocalFrame( capacity ); 
    if( ret )  throw_java_ret( ret, "PushLocalFrame" ); 
}

//-------------------------------------------------------------------------------------Class::Class

Class::Class( const char* name )
{ 
    if( name )  load( name ); 
}

//-------------------------------------------------------------------------------------Class::Class

Class::Class( const string& name )
{ 
    load( name ); 
}

//--------------------------------------------------------------------------------------Class::load

void Class::load( const string& name )
{
    Env env = jni_env();

    jclass jcls = env.find_class( name );
    assign( jcls );

    _name = name;
}

//---------------------------------------------------------------------------------Class::method_id

jmethodID Class::method_id( const char* name, const char* signature ) const
{
    Env       env      = jni_env();
    jmethodID result = 0;

    try
    {
        jclass cls = (jclass)get_jobject();
        if( !cls )  
            throw_xc( "Z-JAVA-106", string("Method ") + name + signature );

        result = env.get_method_id( cls, name, signature );
    }
    catch( Xc& x )  { x.append_text( _name );  throw x; }

    return result;
}

//----------------------------------------------------------------------------------Class::field_id

jfieldID Class::field_id( const char* name, const char* signature ) const
{
    Env      env      = jni_env();
    jfieldID result = 0;

    try
    {
        jclass cls = (jclass)get_jobject();
        if( !cls )  
            throw_xc( "Z-JAVA-106", string("Method ") + name + signature );

        result = env.get_field_id( cls, name, signature );
    }
    catch( Xc& x )  { x.append_text( _name );  throw x; }

    return result;
}

//------------------------------------------------------------------------Jobject::static_method_id

jmethodID Class::static_method_id( const char* name, const char* signature ) const
{ 
    Env       env      = jni_env();
    jmethodID result = 0;

    try
    {
        jclass cls = (jclass)get_jobject();
        if( !cls )  
            throw_xc( "Z-JAVA-106", string("Static method ") + name );

        result = env.get_static_method_id( cls, name, signature ); 
    }
    catch( Xc& x )  { x.append_text( _name );  throw x; }

    return result;
}

//------------------------------------------------------------------------------Class::alloc_object

jobject Class::alloc_object() const
{
    Env env;

    Z_LOG2( Vm::java_log_category, "AllocObject()\n" );
    jobject result = env->AllocObject( get_jclass() );
    env.check_result(result, "AllocObject");

    return result;
}

//--------------------------------------------------------------------------------Class::new_object

jobject Class::new_object( const char* signature, ... ) const
{
    jobject result = NULL;

    try
    {
        Env env = jni_env();

        va_list args;
        va_start( args, signature );

            Z_LOG2( Vm::java_log_category, "NewObjectV(" << signature << ") ...\n" );

            result = env->NewObjectV( (jclass)*this, method_id( "<init>", signature ), args );

            Z_LOG2( Vm::java_log_category, "NewObjectV(" << signature << ")  OK\n" );
        
        va_end( args );

        if( !env.result_is_ok(result) )  env.throw_java( "NewObject", _name );
    }
    catch( Xc& x )  { x.append_text( "Class::new_object " + _name + signature );  throw x; }
    
    return result;
}

//--------------------------------------------------------------------------------------Class::name

const string& Class::name()
{
    if( _name == "" )
        _name = Env().get_class_name(get_jclass());

    return _name;
}

//-----------------------------------------------------------------Class::assert_is_assignable_from

void Class::assert_is_assignable_from(jobject jo) const 
{
    if (!is_assignable_from(jo))  
        throw_xc( "Z-JAVA-118", _name, Class::of_object(jo)->name() );
}

//-------------------------------------------------------------------------Class::is_assignable_from

bool Class::is_assignable_from(jobject jo) const 
{
    return jo == NULL  ||  0 != Env()->IsInstanceOf(jo, get_jclass());
}

//----------------------------------------------------------------------------------Class::of_object

ptr<Class> Class::of_object( jobject o )
{
    if( !o )  
        throw_xc( "Z-JAVA-106", Z_FUNCTION );

    Env env = jni_env();
    jclass jClass = env->GetObjectClass( o );
    env.check_result(jClass, "GetObjectClass" );

    return Z_NEW( Class( jClass ) );
}

//------------------------------------------------------------------------Local_jobject::get_jclass

jclass Local_jobject::get_jclass()
{
    if( !_jclass )
    {
        Env env = jni_env();

        if( !_jvalue.l )  
            throw_xc( "Z-JAVA-106", "get_jclass()" );

        _jclass = env->GetObjectClass( _jvalue.l );
        env.check_result(_jclass, "GetObjectClass");
    }

    return _jclass;
}

//-------------------------------------------------------------------------------Jobject::method_id

jmethodID Jobject::method_id( const char* name, const char* signature )
{
    Env       env      = jni_env();
    jmethodID result = 0;

    try
    {
        jclass cls = get_jclass();
        if( !cls )  
            throw_xc( "Z-JAVA-106", string("Method ") + name );

        result = env.get_method_id( cls, name, signature );
    }
    catch( Xc& x )  { x.append_text( string("Jobject::method_id ") + name + signature );  throw x; }

    return result;
}

//------------------------------------------------------------------------Jobject::static_method_id

jmethodID Jobject::static_method_id( const char* name, const char* signature ) 
{ 
    Env       env      = jni_env();
    jmethodID result = 0;

    try
    {
        result = env.get_static_method_id( get_jclass(), name, signature ); 
    }
    catch( Xc& x )  { x.append_text( string("Jobject::static_method_id") + name + signature );  throw x; }

    return result;
}

//------------------------------------------------------------------------Jobject::call_void_method

void Jobject::call_void_method( const char* name, const char* signature, ... )
{
    Env       env      = jni_env();
    jobject   object = get_jobject();
    jmethodID method = method_id( name, signature );

    va_list args;
    va_start( args, signature );

        Z_LOG2( Vm::java_log_category, "CallVoidMethodV(" << name << signature << ") ...\n" );

        env->CallVoidMethodV( object, method, args );

        Z_LOG2( Vm::java_log_category, "CallVoidMethodV(" << name << signature << ")  OK\n" );
    
    va_end( args );

    if( env->ExceptionCheck() )  env.throw_java( name );
}

//----------------------------------------------------------------------Jobject::call_object_method

jobject Jobject::call_object_method( const char* name, const char* signature, ... )
{
    Env       env      = jni_env();
    jobject   object = get_jobject();
    jmethodID method = method_id( name, signature );

    va_list args;
    va_start( args, signature );

        Z_LOG2( Vm::java_log_category, "CallObjectMethodV(" << name << signature << ") ...\n" );

        jobject result = env->CallObjectMethodV( object, method, args );

        Z_LOG2( Vm::java_log_category, "CallObjectMethodV(" << name << signature << ")  OK\n" );
    
    va_end( args );

    if( env->ExceptionCheck() )  env.throw_java( name );

    return result;
}

//----------------------------------------------------------------------Jobject::call_string_method

string Jobject::call_string_method( const char* name, const char* signature, ... )
{
    string    result;
    Env       env    = jni_env();
    jobject   object = get_jobject();
    jmethodID method = method_id( name, signature );

    va_list args;
    va_start( args, signature );

        Z_LOG2( Vm::java_log_category, "CallObjectMethodV(" << name << signature << ") ...\n" );

        Local_jstring jstr = (jstring)env->CallObjectMethodV( object, method, args );

        Z_LOG2( Vm::java_log_category, "CallObjectMethodV(" << name << signature << ")  OK\n" );

        jboolean exception_check = env->ExceptionCheck();

    va_end( args );

    result = env.string_from_jstring( jstr );

    if( exception_check )  env.throw_java( name );

    return result;
}

//----------------------------------------------------------------------Jobject::call_string_method

string Jobject::call_string_method( const jmethodID method_id, ... )
{
    Env     env      = jni_env();
    jobject object = get_jobject();

    va_list args;
    va_start( args, method_id );

        Z_LOG2( Vm::java_log_category, "CallObjectMethodV() ...\n" );

        Local_jstring jstr = (jstring)env->CallObjectMethodV( object, method_id, args );

        Z_LOG2( Vm::java_log_category, "CallObjectMethodV()  OK\n" );

        jboolean exception_check = env->ExceptionCheck();

    va_end( args );

    string result = env.string_from_jstring( jstr );

    if( exception_check )  env.throw_java( "CallObjectMethodV" );
    
    return result;
}

//-------------------------------------------------------------------------Jobject::call_int_method

jint Jobject::call_int_method( const char* name, const char* signature, ... )
{
    Env       env      = jni_env();
    jobject   object = get_jobject();
    jmethodID method = method_id( name, signature );

    va_list args;
    va_start( args, signature );

        Z_LOG2( Vm::java_log_category, "CallIntMethodV(" << name << signature << ") ...\n" );

        jint result = env->CallIntMethodV( object, method, args );

        Z_LOG2( Vm::java_log_category, "CallIntMethodV(" << name << signature << ")  OK\n" );

    va_end( args );

    if( env->ExceptionCheck() )  env.throw_java( name );
    
    return result;
}

//------------------------------------------------------------------------Jobject::call_long_method

jlong Jobject::call_long_method( const char* name, const char* signature, ... )
{
    Env       env      = jni_env();
    jobject   object = get_jobject();
    jmethodID method = method_id( name, signature );

    va_list args;
    va_start( args, signature );

        Z_LOG2( Vm::java_log_category, "CallLongMethodV(" << name << signature << ") ...\n" );

        jlong result = env->CallLongMethodV( object, method, args );

        Z_LOG2( Vm::java_log_category, "CallLongMethodV(" << name << signature << ")  OK\n" );

    va_end( args );

    if( env->ExceptionCheck() )  env.throw_java( name );
    
    return result;
}

//------------------------------------------------------------------------Jobject::call_bool_method

bool Jobject::call_bool_method( const char* name, const char* signature, ... )
{
    Env       env      = jni_env();
    jobject   object = get_jobject();
    jmethodID method = method_id( name, signature );

    va_list args;
    va_start( args, signature );

        Z_LOG2( Vm::java_log_category, "CallBooleanMethodV(" << name << signature << ") ...\n" );

        jboolean result = env->CallBooleanMethodV( object, method, args );

        Z_LOG2( Vm::java_log_category, "CallBooleanMethodV(" << name << signature << ")  OK\n" );

    va_end( args );

    if( env->ExceptionCheck() )  env.throw_java( name );
    
    return result != 0;
}

//------------------------------------------------------------------------Jobject::call_bool_method

bool Jobject::call_bool_method( jmethodID method_id, ... )
{
    Env     env      = jni_env();
    jobject object = get_jobject();

    va_list args;
    va_start( args, method_id );

        Z_LOG2( Vm::java_log_category, "CallBooleanMethodV() ...\n" );

        jboolean result = env->CallBooleanMethodV( object, method_id, args );

        Z_LOG2( Vm::java_log_category, "CallBooleanMethodV()  OK\n" );

    va_end( args );

    if( env->ExceptionCheck() )  env.throw_java( "CallBooleanMethod" );
    
    return result != 0;
}

//---------------------------------------------------------------Jobject::call_static_object_method

jobject Jobject::call_static_object_method( const char* name, const char* signature, ... )
{
    jobject result = NULL;

    try
    {
        Env       env         = jni_env();
        jclass    cls       = get_jclass();
        jmethodID method_id = static_method_id( name, signature );


        va_list args;
        va_start( args, signature );

            Z_LOG2( Vm::java_log_category, "CallStaticObjectMethod(" << name << signature << ") ...\n" );

            result = env->CallStaticObjectMethodV( cls, method_id, args );

            Z_LOG2( Vm::java_log_category, "CallStaticObjectMethod(" << name << signature << ")  OK.\n" );

        va_end( args );

        if( env->ExceptionCheck() )  env.throw_java( name );
    }
    catch( Xc& x )  { x.append_text( string("Jobject::call_static_object_method ") + name + signature );  throw x; }

    return result;
}

//-------------------------------------------------------------------Global_jobject::assign_add_ref

void Global_jobject::assign_add_ref( jobject jo )
{
    if( _jvalue.l != jo ) 
    {
        Env env = jni_env();

        jobject new_jobject = NULL;

        if( jo )
        {
            new_jobject = env.new_global_ref(jo);
            env->DeleteLocalRef(jo);  // jo wird ungültig!
        }

        if( _jvalue.l )
        {
            env.delete_global_ref(_jvalue.l);
            _jvalue.l = NULL;
        }

        _jvalue.l = new_jobject;


        if( _jclass )
        {
            env.delete_global_ref(_jclass);
            _jclass = NULL;
        }
    }
}

//-----------------------------------------------------------------------Global_jobject::get_jclass

jclass Global_jobject::get_jclass()
{
    if( !_jclass )
    {
        if( !_jvalue.l )  throw_xc( "Z-JAVA-106", "get_jclass()" );

        Env env = jni_env();

        local_jobject<jclass> c = env->GetObjectClass( _jvalue.l );
        env.check_result(c, "GetObjectClass" );

        _jclass = static_cast<jclass>( env.new_global_ref(c) );
    }

    return _jclass;
}

//--------------------------------------------------------------------Local_jobject::~Local_jobject

Local_jobject::~Local_jobject()
{ 
    if( _call_close_at_end  &&  get_jobject() )
    {
        try
        {
            call_void_method( "close", "()V" );
        }
        catch( exception& x )
        {
            Z_LOG2( Vm::java_log_category, Z_FUNCTION << ".close() ==> ERROR " << x.what() << '\n' );
        }
    }

    assign( NULL ); 
}

//--------------------------------------------------------------------Local_jobject::assign_add_ref

void Local_jobject::assign_add_ref( jobject jo )
{
    Env env = jni_env();

    jobject local_reference = NULL;
    
    if( jo )
    {
        Z_LOG2( Vm::java_log_category, Z_FUNCTION << " NewLocalRef()\n" );

        local_reference = env->NewLocalRef( jo );
        env.check_result(local_reference, "NewLocalRef" );
    }

    assign( local_reference );
}

//----------------------------------------------------------------------------Local_jobject::assign

void Local_jobject::assign( jobject jo )
{
    Env env = jni_env();

    if( _jclass )
    {
        env->DeleteLocalRef( _jclass );
        _jclass = NULL;
    }

    if( _jvalue.l )
    {
        env->DeleteLocalRef( _jvalue.l );
        _jvalue.l = NULL;
    }

    _jvalue.l = jo;
}

//------------------------------------------------------Read_jstring_critical::Read_jstring_critical

Read_jstring_critical::Read_jstring_critical( jstring jstr )
:
    _jni_env( jni_env() ),
    _jstring(jstr)
{
    jboolean is_copy;

    _jchars = _jni_env->GetStringCritical( _jstring, &is_copy );
    if( !_jchars )  Env(_jni_env).throw_java( "GetStringCritical()" );
}

//----------------------------------------------------Read_jstring_critical::~Read_jstring_critical

Read_jstring_critical::~Read_jstring_critical()
{
    _jni_env->ReleaseStringCritical( _jstring, _jchars );
}

//-----------------------------------------------------------------------------------Member::Member

Member::Member(const Class* cls) 
: 
    _class(cls) 
{ 
}

//-----------------------------------------------------------------------------Procedure::Procedure

Procedure::Procedure( const Class* cls, const string& name, const Parameter_list_signature& signature ) 
:
    Member(cls),
    //_signature(signature),
    _id(cls->method_id(name, signature))
{ 
}

//-----------------------------------------------------------------------------Procedure::Procedure
    
Procedure::Procedure( const Class* cls, const char* name, const char* signature ) 
: 
    Member(cls),
    //_signature(signature),
    _id(cls->method_id(name, signature))
{ 
}

//-------------------------------------------------------------------------------Method::value_call

//Value Method::value_call( jobject jo, const Parameter_list& parameter_list ) const
//{
//    Value result;
//
//    switch( signature().return_simple_type() ) {
//        case t_void: 
//            call( jo, parameter_list ); 
//            break;
//
//        case t_boolean:
//            result.assign_jboolean( bool_call( jo, parameter_list ) );
//            break;
//
//        case t_byte:
//            result.assign_jbyte( byte_call( jo, parameter_list ) );
//            break;
//
//        case t_char:
//            result.assign_jchar( char_call( jo, parameter_list ) );
//            break;
//
//        case t_short:
//            result.assign_jshort( short_call( jo, parameter_list ) );
//            break;
//
//        case t_int:
//            result.assign_jint( int_call( jo, parameter_list ) );
//            break;
//
//        case t_long:
//            result.assign_jlong( long_call( jo, parameter_list ) );
//            break;
//
//        case t_float:
//            result.assign_jfloat( float_call( jo, parameter_list ) );
//            break;
//
//        case t_double:
//            result.assign_jdouble( double_call( jo, parameter_list ) );
//            break;
//
//        case t_object:
//            result.assign_jobject( object_call( jo, parameter_list ) );
//            break;
//
//        //case t_array:
//        //    result.assign_jarray( object_call( jo, parameter_list ) );
//        //    break;
//
//        default:
//            throw_xc( Z_FUNCTION );
//    }
//
//    return result;
//}

//----------------------------------------------------------------------------------Procedure::call

void Procedure::call( jobject jo, const Parameter_list& parameter_list ) const
{
    assert(jo);

    Env env;
    env->CallVoidMethodA( jo, id(), JVALUES_CAST(parameter_list.jvalue_array()) );
    if( env->ExceptionCheck() )  env.throw_java( "CallVoidMethodA" );
}

//--------------------------------------------------------------------------------Method::bool_call

bool Method::bool_call( jobject jo, const Parameter_list& parameter_list ) const
{
    assert(jo);

    Env env;

    jboolean result = env->CallBooleanMethodA( jo, _id, JVALUES_CAST(parameter_list.jvalue_array()) );
    if( env->ExceptionCheck() )  env.throw_java( "CallBooleanMethodA" );

    return result != 0;
}

//--------------------------------------------------------------------------------Method::char_call

wchar_t Method::char_call( jobject jo, const Parameter_list& parameter_list ) const
{
    assert(jo);

    Env env;

    jchar result = env->CallCharMethodA( jo, _id, JVALUES_CAST(parameter_list.jvalue_array()) );
    if( env->ExceptionCheck() )  env.throw_java( "CallCharMethodA" );

    return result;
}

//--------------------------------------------------------------------------------Method::byte_call

jbyte Method::byte_call( jobject jo, const Parameter_list& parameter_list ) const
{
    assert(jo);

    Env env;

    jbyte result = env->CallByteMethodA( jo, _id, JVALUES_CAST(parameter_list.jvalue_array()) );
    if( env->ExceptionCheck() )  env.throw_java( "CallByteMethodA" );

    return result;
}

//-------------------------------------------------------------------------------Method::short_call

short Method::short_call( jobject jo, const Parameter_list& parameter_list ) const
{
    assert(jo);

    Env env;

    short result = env->CallShortMethodA( jo, _id, JVALUES_CAST(parameter_list.jvalue_array()) );
    if( env->ExceptionCheck() )  env.throw_java( "CallShortMethodA" );

    return result;
}

//---------------------------------------------------------------------------------Method::int_call

int Method::int_call( jobject jo, const Parameter_list& parameter_list ) const
{
    assert(jo);

    Env env;

    int result = env->CallIntMethodA( jo, _id, JVALUES_CAST(parameter_list.jvalue_array()) );
    if( env->ExceptionCheck() )  env.throw_java( "CallIntMethodA" );

    return result;
}

//--------------------------------------------------------------------------------Method::long_call

int64 Method::long_call( jobject jo, const Parameter_list& parameter_list ) const
{
    assert(jo);

    Env env;

    int64 result = env->CallLongMethodA( jo, _id, JVALUES_CAST(parameter_list.jvalue_array()) );
    if( env->ExceptionCheck() )  env.throw_java( "CallLongMethodA" );

    return result;
}

//-------------------------------------------------------------------------------Method::float_call

float Method::float_call( jobject jo, const Parameter_list& parameter_list ) const
{
    assert(jo);

    Env env;

    float result = env->CallFloatMethodA( jo, _id, JVALUES_CAST(parameter_list.jvalue_array()) );
    if( env->ExceptionCheck() )  env.throw_java( "CallFloatMethodA" );

    return result;
}

//------------------------------------------------------------------------------Method::double_call

double Method::double_call( jobject jo, const Parameter_list& parameter_list ) const
{
    assert(jo);

    Env env;

    double result = env->CallDoubleMethodA( jo, _id, JVALUES_CAST(parameter_list.jvalue_array()) );
    if( env->ExceptionCheck() )  env.throw_java( "CallDoubleMethodA" );

    return result;
}

//-----------------------------------------------------------------------------Method::jobject_call

jobject Method::jobject_call( jobject jo, const Parameter_list& parameter_list ) const
{
    assert(jo);

    Env env;

    jobject result = env->CallObjectMethodA( jo, _id, JVALUES_CAST(parameter_list.jvalue_array()) );
    if( env->ExceptionCheck() )  env.throw_java( "CallObjectMethodA" );

    return result;
}

//------------------------------------------------------------------------------Method::object_call

string Method::string_call( jobject jo, const Parameter_list& parameter_list ) const
{
    assert(jo);

    Env env;

    jstring jstr = (jstring)env->CallObjectMethodA( jo, _id, JVALUES_CAST(parameter_list.jvalue_array()) );
    if( env->ExceptionCheck() )  env.throw_java( "CallObjectMethodA" );

    string result = env.string_from_jstring( jstr );

    env->DeleteLocalRef( jstr );
    return result;
}

//---------------------------------------------------------------------Static_method::Static_method

Static_method::Static_method( const Class* cls, const string& name, const Parameter_list_signature& signature ) 
:
    Member(cls),
    _id(cls->static_method_id(name, signature))
{ 
}

//---------------------------------------------------------------------Static_method::Static_method
    
Static_method::Static_method( const Class* cls, const char* name, const char* signature ) 
: 
    Member(cls),
    _id(cls->static_method_id(name, signature))
{ 
}

//------------------------------------------------------------------------------Static_method::call

void Static_method::call( jclass cls, const Parameter_list& parameter_list ) const
{
    assert(cls);

    Env env;
    env->CallVoidMethodA( cls, id(), JVALUES_CAST(parameter_list.jvalue_array()) );
    if( env->ExceptionCheck() )  env.throw_java( "CallVoidMethodA" );
}

//-------------------------------------------------------------------------Static_method::bool_call

bool Static_method::bool_call( jclass cls, const Parameter_list& parameter_list ) const
{
    assert(cls);

    Env env;

    jboolean result = env->CallBooleanMethodA( cls, _id, JVALUES_CAST(parameter_list.jvalue_array()) );
    if( env->ExceptionCheck() )  env.throw_java( "CallBooleanMethodA" );

    return result != 0;
}

//-------------------------------------------------------------------------Static_method::char_call

wchar_t Static_method::char_call( jclass cls, const Parameter_list& parameter_list ) const
{
    assert(cls);

    Env env;

    jchar result = env->CallCharMethodA( cls, _id, JVALUES_CAST(parameter_list.jvalue_array()) );
    if( env->ExceptionCheck() )  env.throw_java( "CallCharMethodA" );

    return result;
}

//-------------------------------------------------------------------------Static_method::byte_call

jbyte Static_method::byte_call( jclass cls, const Parameter_list& parameter_list ) const
{
    assert(cls);

    Env env;

    jbyte result = env->CallByteMethodA( cls, _id, JVALUES_CAST(parameter_list.jvalue_array()) );
    if( env->ExceptionCheck() )  env.throw_java( "CallByteMethodA" );

    return result;
}

//------------------------------------------------------------------------Static_method::short_call

short Static_method::short_call( jclass cls, const Parameter_list& parameter_list ) const
{
    assert(cls);

    Env env;

    short result = env->CallShortMethodA( cls, _id, JVALUES_CAST(parameter_list.jvalue_array()) );
    if( env->ExceptionCheck() )  env.throw_java( "CallShortMethodA" );

    return result;
}

//--------------------------------------------------------------------------Static_method::int_call

int Static_method::int_call( jclass cls, const Parameter_list& parameter_list ) const
{
    assert(cls);

    Env env;

    int result = env->CallIntMethodA( cls, _id, JVALUES_CAST(parameter_list.jvalue_array()) );
    if( env->ExceptionCheck() )  env.throw_java( "CallIntMethodA" );

    return result;
}

//-------------------------------------------------------------------------Static_method::long_call

int64 Static_method::long_call( jclass cls, const Parameter_list& parameter_list ) const
{
    assert(cls);

    Env env;

    int64 result = env->CallLongMethodA( cls, _id, JVALUES_CAST(parameter_list.jvalue_array()) );
    if( env->ExceptionCheck() )  env.throw_java( "CallLongMethodA" );

    return result;
}

//------------------------------------------------------------------------Static_method::float_call

float Static_method::float_call( jclass cls, const Parameter_list& parameter_list ) const
{
    assert(cls);

    Env env;

    float result = env->CallFloatMethodA( cls, _id, JVALUES_CAST(parameter_list.jvalue_array()) );
    if( env->ExceptionCheck() )  env.throw_java( "CallFloatMethodA" );

    return result;
}

//-----------------------------------------------------------------------Static_method::double_call

double Static_method::double_call( jclass cls, const Parameter_list& parameter_list ) const
{
    assert(cls);

    Env env;

    double result = env->CallDoubleMethodA( cls, _id, JVALUES_CAST(parameter_list.jvalue_array()) );
    if( env->ExceptionCheck() )  env.throw_java( "CallDoubleMethodA" );

    return result;
}

//----------------------------------------------------------------------Static_method::jobject_call

jobject Static_method::jobject_call( jclass cls, const Parameter_list& parameter_list ) const
{
    assert(cls);

    Env env;

    jobject result = env->CallStaticObjectMethodA( cls, _id, JVALUES_CAST(parameter_list.jvalue_array()) );
    if( env->ExceptionCheck() )  env.throw_java( "CallObjectMethodA" );

    return result;
}

//-------------------------------------------------------------------------------------Field::Field
    
Field::Field( const Class* cls, const string& name, const Simple_signature& signature ) 
:
    Member(cls),
    //_signature(signature),
    _id(cls->field_id(name, signature))
{ 
}

//-------------------------------------------------------------------------------------Field::Field
    
Field::Field( const Class* cls, const char* name, const char* signature ) 
: 
    Member(cls),
    //_signature(signature),
    _id(cls->field_id(name, signature))
{ 
}

//----------------------------------------------------------------------------------Field::set_long
    
void Field::set_long(jobject jo, jlong v) const
{
    Env env;

    env->SetLongField(jo, id(), v);
    if( env->ExceptionCheck() )  env.throw_java( "SetLongField" );
}

//-------------------------------------------------------------------------------------------------

} //namespace javabridge
} //namespace zschimmer
