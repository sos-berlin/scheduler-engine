// $Id: spooler_history.cxx,v 1.3 2002/04/05 22:14:39 jz Exp $

#include "../kram/sos.h"
#include "spooler.h"
#include "../zschimmer/z_com.h"

using zschimmer::replace_regex;
using zschimmer::split;
using zschimmer::join;
using zschimmer::zmap;
using zschimmer::variant_is_numeric;

namespace sos {
namespace spooler {

const char history_field_names[] = "id:numeric," 
                                   "spooler_id,"
                                   "job_name,"
                                   "start:Datetime,"
                                   "end:Datetime,"
                                   "cause,"
                                   "steps,"
                                   "error,"
                                   "error_code,"
                                   "error_text,"
                                   "parameters,"
                                   "log";

const int max_field_length = 1024;      // Das ist die Feldgröße von Any_file -type=(...) für tabulierte Datei.
const int blob_field_size  = 50000;

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
//---------------------------------------------------------------------------------------sql_quoted

inline string sql_quoted( const string& value ) 
{ 
    return quoted_string( value, '\'', '\'' ); 
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
            if( _job_id_select.eof() )  execute( "INSERT into " + _spooler->_variables_tablename + " (name,wert) values ('spooler_job_id','0')" );
            _job_id_select.close( close_cursor );
            commit();

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
    Transaction ta = this;

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
    }

    ta.commit();        // Für select und für create table (jedenfalls bei Jet)
}

//-------------------------------------------------------------------------------Spooler_db::get_id
// Wird von den Threads gerufen

int Spooler_db::get_id()
{
    int id;

    rollback();

    Transaction ta = this;
    {
        if( _db.opened() )
        {
            //_job_id_update.execute();    // id++

            //_job_id_select.execute();
            //id = _job_id_select.get_record().as_int(0);
            //_job_id_select.close( close_cursor );

            execute( "UPDATE " + _spooler->_variables_tablename + " set \"wert\" = \"wert\"+1 where \"name\"='spooler_job_id'" );

            Any_file sel;
            sel.open( "-in " + _db_name + "SELECT \"wert\" from " + _spooler->_variables_tablename + " where \"name\"='spooler_job_id'" );
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
        Transaction    ta = &_spooler->_db;
        Archive_switch archive;
        string         section = _job->profile_section();
        vector<string> my_columns = split( ", *", replace_regex( history_field_names, ":[^,]+", "" ) );
        string         extra_columns;

        _filename     = read_profile_string    ( "factory.ini", section.c_str(), "history_file" );
        extra_columns = read_profile_string    ( "factory.ini", section.c_str(), "history_columns"    , _spooler->_history_columns    );
        _on_process   = read_profile_on_process( "factory.ini", section.c_str(), "history_on_process" , _spooler->_history_on_process );
        archive       = read_profile_archive   ( "factory.ini", section.c_str(), "history_archive"    , _spooler->_history_archive    );
        _with_log     = read_profile_bool      ( "factory.ini", section.c_str(), "history_with_log"   , _spooler->_history_with_log   );

        _columns = history_field_names;
        if( extra_columns != "" )  _columns += "," + extra_columns;

        if( _spooler->_db.opened()  &&  _filename == "" )
        {
            _spooler->_db.open_history_table();

            const Record_type* type = _spooler->_db._history_table.spec().field_type_ptr();
            if( type ) {
                for( int i = 0; i < type->field_count(); i++ )
                {
                    string name = type->field_descr_ptr(i)->name();
                    int j;
                    for( j = 0; j < my_columns.size(); j++ )  if( stricmp( my_columns[j].c_str(), name.c_str() ) == 0 )  break;
                    if( j == my_columns.size() )  _extra_names.push_back( name );
                }
            }

            _use_db = true;
        }
        else
        {
            _extra_names = split( ", *", replace_regex( extra_columns, ":[^,]+", "" ) );

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

            string head_line = history_field_names;
            if( _extra_names.size() )  head_line += "\t" + join( "\t", _extra_names );
            _file.print( head_line + SYSTEM_NL );
            //_file.print( replace_regex( replace_regex( _columns, ":[^,]+", "" ), ", *", "\t" ) + SYSTEM_NL );

            _use_file = true;
        }

         //record.type()->field_descr_ptr("error_text")->type_ptr()->field_size()
        ta.commit();
 
        _extra_values.resize( _extra_names.size() );
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

    int i = _tabbed_record.length();

    _tabbed_record += value.substr( 0, max_field_length );

    if( strchr( value.c_str(), '\t' )
     || strchr( value.c_str(), '\n' ) )
    {
        for( i; i < _tabbed_record.length(); i++ )  
        {
            if( _tabbed_record[i] == '\t' )  _tabbed_record[i] = ' ';
            if( _tabbed_record[i] == '\n' )  _tabbed_record[i] = ' ';
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

                string log_filename = _job->_log.filename();

                if( _with_log  &&  !log_filename.empty()  &&  log_filename[0] != '*' )
                {
                    try {
                        copy_file( "file -b " + log_filename, 
                                   _spooler->_db.dbname() + " -table=" + _spooler->_history_tablename + " -blob='log' where \"id\"=" + as_string( _job->_task->_id ) );
                    }
                    catch( const exception& ) {}
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

                for( int i = 0; i < _extra_values.size(); i++ ) 
                {
                    try {
                        VARIANT* v = &_extra_values[i];
                        if( v->vt != VT_EMPTY  &&  v->vt != VT_NULL )
                        {
                            string s = variant_as_string(*v);
                            if( !variant_is_numeric(*v) )  s = sql_quoted(s);
                            stmt += ", " + _extra_names[i] + "=" + s;
                        }
                    }
                    catch( const exception& ) {}
                }

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
        append_tabbed( start? "" : Time::now().as_string(Time::without_ms) );
        append_tabbed( _job->_task->_cause );
        append_tabbed( _job->_task->_step_count );
        append_tabbed( _job->has_error()? 1 : 0 );
        append_tabbed( _job->_error.code() );
        append_tabbed( _job->_error.what() );
        append_tabbed( parameters );

        if( !start )
        {
            for( int i = 0; i < _extra_values.size(); i++ )  
            {
                string v;
            
                try {
                    v = variant_as_string( _extra_values[i] );
                } 
                catch( const exception& ) {}

                append_tabbed( v );
            }
        }

        _file.print( _tabbed_record + SYSTEM_NL );
        //zu langsam: _file.syncdata();
    }
}

//-------------------------------------------------------------------------------Job_history::start

void Job_history::start()
{
    _start_called = true;

    for( int i = 0; i < _extra_values.size(); i++ )  _extra_values[i].Clear();

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
    if( !_start_called )  return;
    _start_called = false;

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
                _spooler->_db.execute( "DELETE from " + _spooler->_history_tablename + " where \"id\"=" + as_string(_job->_task->_id) );
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

//---------------------------------------------------------------------Job_history::set_extra_field
    
void Job_history::set_extra_field( const string& name, const CComVariant& value )
{
    for( int i = 0; i < _extra_values.size(); i++ )
    {
        if( stricmp( _extra_names[i].c_str(), name.c_str() ) == 0 )  { _extra_values[i] = value; return; }
    }

    throw_xc( "SPOOLER-137", name );
}

//---------------------------------------------------------------------------Job_history::read_last

static string prepend_comma( const string& s )  { return ", " + s; }


xml::Element_ptr Job_history::read_tail( xml::Document_ptr doc, int n, bool with_log )
{
    xml::Element_ptr history_element;

    if( !_error )  
    {
        Transaction ta = &_spooler->_db;
        {
            Any_file sel;

            if( _use_file )
            {
                sel.open( "-in head -" + as_string(n) + " | select * order by id desc | -type=(" + _columns + ") tab -field-names | " + _filename );
            }
            else
            if( _use_db )
            {
                sel.open( "-in head -" + as_string(n) + " | " + _spooler->_db._db_name + 
                          "select id, spooler_id, job_name, start, end, cause, steps, error, error_code, error_text " +
                          join( "", zmap( prepend_comma, _extra_names ) ) +
                          " from " + _spooler->_history_tablename + " where job_name=" + sql_quoted(_job->name()) + " order by id desc" );
            }
            else
                throw_xc( "SPOOLER-136" );

            history_element = doc->createElement( "history" );
            dom_append_nl( history_element );

            const Record_type* type = sel.spec().field_type_ptr();
            Dynamic_area rec ( type->field_size() );
    
            while( !sel.eof() )
            {
                sel.get( &rec );
    
                xml::Element_ptr history_entry = doc->createElement( "history.entry" );
        
                for( int i = 0; i < type->field_count() - 1; i++ )
                {
                    history_entry->setAttribute( as_dom_string( type->field_descr_ptr(i)->name() ), as_dom_string( type->as_string( i, rec.byte_ptr() ) ) );
                }

                int id = type->field_descr_ptr("id")->as_int( rec.byte_ptr() );
                
                string param_xml = file_as_string( _spooler->_db._db_name + "-table=" + _spooler->_history_tablename + " -blob=parameters where id=" + as_string(id) );
                if( !param_xml.empty() )
                {
                    dom_append_nl( history_element );
                    try {
                        xml::Document_ptr par_doc = xml::Document_ptr( __uuidof(xml::DOMDocument30), NULL );
                        par_doc->loadXML( as_dom_string( param_xml ) );
                        history_entry->appendChild( par_doc->documentElement );
                    }
                    catch( const exception& ) {}
                    catch( const _com_error& ) {}
                }

                if( with_log )
                {
                    string log = file_as_string( _spooler->_db._db_name + "-table=" + _spooler->_history_tablename + " -blob=log where id=" + as_string(id) );
                    if( !log.empty() ) dom_append_text_element( history_entry, "log", log );
                }

                history_element->appendChild( history_entry );
                dom_append_nl( history_element );
            }

            sel.close();
        }
        ta.commit();
    }

    return history_element;
}

//-------------------------------------------------------------------------------------------------

} //namespace spooler
} //namespace sos

