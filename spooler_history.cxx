// $Id: spooler_history.cxx,v 1.38 2003/04/02 19:36:56 jz Exp $

#include "spooler.h"
#include "../zschimmer/z_com.h"

using namespace zschimmer;

namespace sos {
namespace spooler {

const char history_column_names[] =    "id"           ":numeric," 
                                       "spooler_id,"
                                       "job_name,"
                                       "start_time"   ":Datetime,"
                                       "end_time"     ":Datetime,"
                                       "cause,"
                                       "steps,"
                                       "error,"
                                       "error_code,"
                                       "error_text,"
                                       "parameters";

const char history_column_names_db[] = "log";    // Spalten zusätzlich in der Datenbank

const int max_field_length = 1024;      // Das ist die Feldgröße von Any_file -type=(...) für tabulierte Datei.
const int blob_field_size  = 1900;      // Bis zu dieser Größe wird ein Blob im Datensatz geschrieben. ODBC erlaubt nur 2000 Zeichen lange Strings

//---------------------------------------------------------------------------------------------test
/*
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
*/
//------------------------------------------------------------------------------------------uquoted

static string uquoted( const string& value ) 
{ 
    return quoted_string( ucase( value ), '\"', '\"' ); 
}

//---------------------------------------------------------------------------------------sql_quoted

inline string sql_quoted( const string& value ) 
{ 
    return quoted_string( value, '\'', '\'' ); 
}

//----------------------------------------------------------------------------------sql_quoted_name

inline string sql_quoted_name( const string& value ) 
{ 
    return quoted_string( value, '"', '"' ); 
}

//------------------------------------------------------------------------------------prepend_comma

static string prepend_comma( const string& s )  
{ 
    return ", " + s; 
}

//-------------------------------------------------------------------------Transaction::Transaction

Transaction::Transaction( Spooler_db* db )
: 
    _db(db), 
    _guard(&db->_lock) 
{
    _db->rollback();     // Falls irgendeine Transaktion offengeblieben ist
}

//------------------------------------------------------------------------Transaction::~Transaction

Transaction::~Transaction()
{ 
    if( _db )  
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
    _db = NULL;
    _guard.leave(); 
}

//----------------------------------------------------------------------------Transaction::rollback

void Transaction::rollback()
{ 
    _db->rollback(); 
    _db = NULL; 
    _guard.leave(); 
}

//---------------------------------------------------------------------------Spooler_db::Spooler_db

Spooler_db::Spooler_db( Spooler* spooler )
:
    _zero_(this+1),
    _spooler(spooler)
{
}

//---------------------------------------------------------------------------------Spooler_db::open

void Spooler_db::open( const string& db_name, bool need_db )
{
    string my_db_name = db_name;

    _db_name = db_name;
    
    if( _db_name != "" )
    {
        if( _db_name.find(' ') == string::npos  &&  _db_name.find( ':', 1 ) == string::npos )
        {
            if( !is_absolute_filename( _db_name  )  &&  (_spooler->log_directory() + " ")[0] == '*' ) 
            {
                if( need_db )  throw_xc( "SPOOLER-142", _db_name );
                return;
            }

              _db_name = "odbc "         + make_absolute_filename( _spooler->log_directory(),   _db_name );
            my_db_name = "odbc -create " + make_absolute_filename( _spooler->log_directory(), my_db_name );
        }

        if( _db_name.substr(0,5) == "odbc " 
         || _db_name.substr(0,5) == "jdbc ")   _db_name = _db_name.substr(0,5) + " -id=spooler " +   _db_name.substr(5),
                                             my_db_name = _db_name.substr(0,5) + " -id=spooler " + my_db_name.substr(5);

        try
        {
            string stmt;

            _spooler->_log.info( "Datenbank wird geöffnet: " + my_db_name );

            _db.open( "-in -out " + my_db_name );   // -create

            _db_name += " ";

            create_table_when_needed( _spooler->_variables_tablename, 
                                      "\"NAME\" char(100) not null,"
                                      "\"WERT\" char(250),"  
                                      "primary key ( \"name\" )" );


            vector<string> create_extra = vector_map( sql_quoted_name, vector_split( " *, *", _spooler->_history_columns ) );
            for( int i = 0; i < create_extra.size(); i++ )  create_extra[i] += " char(250),";

            create_table_when_needed( _spooler->_history_tablename, 
                                      "\"ID\"          integer not null,"
                                      "\"SPOOLER_ID\"  char(100),"
                                      "\"JOB_NAME\"    char(100) not null,"
                                      "\"START_TIME\"  datetime not null,"
                                      "\"END_TIME\"    datetime,"
                                      "\"CAUSE\"       char(50),"
                                      "\"STEPS\"       integer,"
                                      "\"ERROR\"       bit,"
                                      "\"ERROR_CODE\"  char(50),"
                                      "\"ERROR_TEXT\"  char(250),"
                                      "\"PARAMETERS\"  clob,"
                                      "\"LOG\"         blob," 
                                      + join( "", create_extra ) 
                                      + "primary key( \"ID\" )" );

            stmt = "UPDATE " + uquoted(_spooler->_variables_tablename) + " set \"WERT\" = \"WERT\"+1 where \"NAME\"='spooler_job_id'";
            _job_id_update.prepare( _db_name + stmt );


            stmt = "SELECT \"WERT\" from " + _spooler->_variables_tablename + " where \"NAME\"='spooler_job_id'";
            _job_id_select.prepare( "-in " + _db_name + stmt );

            _job_id_select.execute();
            if( _job_id_select.eof() )  execute( "INSERT into " + uquoted(_spooler->_variables_tablename) + " (\"NAME\",\"WERT\") values ('spooler_job_id','0')" );
            _job_id_select.close( close_cursor );
            commit();

            //stmt = "UPDATE " + _spooler->_history_tablename + " set \"end\"=?, steps=?, \"error\"=?, error_code=?, error_text=?  where \"id\"=?";
            //_history_update.prepare( _db_name + stmt );       //            1        2            3             4             5               6
            //_history_update_params.resize( 1+6 );
            //for( int i = 1; i <= 6; i++ )  _history_update.bind_parameter( i, &_history_update_params[i] );
        }
        catch( const exception& x )  
        { 
            if( need_db )  throw;
            
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
            _history_table.open( "-in -out -key=id sql -table=" + _spooler->_history_tablename + 
                               //" -sql-fields=(id,spooler_id,job_name,start_time,cause,parameters) | " +       extra-Felder nicht vergessen!
                                 " | " + _db_name + " -ignore=(log) -max-length=" + as_string(blob_field_size) );
        }
    }
}

//-------------------------------------------------------------Spooler_db::create_table_when_needed

void Spooler_db::create_table_when_needed( const string& tablename, const string& fields )
{
    Transaction ta = this;

    try
    {
        Any_file select;
        select.open( "-in " + _db_name + " SELECT count(*) from " + tablename );
        select.get_record();
        select.close();
        // ok
    }
    catch( const exception& x )
    {
        _spooler->_log.warn( x.what() );
        _spooler->_log.info( "Tabelle " + tablename + " wird eingerichtet" );
        _db.put( "CREATE TABLE " + tablename + " (" + fields + ") " );
    }

    ta.commit();        // Für select und für create table (jedenfalls bei Jet)
}

//-------------------------------------------------------------------------------Spooler_db::get_id
// Wird von den Threads gerufen

int Spooler_db::get_id()
{
    int id;


    Transaction ta = this;
    {
        if( _db.opened() )
        {
            //_job_id_update.execute();    // id++

            //_job_id_select.execute();
            //id = _job_id_select.get_record().as_int(0);
            //_job_id_select.close( close_cursor );

            execute( "UPDATE " + uquoted(_spooler->_variables_tablename) + " set \"WERT\" = \"WERT\"+1 where \"NAME\"='spooler_job_id'" );

            Any_file sel;
            sel.open( "-in " + _db_name + "SELECT \"WERT\" from " + _spooler->_variables_tablename + " where \"NAME\"='spooler_job_id'" );
            id = sel.get_record().as_int(0);

            LOG( "Spooler_db::get_id() = " << id << '\n' );
        }
        else
        {
            id = ++_next_free_job_id;
        }
    }
    ta.commit();

    return id;
}

//------------------------------------------------------------------------------Spooler_db::execute

void Spooler_db::execute( const string& stmt )
{ 
    THREAD_LOCK( _lock )
    {
        LOG( "Spooler_db::execute  " << stmt << '\n' );
        _db.put( stmt ); 
    }
}

//-------------------------------------------------------------------------------Spooler_db::commit

void Spooler_db::commit()
{
    THREAD_LOCK( _lock )
    {
        if( _db.opened() )  execute( "COMMIT" );
    }
}

//-----------------------------------------------------------------------------Spooler_db::rollback

void Spooler_db::rollback()
{
    THREAD_LOCK( _lock )
    {
        if( _db.opened() )  execute( "ROLLBACK" );
    }
}   

//------------------------------------------------------------------------Spooler_db::spooler_start

void Spooler_db::spooler_start()
{
    if( _db.opened() )
    {
        try
        {
            _id = get_id();     // Der Spooler-Satz hat auch eine Id
     
            Transaction ta = this;
            {
                execute( "INSERT into " + uquoted(_spooler->_history_tablename) + " (\"ID\",\"SPOOLER_ID\",\"JOB_NAME\",\"START_TIME\") "
                         "values (" + as_string(_id) + "," + sql_quoted(_spooler->id()) + ",'(Spooler)',{ts'" + Time::now().as_string(Time::without_ms) + "'})" );
                ta.commit();
            }
        }
        catch( const exception& x )  
        { 
            _spooler->_log.warn( string("FEHLER BEIM SCHREIBEN DER HISTORIE: ") + x.what() ); 
        }
    }
}

//-------------------------------------------------------------------------Spooler_db::spooler_stop

void Spooler_db::spooler_stop()
{
    if( _db.opened() )
    {
        try
        {
            Transaction ta = this;
            {
                execute( "UPDATE " + uquoted(_spooler->_history_tablename) + " set end_time={ts'" + Time::now().as_string(Time::without_ms) + "'} "
                         "where id=" + as_string(_id) );
                ta.commit();
            }
        }
        catch( const exception& x )  
        { 
            _spooler->_log.warn( string("FEHLER BEIM SCHREIBEN DER HISTORIE: ") + x.what() ); 
        }
    }
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
    string section = _job->profile_section();

    _job_name = _job->name();            // Damit read_tail() nicht mehr auf Job zugreift (das ist ein anderer Thread)

    try
    {
        Transaction ta = +_spooler->_db;
        {
            _filename   = read_profile_string            ( _spooler->_factory_ini, section, "history_file" );
            _history_yes= read_profile_bool              ( _spooler->_factory_ini, section, "history"           , _spooler->_history_yes );
            _on_process = read_profile_history_on_process( _spooler->_factory_ini, section, "history_on_process", _spooler->_history_on_process );

            if( !_history_yes )  return;

            if( _spooler->_db->opened()  &&  _filename == "" )
            {
                _with_log = read_profile_with_log( _spooler->_factory_ini, section, "history_with_log", _spooler->_history_with_log );

                set<string> my_columns = set_map( lcase, set_split( ", *", replace_regex( string(history_column_names) + "," + history_column_names_db, ":[^,]+", "" ) ) );

                _spooler->_db->open_history_table();

                const Record_type* type = _spooler->_db->_history_table.spec().field_type_ptr();
                if( type ) {
                    Sos_ptr<Record_type> extra_type = SOS_NEW( Record_type );

                    for( int i = 0; i < type->field_count(); i++ )
                    {
                        string name = type->field_descr_ptr(i)->name();
                        if( my_columns.find( lcase(name) ) == my_columns.end() )  
                        {
                            _extra_names.push_back( name );
                            type->field_descr_ptr(i)->add_to( extra_type );
                        }
                    }
                    _extra_record.construct( extra_type );
                }

                _use_db = true;
            }
            else
            {
                string         extra_columns = read_profile_string ( _spooler->_factory_ini, section, "history_columns", _spooler->_history_columns );
                Archive_switch arc           = read_profile_archive( _spooler->_factory_ini, section, "history_archive", _spooler->_history_archive );

                _type_string = history_column_names;

                _extra_record.construct( make_record_type( extra_columns ) );

                if( extra_columns != "" )  _type_string += "," + extra_columns;

                _extra_names = vector_split( ", *", replace_regex( extra_columns, ":[^,]+", "" ) );

                if( _filename == "" )
                {
                    _filename = "history";
                    if( !_spooler->id().empty() )  _filename += "." + _spooler->id();
                    _filename += ".job." + _job_name + ".txt";
                }
                _filename = make_absolute_filename( _spooler->log_directory(), _filename );
                if( _filename[0] == '*' )  return;      // log_dir = *stderr

                if( arc )  archive( arc, _filename );  

                _file.open( _filename, O_BINARY | O_RDWR | O_CREAT | O_TRUNC, 0600 );
                _file.print( replace_regex( _type_string, "(:[^,]+)?,", "\t" ) + SYSTEM_NL );

                _job->_log.debug( "Neue Historiendatei eröffnet: " +  _filename );
                _use_file = true;
            }

             //record.type()->field_descr_ptr("error_text")->type_ptr()->field_size()
        }
        ta.commit();
    }
    catch( const exception& x )  
    { 
        _job->_log.warn( string("FEHLER BEIM ÖFFNEN DER HISTORIE: ") + x.what() ); 
        _error = true;
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

//-----------------------------------------------------------------------------Job_history::archive

void Job_history::archive( Archive_switch arc, const string& filename )
{
    if( file_exists( filename ) )
    {
        string ext   = extension_of_path( filename );
        string rumpf = filename;
        if( ext != "" )  rumpf = filename.substr( 0, filename.length() - ext.length() - 1 );

        Sos_optional_date_time time = Time::now().as_time_t();
        string arc_filename = rumpf + "." + time.formatted( "yyyy-mm-dd-HHMMSS" );
        if( ext != "" )  arc_filename +=  "." + ext;

        if( arc == arc_gzip )
        {
            arc_filename += ".gz";
            copy_file( "file -b " + filename, "gzip | " + arc_filename );
        }
        else
        {
            rename_file( filename, arc_filename );
        }

        _job->_log.info( "Bisherige Historie ist archiviert worden unter " + arc_filename );
    }
}

//-----------------------------------------------------------------------Job_history::append_tabbed

void Job_history::append_tabbed( string value )
{
    if( !_tabbed_record.empty() )  _tabbed_record += '\t';

    int i = _tabbed_record.length();

    _tabbed_record += value.substr( 0, max_field_length );

    if( strchr( value.c_str(), '\t' )
     || strchr( value.c_str(), '\n' ) )
    {
        for(; i < _tabbed_record.length(); i++ )  
        {
            if( _tabbed_record[i] == '\t' )  _tabbed_record[i] = ' ';
            if( _tabbed_record[i] == '\n' )  _tabbed_record[i] = ' ';
        }
    }
}

//-------------------------------------------------------------------------------Job_history::write

void Job_history::write( bool start )
{
    string parameters;
    
    if( start | _use_file )  parameters = _job->_task->has_parameters()? xml_as_string( _job->_task->parameters_as_dom() )
                                                                       : "";

    string start_time = !start || _job->_task->_running_since? _job->_task->_running_since.as_string(Time::without_ms)
                                                             : Time::now().as_string(Time::without_ms);

    if( _use_db )
    {
        Transaction ta = +_spooler->_db;
        {
            if( start )
            {
                Record record = _spooler->_db->_history_table.create_record();

                record.set_field( "id"             , _job->_task->_id );
                record.set_field( "spooler_id"     , _spooler->id() );
                record.set_field( "job_name"       , _job->name() );
                record.set_field( "start_time"     , start_time );
                record.set_field( "cause"          , start_cause_name( _job->_task->_cause ) );

                if( !parameters.empty()  &&  parameters.length() < blob_field_size )  record.set_field( "parameters", parameters ), parameters = "";

                _spooler->_db->_history_table.insert( record );

                if( !parameters.empty() )
                {
                    Any_file blob;
                    blob.open( "-out " + _spooler->_db->db_name() + " -table=" + _spooler->_history_tablename + " -blob='parameters' where \"ID\"=" + as_string( _job->_task->_id ) );
                    blob.put( parameters );
                    blob.close();
                }
            }
            else
            {
/*
                _spooler->_db->_history_update_params[1] = Time::now().as_string(Time::without_ms);
                _spooler->_db->_history_update_params[2] = _job->_task->_step_count;
                _spooler->_db->_history_update_params[3] = _job->has_error()? 1 : 0;
                _spooler->_db->_history_update_params[4] = _job->_error.code();
                _spooler->_db->_history_update_params[5] = _job->_error.what().substr( 0, 250 );
                _spooler->_db->_history_update_params[6] = _job->_task->_id;
                _spooler->_db->_history_update.execute();
*/
                string stmt = "UPDATE " + uquoted(_spooler->_history_tablename) + " set ";
                stmt +=   "\"START_TIME\"={ts'" + start_time + "'}";
                stmt += ", \"END_TIME\"={ts'" + Time::now().as_string(Time::without_ms) + "'}";
                stmt += ", \"STEPS\"=" + as_string( _job->_task->_step_count );
                stmt += ", \"ERROR\"=" + as_string( _job->has_error() );
                if( !_job->_error.code().empty() ) stmt += ", \"ERROR_CODE\"=" + sql_quoted( _job->_error.code() );
                if( !_job->_error.what().empty() ) stmt += ", \"ERROR_TEXT\"=" + sql_quoted( _job->_error.what().substr( 0, 250 ) );

                for( int i = 0; i < _extra_record.type()->field_count(); i++ )
                {
                    if( !_extra_record.null(i) )
                    {
                        string s = _extra_record.as_string(i);
                        if( !is_numeric( _extra_record.type()->field_descr_ptr(i)->type_ptr()->info()->_std_type ) )  s = sql_quoted(s);
                        stmt += ", " + sql_quoted_name(_extra_names[i]) + "=" + s;
                    }
                }


                stmt += " where id=" + as_string( _job->_task->_id );
                _spooler->_db->execute( stmt );


                // Jobprotokoll
                string log_filename = _job->_log.filename();
                if( _with_log  &&  !log_filename.empty()  &&  log_filename[0] != '*' )
                {
                    try {
                        string blob_filename = _spooler->_db->db_name() + " -table=" + _spooler->_history_tablename + " -blob='log' where \"ID\"=" + as_string( _job->_task->_id );
                        if( _with_log == arc_gzip )  blob_filename = "gzip | " + blob_filename;
                        copy_file( "file -b " + log_filename, blob_filename );
                    }
                    catch( const exception& x ) { _job->_log.warn( string("Historie: ") + x.what() ); }
                }
            }
        }
        ta.commit();
    }

    if( _use_file )
    {
        _tabbed_record = "";
        append_tabbed( _job->_task->_id );
        append_tabbed( _spooler->id() );
        append_tabbed( _job->name() );
        append_tabbed( start_time );
        append_tabbed( start? "" : Time::now().as_string(Time::without_ms) );
        append_tabbed( start_cause_name( _job->_task->_cause ) );
        append_tabbed( _job->_task->_step_count );
        append_tabbed( _job->has_error()? 1 : 0 );
        append_tabbed( _job->_error.code() );
        append_tabbed( _job->_error.what() );
        append_tabbed( parameters );

        if( !start )
        {
            for( int i = 0; i < _extra_record.type()->field_count(); i++ )
            {
                append_tabbed( _extra_record.as_string(i) );
            }
        }

        _file.print( _tabbed_record + SYSTEM_NL );
        //zu langsam: _file.syncdata();
    }
}

//-------------------------------------------------------------------------------Job_history::start

void Job_history::start()
{
    if( !_history_yes )  return;

    if( _task_id == _job->_task->id() )  return;        // start() bereits gerufen
    _task_id = _job->_task->id();

    if( _error )  return;

    _start_called = true;

    _extra_record.construct( _extra_record.type() );


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
    if( !_history_yes )  return;

    if( !_start_called )  return;
    _start_called = false;

    if( _error )  return;
    if( !_job->_task )  return;     // Vorsichtshalber

    try
    {
        if( _use_file )  _file.seek( _record_pos );

        //if( _job->has_error() )  
        //{
            write( false );
/*
        }
        else
        {
            Z_DEBUG_ONLY( if(_use_file|_use_db) _job->_log.debug9( "Historieneintrag wird wieder gelöscht, weil nicht genug Jobschritte ausgeführt worden sind\n" ); )

            //if( _use_file )  SetEndOfFile();

            if( _use_db )
            {
                Transaction ta = &_spooler->_db;
                _spooler->_db->execute( "DELETE from " + _spooler->_history_tablename + " where \"id\"=" + as_string(_job->_task->_id) );
                ta.commit();
            }
        }
*/
    }
    catch( const exception& x )  
    { 
        _spooler->_log.warn( string("FEHLER BEIM SCHREIBEN DER HISTORIE: ") + x.what() ); 
        //_error = true;
    }

    _task_id = 0;
}

//---------------------------------------------------------------------Job_history::set_extra_field
    
void Job_history::set_extra_field( const string& name, const Variant& value )
{
    if( !_history_yes )  return;

    _extra_record.set_field( name, variant_as_string(value) );
}

//---------------------------------------------------------------------------Job_history::read_tail
// Anderer Thread.
// Hier nicht auf _job etc. zugreifen!

xml::Element_ptr Job_history::read_tail( const xml::Document_ptr& doc, int id, int next, Show_what show )
{
    if( !_history_yes )  throw_xc( "SPOOLER-141", _job_name );

    bool with_log = ( show & show_log ) != 0;

    xml::Element_ptr history_element;

    if( !_error )  
    {
        const int max_n = 1000;
        if( abs(next) > max_n )  next = sgn(next) * max_n,  _spooler->_log.warn( "Max. " + as_string(max_n) + " Historiensätze werden gelesen" );
    
        with_log &= _use_db;

        try
        {
            Transaction ta = +_spooler->_db;
            {
                Any_file sel;

                if( _use_file )
                {
                    if( id != -1  ||  next >= 0 )  throw_xc( "SPOOLER-139" );
                    sel.open( "-in -seq tab -field-names | tail -head=1 -reverse -" + as_string(-next) + " | " + _filename );
                }
                else
                if( _use_db )
                {
                    string prefix = ( next < 0? "-in -seq head -" : "-in -seq tail -reverse -" ) + as_string(max(1,abs(next))) + " | ";
                    string clause = " where \"JOB_NAME\"=" + sql_quoted(_job_name);
                    
                    if( id != -1 )
                    {
                        clause += " and \"ID\"";
                        clause += next<0? "<" : next>0? ">" : "=";
                        clause += as_string(id);
                    }

                    clause += " order by \"ID\" ";
                    if( next < 0 )  clause += " desc";
                    
                    sel.open( prefix + _spooler->_db->_db_name + 
                              "select \"ID\", \"SPOOLER_ID\", \"JOB_NAME\", \"START_TIME\", \"END_TIME\", \"CAUSE\", \"STEPS\", \"ERROR\", \"ERROR_CODE\", \"ERROR_TEXT\" " +
                              join( "", vector_map( prepend_comma, _extra_names ) ) +
                              " from " + _spooler->_history_tablename + 
                              clause );
                }
                else
                    throw_xc( "SPOOLER-136" );

                history_element = doc.createElement( "history" );
                dom_append_nl( history_element );

                const Record_type* type = sel.spec().field_type_ptr();
                Dynamic_area rec ( type->field_size() );
    
                while( !sel.eof() )
                {
                    string           param_xml;
                    xml::Element_ptr history_entry = doc.createElement( "history.entry" );

                    sel.get( &rec );
        
                    for( int i = 0; i < type->field_count(); i++ )
                    {
                        string value = type->as_string( i, rec.byte_ptr() );
                        if( value != "" )
                        {
                            string name = type->field_descr_ptr(i)->name();
                            if( name == "parameters" )  param_xml = value;
                                                  else  history_entry.setAttribute( lcase(name), value );
                        }
                    }

                    int id = type->field_descr_ptr("id")->as_int( rec.byte_ptr() );


#ifndef SPOOLER_USE_LIBXML2     // libxml2 stürzt in Dump() ab:
                    if( _use_db ) 
                        param_xml = file_as_string( _spooler->_db->_db_name + "-table=" + _spooler->_history_tablename + " -blob=parameters where id=" + as_string(id), "" );

                    if( !param_xml.empty() )
                    {
                        try {
                            dom_append_nl( history_element );
                            xml::Document_ptr par_doc;
                            par_doc.create();
                            par_doc.load_xml( param_xml );
                            if( par_doc.documentElement() )  history_entry.appendChild( par_doc.documentElement() );
                        }
                        catch( const exception&  x ) { _spooler->_log.warn( string("Historie: ") + x.what() ); }
                        catch( const _com_error& x ) { _spooler->_log.warn( string("Historie: ") + w_as_string(x.Description() )) ; }
                    }
#endif
                    if( with_log )
                    {
                        string log = file_as_string( "gzip -auto | " + _spooler->_db->_db_name + "-table=" + _spooler->_history_tablename + " -blob=log where \"ID\"=" + as_string(id), "" );
                        if( !log.empty() ) dom_append_text_element( history_entry, "log", log );
                    }

                    history_element.appendChild( history_entry );
                    dom_append_nl( history_element );
                }

                sel.close();
            }
            ta.commit();
        }
        catch( const _com_error& x ) { throw_com_error( x, "Job_history::read_tail" ); }
    }

    return history_element;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

