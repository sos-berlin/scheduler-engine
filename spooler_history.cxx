// $Id: spooler_history.cxx,v 1.1 2002/04/05 13:21:47 jz Exp $

#include "../kram/sos.h"
#include "spooler.h"

using zschimmer::replace_regex;

namespace sos {
namespace spooler {

const char tab_field_names[] = "id:numeric, spooler_id, job_name, start:Datetime, end:Datetime, "
                               "cause, steps, error, error_code, error_text, parameters";

const int max_field_length = 1024;      // Das ist die Feldgröße von Any_file -type=(...) für tabulierte Datei.
const int blob_field_size  = 50000;

//---------------------------------------------------------------------------------------------test

template< class B >
string operator << ( const string& a, const B& b )
{
    ostrstream s;
    s << a << b;
    return s.str();
}

template< class B >
string operator << ( const char* a, const B& b )
{
    ostrstream s;
    s << a << b;
    return s.str();
}


//---------------------------------------------------------------------------------------sql_quoted

inline string sql_quoted( const string& value ) 
{ 
    return quoted_string( value, '\'', '\'' ); 
}

//------------------------------------------------------------------------Transaction::~Transaction

Transaction::~Transaction()
{ 
    if( !_ok )  
    {
        try 
        { 
            rollback();
        } 
        catch( const exception& ) {}
    }
}

//------------------------------------------------------------------------------Transaction::commit

void Transaction::commit()
{ 
    _db->commit();   
    _guard.leave(); 
    _ok = true; 
}

//----------------------------------------------------------------------------Transaction::rollback

void Transaction::rollback()
{ 
    _db->rollback(); 
    _guard.leave(); 
    _ok = true; 
}

//---------------------------------------------------------------------------Spooler_db::Spooler_db

Spooler_db::Spooler_db( Spooler* spooler )
:
    _zero_(this+1),
    _spooler(spooler)
{
}

//---------------------------------------------------------------------------------Spooler_db::open

void Spooler_db::open( const string& db_name )
{
    _db_name = db_name;

    if( _db_name != "" )
    {
        if( _db_name.find(' ') == string::npos )  _db_name = "odbc -create " + make_absolute_filename( _spooler->log_directory(), _db_name );

        try
        {
            string stmt;
            _db.open( "-in -out " + _db_name );   // -create

            _db_name += " ";

            create_table_when_needed( _spooler->_variables_tablename, 
                                      "\"name\" char(100) not null primary key,"
                                      "\"wert\" char(250)"            );

            create_table_when_needed( _spooler->_history_tablename, 
                                      "\"id\"          integer not null primary key,"
                                      "\"spooler_id\"  char(100),"
                                      "\"job_name\"    char(100) not null,"
                                      "\"start\"       date not null,"
                                      "\"end\"         date,"
                                      "\"cause\"       integer,"
                                      "\"steps\"       integer,"
                                      "\"error\"       bit,"
                                      "\"error_code\"  char(50),"
                                      "\"error_text\"  char(250),"
                                      "\"parameters\"  longtext,"
                                      "\"log\"         longtext" );

            stmt = "UPDATE " + _spooler->_variables_tablename + " set \"wert\" = \"wert\"+1 where \"name\"='spooler_job_id'";
            _job_id_update.prepare( _db_name + stmt );


            stmt = "SELECT \"wert\" from " + _spooler->_variables_tablename + " where \"name\"='spooler_job_id'";
            _job_id_select.prepare( "-in " + _db_name + stmt );
            _job_id_select.execute();
            if( _job_id_select.eof() )
            {
                execute( "INSERT into " + _spooler->_variables_tablename + " (name,wert) values ('spooler_job_id','0')" );
                commit();
            }
            _job_id_select.close( close_cursor );


            //stmt = "UPDATE " + _spooler->_history_tablename + " set \"end\"=?, steps=?, \"error\"=?, error_code=?, error_text=?  where \"id\"=?";
            //_history_update.prepare( _db_name + stmt );       //            1        2            3             4             5               6
            //_history_update_params.resize( 1+6 );
            //for( int i = 1; i <= 6; i++ )  _history_update.bind_parameter( i, &_history_update_params[i] );
        }
        catch( const exception& x )  
        { 
            _spooler->_log.warn( string("FEHLER BEIM ÖFFNEN DER HISTORIENDATENBANK: ") + x.what() ); 
        }
    }
}

//--------------------------------------------------------------------------------Spooler_db::close

void Spooler_db::close()
{
    _history_table.close();

    _db.close();
    _db.destroy();
}

//-------------------------------------------------------------------Spooler_db::open_history_table

void Spooler_db::open_history_table()
{
    THREAD_LOCK( _lock )
    {
        if( !_history_table.opened() )
        {
            _history_table.open( "-in -out -key=id sql -table=" + _spooler->_history_tablename + " | " + _db_name + " -max-length=" + as_string(blob_field_size) );
        }
    }
}

//-------------------------------------------------------------Spooler_db::create_table_when_needed

void Spooler_db::create_table_when_needed( const string& tablename, const string& fields )
{
    try
    {
        Any_file select;
        select.open( "-in " + _db_name + " SELECT count(*) from " + tablename );
        select.get_record();
        select.close();
        // ok
    }
    catch( const exception& )
    {
        _db.put( "CREATE TABLE " + tablename + " (" + fields + ") " );
        _db.put( "COMMIT" );  // Für Jet
    }
}

//-------------------------------------------------------------------------------Spooler_db::get_id
// Wird von den Threads gerufen

int Spooler_db::get_id()
{
    Transaction ta = this;
    {
        if( _db.opened() )
        {
            _job_id_update.execute();

            _job_id_select.execute();

            Record record = _job_id_select.get_record();
            _job_id_select.close( close_cursor );

            ta.commit();
            return record.as_int(0);
        }
        else
        {
            return ++_next_free_job_id;
        }
    }
}

//-------------------------------------------------------------------------------Spooler_db::commit

void Spooler_db::commit()
{
    if( _db.opened() )  _db.put( "COMMIT" );
}

//-----------------------------------------------------------------------------Spooler_db::rollback

void Spooler_db::rollback()
{
    if( _db.opened() )  _db.put( "ROLLBACK" );
}

//-------------------------------------------------------------------------Job_history::Job_history

Job_history::Job_history( Job* job )
: 
    _zero_(this+1) 
{ 
    _job = job; 
    _spooler = _job->_spooler; 
}

//-------------------------------------------------------------------------Job_history::Job_history

Job_history::~Job_history()
{
    try
    {
        close();
    }
    catch( const exception& ) {}
}

//--------------------------------------------------------------------------------Job_history::open

void Job_history::open()
{
    try
    {
        Archive_switch archive;

        string section = _job->profile_section();

        _filename     = read_profile_string    ( "factory.ini", section.c_str(), "history_file" );
        //_columns    = read_profile_string    ( "factory.ini", section.c_str(), "history_columns"    , _spooler->_history_columns    );
        _on_process   = read_profile_on_process( "factory.ini", section.c_str(), "history_on_process" , _spooler->_history_on_process );
        archive       = read_profile_archive   ( "factory.ini", section.c_str(), "history_archive"    , _spooler->_history_archive    );
        _with_log     = read_profile_bool      ( "factory.ini", section.c_str(), "history_with_log"   , _spooler->_history_with_log   );

        if( _spooler->_db.opened()  &&  _filename == "" )
        {
            _spooler->_db.open_history_table();
            _use_db = true;
        }
        else
        {
            if( _filename == "" )
            {
                _filename = "/history";
                if( !_spooler->id().empty() )  _filename += "." + _spooler->id();
                _filename += ".job." + _job->name() + ".txt";
            }
            _filename = make_absolute_filename( _spooler->log_directory(), _filename );
            if( _filename[0] == '*' )  return;      // log_dir = *stderr

            if( archive )  
            {
        /*
                if( file_exists( _filename ) ) 
                {
                    string arc_filename = ...
                    if( arc == arc_gzip )
                    {
                        if( arc == arc_gzip )  arc_filename = "gzip | " + arc_filename;
                        copy_file( "file -b " + _filename, arc_filename );
                    }
                    else
                    {
                        rename_file( _filename, _arc_filename );
                    }
                }

                // Was tun im Fehlerfall? Exception in Job::init() abfangen und protokollieren
        */
            }

            _file.open( _filename, O_BINARY | O_RDWR | O_CREAT | O_TRUNC, 0600 );
            _file.print( replace_regex( replace_regex( tab_field_names, ":[^,]+", "" ), ", *", "\t" ) + SYSTEM_NL );

            _use_file = true;
        }


        //record.type()->field_descr_ptr("error_text")->type_ptr()->field_size()
    }
    catch( const exception& x )  
    { 
        _job->_log.warn( string("FEHLER BEIM ÖFFNEN DER HISTORIE: ") + x.what() ); 
    }
}

//-------------------------------------------------------------------------------Job_history::close

void Job_history::close()
{
    try
    {
        _file.close();
    }
    catch( const exception& x )  
    { 
        _job->_log.warn( string("FEHLER BEIM SCHLIESSEN DER HISTORIE: ") + x.what() ); 
    }
}

//-----------------------------------------------------------------------Job_history::append_tabbed

void Job_history::append_tabbed( string value )
{
    if( !_tabbed_record.empty() )  _tabbed_record += '\t';

    if( value.length() <= max_field_length ) 
    {
        int i = _tabbed_record.length();

        _tabbed_record += value;

        if( strchr( value.c_str(), '\t' )
         || strchr( value.c_str(), '\n' ) )
        {
            _tabbed_record += value;
            for( i; i < _tabbed_record.length(); i++ )  
            {
                if( _tabbed_record[i] == '\t' )  _tabbed_record[i] = ' ';
                if( _tabbed_record[i] == '\n' )  _tabbed_record[i] = ' ';
            }
        }
    }
}

//-------------------------------------------------------------------------------Job_history::write

void Job_history::write( bool start )
{
    string parameters = _job->_task->has_parameters()? xml_as_string( _job->_task->parameters_as_dom() )
                                                     : "";

    if( _use_db )
    {
        Transaction ta = &_spooler->_db;
        {
            if( start )
            {
                Record record = _spooler->_db._history_table.create_record();

                record.set_field( "id"             , _job->_task->_id );
                record.set_field( "spooler_id"     , _spooler->id() );
                record.set_field( "job_name"       , _job->name() );
                record.set_field( "start"          , _job->_task->_running_since.as_string(Time::without_ms) );
                record.set_field( "cause"          , (int)_job->_task->_cause );

                if( !parameters.empty()  &&  parameters.length() < blob_field_size )  record.set_field( "parameters", parameters ), parameters = "";

                _spooler->_db._history_table.insert( record );

                if( !parameters.empty() )
                {
                    Any_file blob;
                    blob.open( "-out " + _spooler->_db.dbname() + " -table=" + _spooler->_history_tablename + " -blob='parameters' where \"id\"=" + as_string( _job->_task->_id ) );
                    blob.put( parameters );
                    blob.close();
                }
            }
            else
            {
/*
                _spooler->_db._history_update_params[1] = Time::now().as_string(Time::without_ms);
                _spooler->_db._history_update_params[2] = _job->_task->_step_count;
                _spooler->_db._history_update_params[3] = _job->has_error()? 1 : 0;
                _spooler->_db._history_update_params[4] = _job->_error.code();
                _spooler->_db._history_update_params[5] = _job->_error.what().substr( 0, 250 );
                _spooler->_db._history_update_params[6] = _job->_task->_id;
                _spooler->_db._history_update.execute();
*/
                string stmt = "UPDATE " + _spooler->_history_tablename + " set ";
                stmt +=   "\"end\"="   + sql_quoted( Time::now().as_string(Time::without_ms) );
                stmt += ", \"steps\"=" + as_string( _job->_task->_step_count );
                if( !_job->_error.code().empty() ) stmt += ", \"error_code\"=" + sql_quoted( _job->_error.code() );
                if( !_job->_error.what().empty() ) stmt += ", \"error_text\"=" + sql_quoted( _job->_error.what().substr( 0, 250 ) );
                stmt += " where id=" + as_string( _job->_task->_id );
                _spooler->_db.execute( stmt );
            }

            // und evtl das Jobprotokoll
        }
        ta.commit();
    }
    else
    {
        _tabbed_record = "";
        append_tabbed( _job->_task->_id );
        append_tabbed( _spooler->id() );
        append_tabbed( _job->name() );
        append_tabbed( _job->_task->_running_since.as_string(Time::without_ms) );
        if( start )  append_tabbed( "" );
               else  append_tabbed( Time::now().as_string(Time::without_ms) );
        append_tabbed( _job->_task->_cause );
        append_tabbed( xml_as_string( _job->_task->parameters_as_dom() ) );
        append_tabbed( _job->_task->_step_count );
        append_tabbed( _job->has_error()? 1 : 0 );
        append_tabbed( _job->_error.code() );
        append_tabbed( _job->_error.what() );
        append_tabbed( "" );                    // Jobprotokoll

        for( int i = 0; i < _extra_columns_names.size(); i++ )
        {
            if( start )  append_tabbed( "" );
                   else  append_tabbed( _extra_fields[ _extra_columns_names[i] ] );
        }

        _file.print( _tabbed_record + SYSTEM_NL );
        //zu langsam: _file.syncdata();

/*
            for( vector<string,string>::iterator it = _extra_fields.begin(); it != _extra_fields.end(); it++ )
            {
                vector<string>::iterator it2 = _extra_columns_names.
            }
*/
    }
}

//-------------------------------------------------------------------------------Job_history::start

void Job_history::start()
{
    if( _error )  return;

    try
    {
        if( _use_file )  _record_pos = _file.tell();

        write( true );
    }
    catch( const exception& x )  
    { 
        _spooler->_log.warn( string("FEHLER BEIM SCHREIBEN DER HISTORIE: ") + x.what() );
        //_error = true;
    }
}

//---------------------------------------------------------------------------------Job_history::end

void Job_history::end()
{
    if( _error )  return;
    if( !_job->_task )  return;     // Vorsichtshalber

    try
    {
        if( _use_file )  _file.seek( _record_pos );

        if( _job->_task->_step_count >= _on_process )  
        {
            write( false );
        }
        else
        {
            Z_DEBUG_ONLY( if(_use_file|_use_db) _job->_log.debug9( "Historieneintrag wird wieder gelöscht, weil nicht genug Jobschritte ausgeführt worden sind\n" ); )

            //if( _use_file )  SetEndOfFile();

            if( _use_db )
            {
                Transaction ta = &_spooler->_db;
                _spooler->_db.execute( "DELETE from " << _spooler->_history_tablename << " where \"id\"=" << _job->_task->_id );
                ta.commit();
            }
        }
    }
    catch( const exception& x )  
    { 
        _spooler->_log.warn( string("FEHLER BEIM SCHREIBEN DER HISTORIE: ") + x.what() ); 
        //_error = true;
    }
}

//---------------------------------------------------------------------------Job_history::read_last

Any_file Job_history::read_last( int n )
{
    Any_file result;

    if( !_error )  
    {
        if( _use_file )
        {
            result.open( "-in head -" + as_string(n) + " | select * order by id desc | -type=(" + tab_field_names + ") tab -field-names | " + _filename );
        }
        else
        if( _use_db )
        {
            result.open( "-in head -" + as_string(n) + " | " + _spooler->_db._db_name + 
                         "select id, spooler_id, job_name, start, end, cause, steps, error, error_code, error_text "
                         "from " + _spooler->_history_tablename + " where job_name=" + sql_quoted(_job->name()) + " order by id desc" );
        }
    }

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

