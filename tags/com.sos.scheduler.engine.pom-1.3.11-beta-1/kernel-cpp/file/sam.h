// sam.h                                    (c) 1995 SOS GmbH Berlin

#ifndef __SAM_H
#define __SAM_H

struct Sam_as_record;

//--------------------------------------------------------------------------------Record_as_sam

struct Record_as_sam : Sos_msg_filter
{
    BASE_CLASS( Sos_msg_filter )
                                Record_as_sam           ();
                               ~Record_as_sam           ();


  protected:
    uint                       _obj_input_block_size    ();
    Area                       _obj_input_buffer        ();
    Sos_ptr<Sos_msg_filter>    _obj_create_reverse_filter( Sos_msg_filter* );
    SOS_DECLARE_MSG_DISPATCHER

#   if !defined SYSTEM_RTTI
        void                   _obj_print               ( ostream* s ) const  { *s << "Record_as_sam"; }
#   endif

  private:
    void                       _obj_data_msg            ( Data_msg* );
    void                       _obj_ack_msg             ( Ack_msg* );
    void                       _obj_data_reply_msg      ( Data_reply_msg* );
  //void                       _obj_open_msg            ( Open_msg* );

    friend struct               Sam_as_record;              //reverse filter

    Byte                       _length_bytes [ 3 ];
    Byte                       _length_bytes_count;
    uint                       _get_length;
    Dynamic_area               _get_buffer;
};

//------------------------------------------------------------------------------------Sam_as_record

struct Sam_as_record : Sos_reblocker//Sos_msg_filter
{
    BASE_CLASS( Sos_reblocker )

                                Sam_as_record           ();
                               ~Sam_as_record           ();

  protected:
  //SOS_DECLARE_MSG_DISPATCHER
    Sos_ptr<Sos_msg_filter>    _obj_create_reverse_filter( Sos_msg_filter* );
    uint                       _obj_input_block_size    ();

#   if !defined SYSTEM_RTTI
        void                   _obj_print               ( ostream* s ) const  { *s << "Sam_as_record"; }
#   endif
    
  private:
  //void                       _obj_data_msg            ( Data_msg* );
  //void                       _obj_ack_msg             ( Ack_msg* );
    void                       _blocked_data_msg        ( Data_msg* );

    friend struct               Record_as_sam;          // reverse_filter

    Byte                       _length_bytes [ 3 ];
    uint                       _length;
    Bool                       _collect_length;
};

#endif
