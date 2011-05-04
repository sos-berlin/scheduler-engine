// soscopy.h                                ©1996 SOS GmbH Berlin
//                                          Joacim Zschimmer

#ifndef __SOSPROG_H
#   include "../kram/sosprog.h"
#endif


//#define SOSCOPY_DIALOG

namespace sos {

class Soscopy_dialog;
class Soscopy_copying_dialog;


struct Soscopy_app : Sos_program
{
                       
                                Soscopy_app             ();

    int                         main                    ( int, char** );
    BOOL                        ExitInstance            ();

    uint4                       copy_file               ( Soscopy_copying_dialog* = NULL );

    Fill_zero                  _zero_;

    Sos_string                 _input_filename;
    Sos_string                 _output_filename;
    Sos_string                 _input_file_filename;
    Sos_string                 _output_file_filename;
    Sos_string                 _files_filename;
    Bool                       _open_out_first;
    long                       _count;
    long                       _skip;
    Bool                       _single_step;
    long                       _verbose;                // > 0: Alle _verbose Sätze einen Punkt schreiben

#   ifdef SOSCOPY_DIALOG
        Soscopy_dialog*        _dlg;
#   endif
};


extern Soscopy_app app;


} //namespace
