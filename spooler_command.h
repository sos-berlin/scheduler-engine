// $Id: spooler_command.h,v 1.3 2001/02/08 11:21:15 jz Exp $

#ifndef __SPOOLER_COMMAND_H
#define __SPOOLER_COMMAND_H

namespace sos {
namespace spooler {

void                            append_error_element        ( const xml::Element_ptr&, const Xc_copy& );
void                            dom_append_nl               ( const xml::Element_ptr& );

//--------------------------------------------------------------------------------Command_processor

struct Command_processor
{
                                Command_processor           ( Spooler* spooler )                    : _spooler(spooler),_host(NULL) {}

    string                      execute                     ( const string& xml_text );
    void                        execute_2                   ( const string& xml_text );
    xml::Element_ptr            execute_command             ( const xml::Element_ptr& );
    xml::Element_ptr            execute_config              ( const xml::Element_ptr& );
    xml::Element_ptr            execute_show_state          ();
    xml::Element_ptr            execute_show_threads        ();
    xml::Element_ptr            execute_show_job            ( Job* );
    xml::Element_ptr            execute_modify_job          ( const xml::Element_ptr& );
    xml::Element_ptr            execute_modify_spooler      ( const xml::Element_ptr& );
    xml::Element_ptr            execute_signal_object       ( const xml::Element_ptr& );

    void                        set_host                    ( Host* host )                          { _host = host; }

    Spooler*                   _spooler;
    xml::Document_ptr          _answer;
    Xc_copy                    _error;
    Host*                      _host;
    Security::Level            _security_level;
};

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

#endif
