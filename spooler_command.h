// $Id: spooler_command.h,v 1.22 2003/08/29 20:44:24 jz Exp $

#ifndef __SPOOLER_COMMAND_H
#define __SPOOLER_COMMAND_H

namespace sos {
namespace spooler {

//----------------------------------------------------------------------------------------Show_what

enum Show_what
{
    show_standard       = 0,
    
    show_task_queue     = 0x01,     // Task Queue zeigen
    show_orders         = 0x02,     // Jede Order in der Order_queue zeigen
    show_description    = 0x04,
    show_log            = 0x08,

    show_all_           = 0x80,
    show_all            = 0xFF      // Alle Flags und show_all_ (Bei <show_state> ist z.B. show_orders nicht in show_all enthalten)
};

//-------------------------------------------------------------------------------------------------

void                            dom_append_text_element     ( const xml::Element_ptr& element, const char* element_name, const string& text );
void                            append_error_element        ( const xml::Element_ptr&, const Xc_copy& );
void                            dom_append_nl               ( const xml::Element_ptr& );
string                          xml_as_string               ( const xml::Document_ptr&, bool indent = false );

//--------------------------------------------------------------------------------Command_processor

struct Command_processor
{
                                Command_processor           ( Spooler* spooler )                    : _zero_(this+1),_spooler(spooler),_host(NULL) {}

    void                        execute_file                ( const string& xml_filename );
    string                      execute                     ( const string& xml_text, const Time& xml_mod_time, bool indent = false );
    void                        execute_2                   ( const string& xml_text, const Time& xml_mod_time );
    xml::Element_ptr            execute_command             ( const xml::Element_ptr&, const Time& xml_mod_time );
    xml::Element_ptr            execute_config              ( const xml::Element_ptr&, const Time& xml_mod_time );

    xml::Element_ptr            execute_show_state          ( const xml::Element_ptr&, Show_what );
    xml::Element_ptr            execute_show_history        ( const xml::Element_ptr&, Show_what );
    xml::Element_ptr            execute_show_jobs           ( Show_what );
    xml::Element_ptr            execute_show_threads        ( Show_what );
    xml::Element_ptr            execute_show_process_classes( Show_what );
    xml::Element_ptr            execute_add_jobs            ( const xml::Element_ptr& );
    xml::Element_ptr            execute_show_job            ( const xml::Element_ptr&, Show_what );
  //xml::Element_ptr            execute_show_job            ( Job* );
    xml::Element_ptr            execute_modify_job          ( const xml::Element_ptr& );
    xml::Element_ptr            execute_start_job           ( const xml::Element_ptr& );
    xml::Element_ptr            execute_kill_task           ( const xml::Element_ptr& );
    xml::Element_ptr            execute_modify_spooler      ( const xml::Element_ptr& );
    xml::Element_ptr            execute_signal_object       ( const xml::Element_ptr& );
    xml::Element_ptr            execute_show_job_chains     ( const xml::Element_ptr&, Show_what );
    xml::Element_ptr            execute_show_order          ( const xml::Element_ptr&, Show_what );
    xml::Element_ptr            execute_add_order           ( const xml::Element_ptr& );
    xml::Element_ptr            execute_modify_order        ( const xml::Element_ptr& );

    void                        get_id_and_next             ( const xml::Element_ptr& element, int* id, int* next );

    void                        set_host                    ( Host* host )                          { _host = host; }
    void                        abort_immediately           ( int exit_code = 1 );

    Fill_zero                  _zero_;
    Spooler*                   _spooler;
    bool                       _load_config_immediately;
    xml::Document_ptr          _answer;
    Xc_copy                    _error;
    Host*                      _host;
    Security::Level            _security_level;
    string                     _source_filename;            // Das Verzeichnis wird für <base file=".."> verwendet
};

//-------------------------------------------------------------------------------------------------


} //namespace spooler
} //namespace sos

#endif
