// $Id$

#include "zschimmer.h"
#include "log.h"
#include "z_process.h"
#include "z_com.h"

#include "file.h"


using std::vector;
using namespace zschimmer::io;


namespace zschimmer {


//-------------------------------------------------------------------------------------------------

static Message_code_text error_codes[] =
{
    { "Z-PROCESS-001", "Prozess $1 hat noch nicht geendet oder wait_for_termination() ist nicht gerufen worden" },
    { "Z-PROCESS-002", "Prozess ist bereits gestartet worden" },
    {}
};

//-------------------------------------------------------------------------------------------Z_INIT

Z_INIT( z_process )
{
    add_message_code_texts( error_codes ); 
}

//-----------------------------------------------------------------------priority_class_from_string
/*
int priority_class_from_string( const string& priority )
{
    int result;


    if( priority.length() >= 1  &&  isdigit( (uchar)priority[ 0 ] ) )
    {
        int i = as_int( priority );

      //if( i >= 24 )  result = REALTIME_PRIORITY_CLASS;
      //else
        if( i >= 13 )  result = HIGH_PRIORITY_CLASS;
        else
        if( i >= 10 )  result = ABOVE_NORMAL_PRIORITY_CLASS;
        else
        if( i >=  8 )  result = NORMAL_PRIORITY_CLASS;
        else
        if( i >=  6 )  result = BELOW_NORMAL_PRIORITY_CLASS;
        else
                       result = IDLE_PRIORITY_CLASS;
    }
    else
    {
        string p = lcase( priority );
        
        if( p == "4"   ||  p == "idle"         )  result = IDLE_PRIORITY_CLASS;           // 0x00000040   2..6   idle: 1  time critical: 15
        else
        if( p == "6"   ||  p == "below_normal" )  result = BELOW_NORMAL_PRIORITY_CLASS;   // 0x00004000   4..8   idle: 1  time critical: 15
        else                                                                                                  
        if( p == "8"   ||  p == "normal"       )  result = NORMAL_PRIORITY_CLASS;         // 0x00000020   6..10  idle: 1  time critical: 15
        else
        if( p == "10"  ||  p == "above_normal" )  result = ABOVE_NORMAL_PRIORITY_CLASS;   // 0x00008000   8..12  idle: 1  time critical: 15
        else
        if( p == "13"  ||  p == "high"         )  result = HIGH_PRIORITY_CLASS;           // 0x00000080   11..15 idle: 1  time critical: 15
        else
      // Realtime verdrängt Maus- und Tastaturroutinen.
      //if( p == "24"  ||  p == "realtime"     )  result = REALTIME_PRIORITY_CLASS;       // 0x00000100   2..31  idle: 16 time critical: 31
      //else
        {
            Z_LOG( "ERROR  Unbekannte Priorität: " << priority << "\n" );
            return 0;
        }
    }


#   ifndef Z_WINDOWS
        switch( result )
        {
            case IDLE_PRIORITY_CLASS:           result = +16;  break;
            case BELOW_NORMAL_PRIORITY_CLASS:   result =  +6;  break;
            case NORMAL_PRIORITY_CLASS:         result =   0;  break;
            case ABOVE_PRIORITY_CLASS:          result =  -8;  break;
            case HIGH_PRIORITY_CLASS:           result = -16;  break;
            return 0;
        }
#   endif

    return result;
}
*/
//---------------------------------------------------------------------try_kill_process_immediately

bool try_kill_process_immediately( pid_t pid, const string& debug_string )
{
    bool result = true;

    try
    {
        kill_process_immediately( pid, debug_string );
    }
    catch( const exception& ) 
    {
        result = false;
    }

    return result;
}

//----------------------------------------------------------------------Process_base::~Process_base

Process_base::~Process_base()
{
    // Nicht inline, damit z_process.cxx mit error_codes eingebunden wird.
}

//---------------------------------------------------------------------------Process_base::obj_name

string Process_base::obj_name() const
{ 
    string c = remove_password( _command_line );
    if( c.length() > 100 )  c = c.substr( 0, 100 ) + "...";

    return S() << "Process(pid=" << _pid << ",cmd=" << c << ")"; 
}

//--------------------------------------------------------------Process_base::set_own_process_group

void Process_base::set_own_process_group( bool b )
{ 
    assert_not_started();
    _own_process_group = b; 
}

//-------------------------------------------------------------------------------------------------

} //namespace zschimmer

