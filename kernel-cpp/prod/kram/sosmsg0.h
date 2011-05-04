// sosmsg0.h

//-----------------------------------------------------------------------------------Msg_input_file

struct Msg_input_file : Sos_msg_filter      // Std_file
{
    BASE_CLASS( Sos_msg_filter )

                                Msg_input_file          ( const char* filename, uint flags );
                               ~Msg_input_file          ();

    void                        msg_run                 ();

  protected:
    void                       _obj_ack_msg             ( Ack_msg* );

  private:
    int                        _file_handle;
    Bool                       _end;
};

//-------------------------------------------------------------------Msg_input_file::Msg_input_file

Msg_input_file::Msg_input_file( const char* filename, uint flags )
{
    _file_handle = open( filename, flags, 0 );

    if( _file_handle == -1 ) {
        raise_errno( errno );
    }

  exceptions
}

//------------------------------------------------------------------Msg_input_file::~Msg_input_file

Msg_input_file::~Msg_input_file()
{
    close( _file_handle );
}

//--------------------------------------------------------------------------Msg_input_file::msg_run

void Msg_input_file::msg_run()
{
    Area area = obj_output_ptr()->obj_input_buffer();

    uint len = read( _file_handle, area.char_ptr(), area.size() );

    if( len == 0 ) {
        _end = true;
        obj_send_end();
        return;
    }

    if( len == (uint)-1 ) {
        raise_errno( errno );
    }

    area.length( len );

    obj_send( area );

  exceptions
}

//--------------------------------------------------------------------Msg_input_file::_obj_ack_msg

void Msg_input_file::_obj_ack_msg( Ack_msg* )
{
    &Base_class::_obj_ack_msg;

    if( _end ) {
        obj_finished();
    } else {
        msg_run();
    }
}

//-------------------------------------------------------------------------------Msg_output_file_if

struct Msg_output_file_if : virtual Sos_msg_filter
{
    enum Spec_code
    {
        sc_open = 1
    };

    struct Ext
    {
    };

    virtual                    ~Msg_output_file_if      ()  {}

    virtual void                open                    ( const char* filename, uint flags, uint protection = 0 ) = 0;

//protected:
  //void                        msg                     ( const Sos_msg& ) = 0;

  private:
    int                        _file_handle;
};

//----------------------------------------------------------------------------------Msg_output_file

struct Msg_output_file : Msg_output_file_if
{
                                Msg_output_file         ();
                                Msg_output_file         ( const char* filename, uint flags, uint protection = 0 );
                               ~Msg_output_file         ();

    void                        open                    ( const char* filename, uint flags, uint protection = 0 );

  private:
    void                       _obj_data_msg            ( Data_msg* );
    void                       _obj_end_msg             ( End_msg* );
    void                       _obj_print               ( ostream* ) const;

  private:
    int                        _file_handle;
};

//-----------------------------------------------------------------Msg_output_file::Msg_output_file

Msg_output_file::Msg_output_file()
:
    _file_handle ( 0 )
{
}

//-----------------------------------------------------------------Msg_output_file::Msg_output_file

Msg_output_file::Msg_output_file( const char* filename, uint flags, uint protection )
{
    open( filename, flags, protection );
}

//----------------------------------------------------------------------------Msg_output_file::open

void Msg_output_file::open( const char* filename, uint flags, uint protection )
{
    _file_handle = ::open( filename, flags, protection );

    if( _file_handle == -1 ) {
        raise_errno( errno );
    }

  exceptions
}

//----------------------------------------------------------------Msg_output_file::~Msg_output_file

Msg_output_file::~Msg_output_file()
{
    close( _file_handle );
}

//-------------------------------------------------------------------Msg_output_file::_obj_data_msg

void Msg_output_file::_obj_data_msg( Data_msg* msg_ptr )
{
    uint len = write( _file_handle, msg_ptr->data().ptr(), msg_ptr->data().length() );
    if( len != msg_ptr->data().length() )  raise_errno( errno );

    //obj_post_ack();
    post( Ack_msg( (Sos_object*)msg_ptr->source_ptr(), this ) );

  exceptions
}

//--------------------------------------------------------------------Msg_output_file::_obj_end_msg

void Msg_output_file::_obj_end_msg( End_msg* msg_ptr )
{
    //obj_post_ack();
    post( Ack_msg( (Sos_object*)msg_ptr->source_ptr(), this ) );
}

//----------------------------------------------------------------------Msg_output_file::_obj_print

void Msg_output_file::_obj_print( ostream* s ) const
{
    *s << dec << setw(0) << "Msg_output_file( " << _file_handle << " )";
}


