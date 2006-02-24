// $Id$

#ifndef __SPOOLER_COMMAND_H
#define __SPOOLER_COMMAND_H

namespace sos {
namespace spooler {

//-------------------------------------------------------------------------------------------------

xml::Element_ptr                create_error_element        ( const xml::Document_ptr&, const Xc_copy&, time_t = 0 );

//-----------------------------------------------------------------------------------Show_what_enum

enum Show_what_enum
{
    show_standard           = 0,
    
    show_task_queue         = 0x01,     // Task Queue zeigen
    show_job_orders         = 0x02,     // Jede Order in der Order_queue zeigen, nur unter <job>
    show_job_chain_orders   = 0x04,     // Jede Order in der Order_queue zeigen, nur unter <job_chain>
    show_orders             = 0x08,     // Jede Order in der Order_queue zeigen
    show_description        = 0x10,
    show_log                = 0x20,
    show_task_history       = 0x40,
    show_order_history      = 0x80,
    show_remote_schedulers  = 0x100,
    show_run_time           = 0x200,
    show_job_chains         = 0x400,
    show_job_chain_jobs     = 0x800,
    show_jobs               = 0x1000,   // <jobs>
    show_tasks              = 0x2000,   // <tasks>
    show_payload            = 0x4000,
    show_for_database_only  = 0x8000,   // XML-Element nur für Datenbank
    show_id_only            = 0x10000,

    show_all_               = 0x8000,
    show_all                = 0xFFFF    // Alle Flags und show_all_ (Bei <show_state> ist z.B. show_orders nicht in show_all enthalten)
};

inline Show_what_enum operator | ( Show_what_enum a, Show_what_enum b )  { return (Show_what_enum)( (int)a | (int)b ); } 

//----------------------------------------------------------------------------------------Show_what

struct Show_what
{
                                Show_what                   ( Show_what_enum what = show_standard ) : _zero_(this+1), _what(what), 
                                                                                                      _max_orders(INT_MAX), 
                                                                                                      _max_task_history(10) {}

                                operator Show_what_enum     () const                                { return _what; }
    int                         operator &                  ( Show_what_enum w ) const              { return _what & w; }
    void                        operator |=                 ( Show_what_enum w )                    { _what = _what | w; }


    Fill_zero                  _zero_;
    Show_what_enum             _what;
    int                        _max_orders;
    int                        _max_task_history;
    string                     _job_name;
    int                        _task_id;
};

//-------------------------------------------------------------------------------------------------

void                            dom_append_text_element     ( const xml::Element_ptr& element, const char* element_name, const string& text );
void                            append_error_element        ( const xml::Element_ptr&, const Xc_copy& );
Xc_copy                         xc_from_dom_error           ( const xml::Element_ptr& );
void                            dom_append_nl               ( const xml::Element_ptr& );
string                          xml_as_string               ( const xml::Document_ptr&, bool indent = false );

//--------------------------------------------------------------------------------Command_processor

struct Command_processor
{
                                Command_processor           ( Spooler*, Communication::Operation* cp = NULL );
                               ~Command_processor           ();

    void                        execute_file                ( const string& xml_filename );
    ptr<Http_response>          execute_http                ( Http_request* );
    string                      execute                     ( const string& xml_text, const Time& xml_mod_time, bool indent = false );
    xml::Document_ptr           execute                     ( const xml::Document_ptr&, const Time& xml_mod_time = Time::now() );
    void                        execute_2                   ( const string& xml_text, const Time& xml_mod_time = Time::now() );
    void                        execute_2                   ( const xml::Document_ptr&, const Time& xml_mod_time = Time::now() );
    xml::Document_ptr           dom_from_xml                ( const string& xml_text );
    xml::Element_ptr            execute_command             ( const xml::Element_ptr&, const Time& xml_mod_time );
    void                        begin_answer                ();
    void                        append_error_to_answer      ( const exception& );
    void                        append_error_to_answer      ( const Xc& );
    xml::Element_ptr            execute_config              ( const xml::Element_ptr&, const Time& xml_mod_time );

    xml::Element_ptr            execute_show_state          ( const xml::Element_ptr&, const Show_what& );
    xml::Element_ptr            execute_show_history        ( const xml::Element_ptr&, const Show_what& );
    xml::Element_ptr            execute_show_jobs           ( const Show_what& );
  //xml::Element_ptr            execute_show_threads        ( const Show_what& );
    xml::Element_ptr            execute_show_process_classes( const Show_what& );
    xml::Element_ptr            execute_add_jobs            ( const xml::Element_ptr& );
    xml::Element_ptr            execute_job                 ( const xml::Element_ptr& );
    xml::Element_ptr            execute_job_chain           ( const xml::Element_ptr& );
    xml::Element_ptr            execute_show_job            ( const xml::Element_ptr&, const Show_what& );
  //xml::Element_ptr            execute_show_job            ( Job* );
    xml::Element_ptr            execute_modify_job          ( const xml::Element_ptr& );
    xml::Element_ptr            execute_start_job           ( const xml::Element_ptr& );
    xml::Element_ptr            execute_show_task           ( const xml::Element_ptr&, const Show_what& );
    xml::Element_ptr            execute_kill_task           ( const xml::Element_ptr& );
    xml::Element_ptr            execute_modify_spooler      ( const xml::Element_ptr& );
    xml::Element_ptr            execute_terminate           ( const xml::Element_ptr& );
    xml::Element_ptr            execute_signal_object       ( const xml::Element_ptr& );
    xml::Element_ptr            execute_show_job_chains     ( const xml::Element_ptr&, const Show_what& );
    xml::Element_ptr            execute_show_job_chain      ( const xml::Element_ptr&, const Show_what& );
    xml::Element_ptr            execute_show_order          ( const xml::Element_ptr&, const Show_what& );
    xml::Element_ptr            execute_add_order           ( const xml::Element_ptr& );
    xml::Element_ptr            execute_modify_order        ( const xml::Element_ptr& );
    xml::Element_ptr            execute_remove_order        ( const xml::Element_ptr& );
    xml::Element_ptr            execute_remove_job_chain    ( const xml::Element_ptr& );
    xml::Element_ptr            execute_register_remote_scheduler( const xml::Element_ptr& );
    xml::Element_ptr            execute_service_request     ( const xml::Element_ptr& );

    void                        get_id_and_next             ( const xml::Element_ptr& element, int* id, int* next );

    void                        set_host                    ( Host* );
    void                        set_communication_operation ( Communication::Operation* p )         { _communication_operation = p; }
    void                        set_validate                ( bool b )                              { _validate = b; }
    void                        abort_immediately           ( int exit_code = 1 );

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    Communication::Operation*  _communication_operation;
    bool                       _load_config_immediately;
    xml::Document_ptr          _answer;
    Xc_copy                    _error;
    Host*                      _host;
    bool                       _validate;
    Security::Level            _security_level;
    string                     _source_filename;            // Das Verzeichnis wird für <base file=".."> verwendet
};

//-------------------------------------------------------------------------------------------------


} //namespace spooler
} //namespace sos

#endif
