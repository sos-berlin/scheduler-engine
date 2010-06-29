
#ifndef __SCHEDULER_EVENT2_H
#define __SCHEDULER_EVENT2_H


namespace sos {
namespace scheduler {

//---------------------------------------------------------------------------------Scheduler_event2

struct Scheduler_event2 :  
    idispatch_implementation< Scheduler_event2, spooler_com::Ischeduler_event2 >, 
    spooler_com::Ihas_java_class_name,
    Scheduler_object,
    Non_cloneable
{
    static Class_descriptor     class_descriptor;
    static const Com_method    _methods[];


                            Scheduler_event2            ( Scheduler_object*, const string& code );
                           ~Scheduler_event2            ();

    //IUnknown:
    STDMETHODIMP_(ULONG)    AddRef                      ()                                      { return Idispatch_implementation::AddRef(); }
    STDMETHODIMP_(ULONG)    Release                     ()                                      { return Idispatch_implementation::Release(); }
    STDMETHODIMP            QueryInterface              ( const IID&, void** result );

    // Ihas_java_class_name
    STDMETHODIMP            get_Java_class_name         ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name         ()                                      { return (char*)"sos.spooler.Scheduler_event"; }

    // spooler_com::Ischeduler_event2
    STDMETHODIMP        get_Object                      ( spooler_com::Iorder** );
    STDMETHODIMP        get_Code                        ( BSTR* );

    // Scheduler_object 
    string                  obj_name                    () const;

    Scheduler_object*       object                      () { return _object; }
    string                  code                        () { return _code; }

private:
    Fill_zero              _zero_;
    Scheduler_object*      _object;
    string                 _code;
};

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
