// $Id: java_com.h 14174 2011-01-13 16:26:26Z ss $

#ifndef __ZSCHIMMER_JAVA_COM_H
#define __ZSCHIMMER_JAVA_COM_H


#include "java.h"
#include "z_com_server.h"

namespace zschimmer {
namespace javabridge {

struct Java_idispatch_container;

//------------------------------------------------------------------------------------------Com_env

struct Com_env : Env
{
                                Com_env                     ( JNIEnv* jenv = javabridge::jni_env() ) : Env( jenv ) {}

    void                        jstring_to_bstr             ( const jstring&, BSTR* );
    jobject                     jobject_from_variant        ( const VARIANT&, Java_idispatch_container* = NULL );
    //jobject                   jobject_from_idispatch      ( IDispatch* );
    DISPID                      java_com_get_dispid         ( jlong jidispatch, jstring jname, WORD* dispatch_context = NULL );
    com::Variant                variant_java_com_call       ( jclass, jlong jidispatch, jstring name                 , jobjectArray jparams );
    com::Variant                variant_java_com_call       ( jclass, jlong jidispatch, DISPID, WORD dispatch_context, jobjectArray jparams );
    jobject                     jobject_java_com_call       ( jclass, jlong jidispatch, jstring name                 , jobjectArray jparams );
    jobject                     jobject_java_com_call       ( jclass, jlong jidispatch, DISPID, WORD dispatch_context, jobjectArray jparams );
    com::Variant                variant_java_com_call       ( jclass, jlong jidispatch, jstring name                 , jstring      par1 );
    com::Variant                variant_java_com_call       ( jclass, jlong jidispatch, DISPID, WORD dispatch_context, jstring      par1 );
    com::Variant                jobject_to_variant          (jobject jparam);
    bool                        jobjectarray_to_dispparams  ( jobjectArray jparams, com::Dispparams* );
    com::Variant                variant_java_com_call       ( jclass, jlong jidispatch, DISPID dispid, WORD context, com::Dispparams* );

    jstring                     jstring_from_bstr           ( const BSTR bstr )                     { return jni_env()->NewString( (const jchar*)bstr, SysStringLen(bstr) );}
    jstring                     jstring_from_variant        ( const VARIANT& );
};

//-----------------------------------------------------------------------------------Java_idispatch

struct Java_idispatch : Object
{
    Z_GNU_ONLY(                 Java_idispatch              ();  )                                  // Für gcc 3.2. Nicht implementiert.
                                Java_idispatch              ( IDispatch*, bool is_owner, const char* subclass );
                               ~Java_idispatch              ();

    jobject                 get_jobject                     ()                                      { return _jobject; }


    ptr<IDispatch>             _idispatch;
  //string                     _class_name;
    jobject                    _jobject;
    Global_jobject             _global_jobject;
    bool                       _is_owner;                   // Java-Idispatch wird mit Aufruf von ~Java_idispatch() ungültig
};

//-------------------------------------------------------------------------Java_idispatch_container

struct Java_idispatch_container
{
                                Java_idispatch_container    ()                                      : _zero_(this+1) {}

    void                        add_object                  ( Java_idispatch* );
    void                        release_objects             ()                                      { _java_idispatch_list.clear(); }


    Fill_zero                  _zero_;

    std::list< ptr<Java_idispatch> > _java_idispatch_list;        // Hält alle in einer nativen Methode erzeugten IDispatchs, bis release_objects()
};

//--------------------------------------------------------------------------------Jobject_idispatch

Z_DEFINE_IID( Jobject_idispatch, "feee46eb-6c1b-11d8-8103-000476ee8afb", 
                                  feee46eb,6c1b,11d8,81,03,00,04,76,ee,8a,fb );

struct Jobject_idispatch : Object, //com::simple_idispatch_implementation< Jobject_idispatch >,
                           Non_cloneable
{
                                Jobject_idispatch           ( jobject );
                               ~Jobject_idispatch           ();

    STDMETHODIMP                QueryInterface              ( const IID& iid, void** result )
    {
        Z_IMPLEMENT_QUERY_INTERFACE( this, iid, Jobject_idispatch, result );

        return Object::QueryInterface( iid, result );
    }


    jobject                    _jobject;
};

//-------------------------------------------------------------------------------------------------

} //namespace javabridge
} //namespace zschimmer


#endif

