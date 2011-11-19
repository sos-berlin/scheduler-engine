// fs.h

#ifndef __FS_H
#define __FS_H

//#if !defined __SOSSTREA_H
//    #include <sosstrea.h>
//#endif

struct Sos_binary_istream;
struct Sos_binary_ostream;

#if !defined __TIME_H
#   include <time.h>
#endif

#if !defined __SOSARRAY_H
#   include "../kram/sosarray.h"
#endif

#if !defined __RAPID_H
#   include "../fs/rapid.h"
#endif

#if !defined __ANYFILE_H
#   include "../file/anyfile.h"
#endif

#if !defined __SOSCLIEN_H
#   include "../kram/sosclien.h"
#endif

namespace sos {

struct Fileserver;
struct Fs_client;

struct Fs_base : Rapid
{
        struct Check_value
        {
                                Check_value             ();

            Bool                operator==              ( const Check_value& );
            Bool                operator!=              ( const Check_value& v )      { return !( *this == v ); }
            void                generate                ();

            void                read                    ( Sos_binary_istream* );
            void                write                   ( Sos_binary_ostream* );

        //private:
            uint2              _value;
        };

    struct Id
    {
                                Id                      ();
                                Id                      ( int index );
                                Id                      ( int index, const Check_value& );

        int                     index                   () const;
        Check_value             check_value             () const;     // Exception ID

        void                    read                    ( Sos_binary_istream* );
        void                    write                   ( Sos_binary_ostream* );
        friend ostream&         operator <<             ( ostream&, const Id& );

      private:
        uint2                  _index;
        Check_value            _check_value;
    };
};


struct Fs_client_file : Sos_object, Fs_base
{
                                Fs_client_file          ( Fs_client* );
                                Fs_client_file          ( Fs_client*, const Fs_client_file& );
                               ~Fs_client_file          ();

    Check_value                 check_value             () const    { return _check_value; }
    void                        open                    ( Fd*, Openpar*, const Sos_string& );
    void                        close                   ( Close_mode = close_normal );
    void                        put_burst               ( Sos_binary_istream* );
    void                        get_burst               ( Sos_binary_ostream*, uint recommended_block_size,
                                                          const Byte* key, const Byte* key_end,
                                                          Bool in_open = false );
    void                        put_key                 ( Sos_binary_istream* );

    Record_length               key_length              ()      { return _key_length; }
    Bool                        file_closed             ()      { return _file_closed; }
    void                       _obj_print               ( ostream* s ) const  { *s << "Fs_client_file(\"" << _filename << "\")"; }

    Fill_zero                  _zero_;
    Fs_client*                 _client;
    Any_file                   _file;
    Sos_string                 _filename;
    Const_area_handle          _next_key;
    Dynamic_area/*Const_area_handle*/  _next_record;
    Bool                       _next_record_valid;
    uint                       _key_position;
    uint                       _key_length;
    Bool                       _close_at_eof;
    Bool                       _file_closed;
    Bool                       _text;                   // EBCDIC-ASCII-Konvertierung?
    Check_value            	   _check_value;
    time_t                     _last_used_time;
};


struct Fs_client : Sos_object, Fs_base
{
/*
    struct File_entry
    {
                                File_entry              ()   : _file_ptr(0) {}

        Bool                    free                    () const { return !_file_ptr && !_check_value._value; }
        Fs_client_file*        _file_ptr;
        Check_value            _check_value;
    };
*/

                                Fs_client               ( Fileserver*, const Sos_string& user_name );
                               ~Fs_client               ();

    void                        close_all_files         ();
    Bool                        valid                   () const;
    const char*                 name                    () const;
    void                        check                   ( Id ) const;
    Check_value                 check_value             () const;
    Id                          add                     ( const Sos_ptr<Fs_client_file>& );
    Sos_ptr<Fs_client_file>&    file_ptr                ( Id );

    void                       _obj_print               ( ostream* s ) const  { *s << "Fs_client(\"" << _sos_client._name << "\")"; }
  //Const_area_handle          _obj_client_name         () const              { Dynamic_area a; a = name(); return a; }
    Sos_client*                _obj_client              ()                    { return &_sos_client; }

    Fill_zero                  _zero_;
    Fileserver*                _fileserver;
    Sos_client                 _sos_client;
    Sos_simple_array< Sos_ptr<Fs_client_file> > _file_array;
    Bool                       _valid;
    Check_value                _check_value;
    //Sos_limited_text<fs_max_client_name_length> _name;
  //Sos_string                 _name;
    time_t                     _last_used_time;
};

typedef Sos_simple_array< Sos_static_ptr<Fs_client> >  Fs_client_array;

struct Fileserver : Sos_msg_filter, Fs_base

/*
    Der Fileserver.
    Er ist ein Abs_file, dem Aufträge mit put() übergeben und
    Antworten mit get() entnommen werden.
*/
{
    BASE_CLASS( Sos_object )

                                Fileserver              ();
                               ~Fileserver              ();

    int                         add                     ( Fs_client* );
    void                        kill                    ();
    void                        kill_client             ( int index );

//private:
    void                       _obj_open_msg            ( Open_msg* );
    void                       _obj_run_msg             ( Run_msg* );
    void                       _obj_data_msg            ( Data_msg* );
    void                       _obj_ack_msg             ( Ack_msg* );
    void                       _obj_end_msg             ( End_msg* );
    void                       _obj_error_msg           ( Error_msg* );

#   if !defined SYSTEM_RTTI
    	void                   _obj_print               ( ostream* s ) const { *s << "Fileserver"; }
#   endif

    SOS_DECLARE_MSG_DISPATCHER

    friend struct               Fs_job;

    Fill_zero                  _zero_;
  //Sos_object_ptr             _runner;                 // Nicht Sos_object_ptr!
    Sos_string                 _filename_prefix;
    Sos_static_ptr<Fs_client>   client_ptr              ( Id application, Id user );
    Fs_client_array            _client_array;
    Sos_static_ptr<Fs_job>     _job_ptr;
    Sos_string                 _client_name;            // Hostname etwa "tcp host port|sam/record"
    Bool                       _initialized;
    Bool                       _ending;
    Bool                       _killed;

    static Sos_simple_array<Fileserver*> _fs_array;
};

} //namespace sos

#endif
