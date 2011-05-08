//#define MODULE_NAME "Keyfile"
//#define COPYRIGHT   "(c) 1995 SOS GmbH Berlin"
//#define AUTHOR      "Joacim Zschimmer"
#include "precomp.h"

#if 0

#include <optimize.h>

#if defined __BORLANDC__  ||  _MSC_VER
#   include <io.h>       // open(), read() etc.
#else
#   include <unistd.h>   // read(), write(), close()
#endif

#if defined __BORLANDC__
#   include <dir.h>            // MAXPATH
#endif

#include <limits.h>
#include <fcntl.h>          // open()
#include <sys/stat.h>       // S_IREAD, S_IWRITE
#include <errno.h>

#include <sosstrng.h>
#include <sos.h>
#include <xception.h>
#include <log.h>
#include <absfile.h>
#include <sosfiltr.h>
#include <sosfact.h>


//-------------------------------------------------------------------------------------Key_file

struct Key_file : Sos_msg_filter  // Sos_file
{
    BASE_CLASS( Sos_object )

  protected:
    void                       _obj_msg                 ( Sos_msg* );
    void                       _obj_print               ( ostream* ) const;

  private:
  //void                       _obj_open_msg            ( Open_msg* );
  //void                       _obj_run_msg             ( Run_msg* );
    void                       _obj_end_msg             ( End_msg* );
    void                       _obj_data_msg            ( Data_msg* );
    void                       _obj_get_msg             ( Get_msg* m );
  //void                       _obj_ack_msg             ( Ack_msg* );
    void                       _obj_seek_msg            ( Seek_msg* );
    void                        next_data               ();
    void                       _read                    ( Area* );

    int                        _block_size
};

//-------------------------------------------------------------------------------Key_file_descr

struct Key_file_descr : Sos_object_descr
{
    BASE_CLASS( Sos_object_descr )

    const char* name() const  { return "unsorted_key_file"; }

    Sos_object_ptr create( Subtype_code ) const
    {
        Sos_ptr<Key_file> file = SOS_NEW_PTR( Key_file );
        return +file;
    }
};

       const Key_file_descr     _key_file_descr;
extern const Sos_object_descr&   key_file_descr = _key_file_descr;

//-----------------------------------------------------------------------Key_file::_read

void Key_file::_read( Area* buffer_ptr )
{
    uint len = read( _file_handle, buffer_ptr->char_ptr(), buffer_ptr->size() );

    if( len == 0 )         throw Eof_error();
    if( len == (uint)-1 )  throw Errno_error();

    buffer_ptr->length( len );
}

//---------------------------------------------------------------Key_file::_obj_open_msg
/*
void Key_file::_obj_open_msg( Open_msg* m )
{
    int OPEN_MODE_IST_IMMER_INOUT;

    _obj_client_ptr = m->source_ptr();

    File_base::Open_mode open_mode = File_base::inout;  // = m->file_spec().open_mode();

    char fn [ MAXPATH + 1 ];

    if( length( m->name() ) > sizeof fn - 1 )  throw Xc( "D104" );

    strcpy( fn, m->name() );

#   if !defined SYSTEM_WIN
        if( strcmpi( fn, "*stdin"      ) == 0
         || strcmp ( fn, "/dev/stdin"  ) == 0 )  _file_handle = fileno( stdin );
        else
        if( strcmpi( fn, "*stdout"     ) == 0
         || strcmp ( fn, "/dev/stdout" ) == 0 )  _file_handle = fileno( stdout );
        else
        if( strcmpi( fn, "*stderr"     ) == 0
         || strcmp ( fn, "/dev/stderr" ) == 0 )  _file_handle = fileno( stderr );
        else
#   endif
    {   // nicht stdxxx
        int access = 0;

        if(  ( open_mode & File_base::inout )  == File_base::inout )  access |= O_RDWR;
        else
        {
          if(  open_mode & File_base::in         )        access |= O_RDONLY;
          if(  open_mode & File_base::out        )
          {
                                                          access |= O_WRONLY;
              if( !( open_mode & File_base::nocreate ) )  access |= O_CREAT;
              if(    open_mode & File_base::ate        )  access |= O_APPEND;
              if(    open_mode & File_base::trunc      )  access |= O_TRUNC;
              if(    open_mode & File_base::noreplace  )  access |= O_EXCL;
          }
        }
      //if(    open_mode &                       )  access |= O_NDELAY;
        #if defined SYSTEM_WIN || defined SYSTEM_DOS
      //if(    open_mode & Open_mode::binary     )  access |= O_BINARY;
      //                                       else access &= ~O_TEXT;
            access |= O_BINARY;  // immer, sonst funktioniert seek() nicht
        #endif

        _file_handle = open( m->name(), access, S_IREAD_all | S_IWRITE_all );
   }

   if( _file_handle == -1 )  throw Errno_error();

   obj_reply_ack();
}
*/
//----------------------------------------------------------------Key_file::_obj_run_msg
/*
void Key_file::_obj_run_msg( Run_msg* m )
{
    _runner_ptr = m->source_ptr();

    if( !obj_output_ptr() ) {
        LOG( *this << ".obj_output_ptr( _runner_ptr->obj_reverse_filter() )\n" );
        obj_output_ptr( SOS_CAST( Sos_msg_filter, _runner_ptr )->obj_reverse_filter() );
        //LOG( *this << ".obj_output_ptr( _runner_ptr )\n" );
        //obj_output_ptr( _runner_ptr );
    }

    next_data();
}
*/
//-------------------------------------------------------------------Key_file::next_data

void Key_file::next_data()
{
    Area buffer = obj_output_ptr()->obj_input_buffer();

    try {
        _read( &buffer );
    }
    catch( const Eof_error& )
    {
        reply( Ack_msg( _runner_ptr, this ) );
    }
    catch( const Xc& x )
    {
        reply( Error_msg( _runner_ptr, this, x ) );
    }

    obj_send( buffer );
}

//----------------------------------------------------------------------Key_file::_obj_data_msg

void Key_file::_obj_data_msg( Data_msg* msg_ptr )
{
    Paßt der Satz noch in den letzten Block der Datei? schreiben, sonst neuen Block schreiben;

    int block_rest_size = _block_size - ( _file_size % _block_size );

    try {
        _output->__write__( _file_size, m->data() );
    }
    catch( const Too_long_error& )
    {
        _output->__write__( round_up( _file_size, _block_size ), m->data() );
    }

/*
    uint len = write( _file_handle, msg_ptr->data().ptr(), msg_ptr->data().length() );
    if( len != msg_ptr->data().length() )  throw Errno_error();

    reply( Ack_msg( msg_ptr->source_ptr(), this ) );
*/
}

//-----------------------------------------------------------------------Key_file::_obj_end_msg

void Key_file::_obj_end_msg( End_msg* msg_ptr )
{
    int rc = close( _file_handle );
    _file_handle = -1;
    if( rc )  throw Errno_error();

    reply( Ack_msg( msg_ptr->source_ptr(), this ) );
}

//-----------------------------------------------------------------------Key_file::_obj_ack_msg

void Key_file::_obj_ack_msg( Ack_msg* )
{
    next_data();
}

//-----------------------------------------------------------------------Key_file::_obj_get_msg

void Key_file::_obj_get_msg( Get_msg* m )
{
    °°Lesen bis \n, Record_as_nl nutzbar?;

    long pos = -1;

    _get_buffer.allocate_min( m->length()? m->length() : /*obj_default_buffer_size*/15*4096 );

    if( !_dont_use_tell ) {
        pos = tell( _file_handle );
        if( pos == -1 ) {
            if( errno == ESPIPE )  _dont_use_tell = true;       // s.a. _obj_seek_msg
                             else  throw Errno_error( errno );
        }
    }

    _read( &_get_buffer );

    reply( Data_reply_msg( m->source_ptr(), this, _get_buffer, pos ) );
}

//---------------------------------------------------------------Key_file::_obj_seek_msg

void Key_file::_obj_seek_msg( Seek_msg* m )
{
    if( _dont_use_tell )  throw Errno_error( ESPIPE );        // ???

    lseek( _file_handle, m->_pos, SEEK_SET );

    reply( Ack_msg( m->source_ptr(), this ) );
}

//------------------------------------------------------------------Key_file::_obj_print

void Key_file::_obj_print( ostream* s ) const
{
    *s << dec << setw(0) << "Key_file( " << _file_handle << " )";
}

//--------------------------------------------------------------------Key_file::_obj_msg

SOS_BEGIN_MSG_DISPATCHER( Key_file )
    SOS_DISPATCH_MSG( data )
    SOS_DISPATCH_MSG( get  )
    SOS_DISPATCH_MSG( seek )
    SOS_DISPATCH_MSG( get_direct )
    SOS_DISPATCH_MSG( store )
  //SOS_DISPATCH_MSG( delete_record )
  //SOS_DISPATCH_MSG( open )
    SOS_DISPATCH_MSG( end  )
SOS_END_MSG_DISPATCHER

#endif

