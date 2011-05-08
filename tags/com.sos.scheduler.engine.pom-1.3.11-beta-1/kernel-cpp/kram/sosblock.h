// sosblock.h                                           © 1995 SOS GmbH Berlin

#ifndef __SOSBLOCK_H
#define __SOSBLOCK_H

#if !defined __SOSFILTR_H
#   include <sosobj.h>
#   include <sosmsg.h>
#   include <sosfiltr.h>         // Sos_msg_filter
#endif

namespace sos
{

//------------------------------------------------------------------------------------Sos_reblocker

struct Sos_reblocker : Sos_msg_filter
{
    BASE_CLASS( Sos_msg_filter )

  protected:
                                Sos_reblocker           ( Bool block_read );

    SOS_DECLARE_MSG_DISPATCHER
  //uint                       _obj_input_block_size    ();
    virtual Sos_ptr<Sos_msg_filter> _obj_create_reverse_filter( Sos_msg_filter* );

//private:
    void                       _obj_open_msg            ( Open_msg* );
    void                       _obj_data_msg            ( Data_msg* );
    void                       _obj_ack_msg             ( Ack_msg * );
    void                       _process_input           ();

    virtual void               _blocked_data_msg        ( Data_msg* ) = 0;
    void                        obj_reply_ack           ();

    Dynamic_area               _input_buffer;
    const Byte*                _input_ptr;
    const Byte*                _input_end;
  //uint                       _input_block_size;       // Sos_read_blocker
    uint                       _output_block_size;      // Sos_write_blocker
    Dynamic_area               _output_buffer;          // Sos_write_blocker
    Area                       _buffer;
  //Bool                       _eof;
    Bool                       _block_read;

  private:
    void                        send_data               ( const Const_area& );
    Bool                       _ack; 
    int                        _in_process_input;
};

//---------------------------------------------------------------------------------Sos_read_blocker
/*
struct Sos_read_blocker : Sos_reblocker
{
    BASE_CLASS( Sos_reblocker )

                                Sos_read_blocker        ();

  protected:
    virtual void               _blocked_data_msg        ( Data_msg* );
    Area                       _obj_input_buffer        ();
};

//--------------------------------------------------------------------------------Sos_write_blocker

struct Sos_write_blocker : Sos_reblocker
{
    BASE_CLASS( Sos_reblocker )

                                Sos_write_blocker       ();

  protected:
    virtual void               _blocked_data_msg        ( Data_msg* );
    Area                       _obj_input_buffer        ();
};

//==========================================================================================inlines

//---------------------------------------------------------------Sos_read_blocker::Sos_read_blocker

inline Sos_read_blocker::Sos_read_blocker()
:
    Sos_reblocker(true )
{
}
*/

} //namespace sos

#endif
