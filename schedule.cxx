// $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#include "spooler.h"
#ifdef Z_SCHEDULE_DEVELEPMENT

namespace sos {
namespace scheduler {
namespace schedule {

//-------------------------------------------------------------------------------Schedule_subsystem
    
struct Schedule_subsystem : Schedule_subsystem_interface
{
                                Schedule_subsystem          ( Scheduler* );

    // Subsystem:
    void                        close                       ();
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
    bool                        subsystem_activate          ();


    // File_based_subsystem

    string                      xml_element_name            () const                                { return "schedule"; }
    string                      xml_elements_name           () const                                { return "schedules"; }
    void                        assert_xml_element_name     ( const xml::Element_ptr& ) const;
    string                      object_type_name            () const                                { return "Schedule"; }
    string                      filename_extension          () const                                { return ".schedule.xml"; }
  //string                      normalized_name             ( const string& name ) const            { return lcase( name ); }
    ptr<Schedule>               new_file_based              ();
    xml::Element_ptr            new_file_baseds_dom_element ( const xml::Document_ptr& doc, const Show_what& ) { return doc.createElement( "schedules" ); }


    // Job_subsystem_interface

    ptr<Schedule_folder>        new_schedule_folder         ( Folder* folder )                      { return Z_NEW( Schedule_folder( folder ) ); }



    // Ischedules

    //STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    //STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Schedules"; }
    //STDMETHODIMP            get_Schedule                    ( BSTR, spooler_com::Ischedule** );
    //STDMETHODIMP            get_Schedule_or_null            ( BSTR, spooler_com::Ischedule** );
    //STDMETHODIMP                Create_schedule             ( spooler_com::Ischedule** );
    //STDMETHODIMP                Add_schedule                ( spooler_com::Ischedule* );

  private:
    //static Class_descriptor     class_descriptor;
    //static const Com_method     _methods[];
};

//----------------------------------------chedule_subsystem_interface::Schedule_subsystem_interface

Schedule_subsystem_interface::Schedule_subsystem_interface( Scheduler* scheduler, Type_code t )   
: 
    file_based_subsystem<Schedule>( scheduler, this, t )
{
}

//-----------------------------------------------------------Schedule_subsystem::Schedule_subsystem
    
Schedule_subsystem::Schedule_subsystem( Scheduler* scheduler )
:
    Schedule_subsystem_interface( scheduler, type_schedule_subsystem )
{
}

//------------------------------------------------------------------------Schedule_subsystem::close
    
void Schedule_subsystem::close()
{
    set_subsystem_state( subsys_stopped );
    file_based_subsystem<Schedule>::close();
}

//---------------------------------------------------------Schedule_subsystem::subsystem_initialize

bool Schedule_subsystem::subsystem_initialize()
{
    file_based_subsystem<Schedule>::subsystem_initialize();
    set_subsystem_state( subsys_initialized );
    return true;
}

//---------------------------------------------------------------Schedule_subsystem::subsystem_load

bool Schedule_subsystem::subsystem_load()
{
    file_based_subsystem<Schedule>::subsystem_load();
    set_subsystem_state( subsys_loaded );
    return true;
}

//-----------------------------------------------------------Schedule_subsystem::subsystem_activate

bool Schedule_subsystem::subsystem_activate()
{
    set_subsystem_state( subsys_active );
    file_based_subsystem<Schedule>::subsystem_activate();
    return true;
}

//------------------------------------------------------Schedule_subsystem::assert_xml_element_name

void Schedule_subsystem::assert_xml_element_name( const xml::Element_ptr& element ) const
{
    if( !element.nodeName_is( "run_time" ) )  File_based_subsystem::assert_xml_element_name( e );
}

//-----------------------------------------------------Schedule_subsystem<Schedule>::new_file_based

ptr<Schedule> Schedule_subsystem::new_file_based()
{
    return Z_NEW( Schedule( this ) );
}

//---------------------------------------------------------------------Schedule_folder::execute_xml

xml::Element_ptr Schedule_subsystem::execute_xml( Command_processor* command_processor, const xml::Element_ptr& element, const Show_what& show_what )
{
    xml::Element_ptr result;

    if( element.nodeName_is( "schedule" )  ||
        element.nodeName_is( "run_time" )     )  spooler()->root_folder()->schedule_folder()->add_or_replace_file_based_xml( element );
    else
    //if( string_begins_with( element.nodeName(), "schedule." ) ) 
    //{
    //    schedule( Absolute_path( root_path, element.getAttribute( "schedule" ) ) )->execute_xml( element, show_what );
    //}
    //else
        z::throw_xc( "SCHEDULER-113", element.nodeName() );

    return command_processor->_answer.createElement( "ok" );
}

//-------------------------------------------------------------------------------Schedule::_methods

const Com_method Schedule::_methods[] =
{
#ifdef COM_METHOD
    COM_PROPERTY_GET( Schedule,  1, Java_class_name, VT_BSTR    , 0 ),
    COM_PROPERTY_PUT( Schedule,  2, Xml            ,              0, VT_BSTR ),
#endif
    {}
};

//-------------------------------------------------------------------------------Schedule::Schedule

Schedule::Schedule( Schedule_use* use, File_based* source_file_based )
:
    Idispatch_implementation( &class_descriptor ),
    _zero_(this+1),
    _spooler(host_object->_spooler),
    _host_object(host_object),
    _source_file_based(source_file_based),
    _holidays(host_object->_spooler),
    _months(12)
{
    _log = use->log();  //Z_NEW( Prefix_log( host_object ) );

    if( _host_object->scheduler_type_code() == Scheduler_object::type_order )
    {
        _once = true;
    }

    _dom.create();
    _dom.appendChild( _dom.createElement( "run_time" ) );       // <run_time/>
}

//------------------------------------------------------------------------------Schedule::~Schedule
    
Schedule::~Schedule()
{
    try
    {
        close();
    }
    catch( exception& x ) { Z_LOG2( "scheduler", Z_FUNCTION << ": " << x.what() ); }
}

//----------------------------------------------------------------------------------Schedule::close

void Schedule::close()
{
    _host_object = NULL;
}

//-------------------------------------------------------------------------Schedule::QueryInterface

STDMETHODIMP Schedule::QueryInterface( const IID& iid, void** result )
{
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, spooler_com::Irun_time           , result );
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, spooler_com::Ihas_java_class_name, result );
    Z_IMPLEMENT_QUERY_INTERFACE( this, iid, IDispatch                        , result );

    return Idispatch_implementation::QueryInterface( iid, result );
}

//--------------------------------------------------------------------Schedule::can_be_replaced_now

bool Schedule::can_be_replaced_now()
{
    return replacement()  &&  
           replacement()->file_based_state() == File_based::s_initialized &&
           _holder_set.empty();
}

//--------------------------------------------------------------------------Schedule::on_initialize

bool Schedule::on_initialize()
{
    return true;
}

//--------------------------------------------------------------------------------Schedule::on_load

bool Schedule::on_load()
{
    return true;
}

//----------------------------------------------------------------------------Schedule::on_activate

bool Schedule::on_activate()
{
    return true;
}

//---------------------------------------------------------------------Schedule::can_be_removed_now

bool Schedule::can_be_removed_now()
{
    return is_to_be_removed()  &&  _in_use_set.empty();
}

//----------------------------------------------------------------------Schedule::prepare_to_remove

void Schedule::prepare_to_remove()
{
    if( !is_to_be_removed() )
    {
    }

    My_file_based::prepare_to_remove();
}

//---------------------------------------------------------------------------Schedule::remove_error

zschimmer::Xc Schedule::remove_error()
{
    //return zschimmer::Xc( "SCHEDULER-886", string_from_holders() );
}

//---------------------------------------------------------------------Schedule::prepare_to_replace

void Schedule::prepare_to_replace()
{
    assert( replacement() );
}

//-------------------------------------------------------------------------Schedule::on_replace_now

Schedule* Schedule::on_replace_now()
{
    set_replacement( NULL );

    return this;
}

//--------------------------------------------------------------------------------Schedule::put_Xml

STDMETHODIMP Schedule::put_Xml( BSTR xml )
{
    Z_COM_IMPLEMENT( set_xml( string_from_bstr( xml ) ) );

    // *** this ist ungültig ***
}

//----------------------------------------------------------------------------Schedule::operator ==

bool Schedule::operator == ( const Schedule& r )
{
    return dom_document().xml() == r.dom_document().xml();
}

//----------------------------------------------------------------------------Schedule::set_default

void Schedule::set_default()
{
    Day default_day;

    default_day.set_default();
    _weekday_set.fill_with_default( default_day );
    //for( int i = 0; i < 12; i++ )
    //{
    //    if( !_months[ i ] )
    //    {
    //        _months[ i ] = Z_NEW( Month() );
    //        _months[ i ]->set_default();
    //    }
    //}
}

//--------------------------------------------------------------------------------Schedule::set_xml

void Schedule::set_xml( const string& xml )
{
    //_xml = xml;

    //set_dom( _spooler->_dtd.validate_xml( xml ).documentElement() );
    xml::Document_ptr doc ( xml );
    if( _spooler->_validate_xml )  _spooler->_schema.validate( xml::Document_ptr( xml ) );

    if( !_host_object  ||  _host_object->scheduler_type_code() != Scheduler_object::type_order )  z::throw_xc( "SCHEDULER-352" );
    dynamic_cast<Order*>( _host_object ) -> set_run_time( doc.documentElement() );

    // *** this ist ungültig ***
}

//--------------------------------------------------------------------------Schedule::call_function

Period Schedule::call_function( Scheduler_use* schedule_use, const Time& requested_beginning )
{
    // Die Funktion sollte keine Zeit in der wiederholten Stunde nach Ende der Sommerzeit liefern.


    Period result;

    if( !_start_time_function_error )
    {
        try
        {

            string date_string = requested_beginning.as_string( Time::without_ms );
            string param2      = schedule_use->name_for_function();
                                 //!_host_object? "" :
                                 //schedule_use->scheduler_type_code() == Scheduler_object::type_order? dynamic_cast<Order*>( schedule_use )->string_id() :
                                 //schedule_use->scheduler_type_code() == Scheduler_object::type_job  ? dynamic_cast<Job*  >( schedule_use )->name()
                                                                                                    : "";
            Scheduler_script* scheduler_script = _spooler->scheduler_script_subsystem()->default_scheduler_script();
            string            function_name    = _start_time_function;
            if( scheduler_script ->module()->kind() == Module::kind_java )  function_name += "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;";

            Variant v = scheduler_script ->module_instance()->call( function_name, date_string, param2 );
            
            if( !v.is_null_or_empty_string() )
            {
                Time t;
                if( variant_is_numeric( v ) )  t = (time_t)v.as_int64();
                else
                if( v.vt == VT_DATE         )  t = seconds_since_1970_from_com_date( V_DATE( &v ) );
                else                           
                                               t.set_datetime( v.as_string() );

                if( t < requested_beginning )
                {
                    //if( _log )  _log->warn( message_string( "SCHEDULER-394", _start_time_function, t, requested_beginning ) );
                    z::throw_xc( "SCHEDULER-394", _start_time_function, t, requested_beginning );
                }
                else 
                if( !t.is_never() ) 
                {
                    result.set_single_start( t );
                }
            }
        }
        catch( exception& x )
        {
            _start_time_function_error = true;
            z::throw_xc( "SCHEDULER-393", _start_time_function, x );
        }
    }

    return result;
}

//--------------------------------------------------------------------Schedule::month_index_by_name

int Schedule::month_index_by_name( const string& mm )
{
    string m = lcase( mm );

    for( int i = 0; i < 12; i++ )
    {
        if( m == month_names[ i ] )  return i;
    }

    int result = as_int( m );
    if( result >= 1  &&  result <= 12 )  return result - 1;

    z::throw_xc( "INVALID-MONTH", mm );      // Sollte wegen XML-Schemas nicht passieren
}

//-----------------------------------------------------------------Schedule::month_indices_by_names

list<int> Schedule::month_indices_by_names( const string& mm )
{
    list<int>      result;
    vector<string> names  = vector_split( " +", mm );

    Z_FOR_EACH( vector<string>, names, it )
    {
        result.push_back( month_index_by_name( *it ) );
    }

    return result;
}

//--------------------------------------------------------------------------------Schedule::set_dom

void Schedule::set_dom( const xml::Element_ptr& element )
{
    if( !element )  return;

    if( _modified_event_handler )  _modified_event_handler->on_before_modify_run_time();


    Sos_optional_date_time  dt;
    Period                  default_period;
    Day                     default_day;
    bool                    period_seen    = false;


    _dom.create();
    _dom.appendChild( _dom.clone( element ) );

    _set = true;

    _once = element.bool_getAttribute( "once", _once );
    if( _host_object  &&  _host_object->scheduler_type_code() == Scheduler_object::type_order  &&  !_once )  z::throw_xc( "SCHEDULER-220", "once='no'" );

    _start_time_function = element.getAttribute( "start_time_function" );

    default_period.set_dom( element );
    default_day = default_period;

    bool a_day_set = false;


    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "period" ) )
        {
            if( !period_seen )  period_seen = true, default_day = Day();
            default_day.add( Period( e, &default_period ) );
        }
        else
        if( e.nodeName_is( "at" ) )
        {
            a_day_set = true;
            string at = e.getAttribute( "at" );
            if( at == "now" )  _once = true;   // "now" wirkt nicht in _at_set, weil der Zeitpunkt gleich verstrichen sein wird
            Time at_time;
            at_time.set_datetime( at );
            _at_set.add( at_time );
        }
        else
        if( e.nodeName_is( "date" ) )
        {
            a_day_set = true;
            dt.assign( e.getAttribute( "date" ) );
            if( !dt.time_is_zero() )  z::throw_xc( "SCHEDULER-208", e.getAttribute( "date" ) );
            Date date;
            date._day_nr = (int)( dt.as_time_t() / (24*60*60) );
            date._day.set_dom_periods( e, &default_day, &default_period );
            _date_set._date_set.insert( date );
        }
        else
        if( e.nodeName_is( "weekdays" ) )
        {
            a_day_set = true;
            _weekday_set.set_dom( e, &default_day, &default_period );
        }
        else
        if( e.nodeName_is( "monthdays" ) )
        {
            a_day_set = true;
            _monthday_set.set_dom( e, &default_day, &default_period );
        }
        else
        if( e.nodeName_is( "ultimos" ) )
        {
            a_day_set = true;
            _ultimo_set.set_dom( e, &default_day, &default_period );
        }
        else
        if( e.nodeName_is( "month" ) )
        {
            a_day_set = true;

            ptr<Month> month = Z_NEW( Month() );
            month->set_dom( e );

            list<int> months = month_indices_by_names( e.getAttribute( "month" ) );
            
            Z_FOR_EACH( list<int>, months, it )
            {
                if( _months[ *it ] )  z::throw_xc( "SCHEDULER-443", month_names[ *it ] );
                _months[ *it ] = month;
            }
        }
        if( e.nodeName_is( "holidays" ) )
        {
            _holidays.clear();
            _holidays.set_dom( _source_file_based, e );
        }
        else
        if( e.nodeName_is( "holiday" ) )
        {
            _holidays.set_dom( _source_file_based, e );
        }
    }


    if( !a_day_set )  _weekday_set.fill_with_default( default_day );
    //if( !a_day_is_set ) 
    //{
    //    ptr<Month> default_month = Z_NEW( Month() );
    //    default_month->_weekday_set.fill_with_default( default_day );
    //    for( int i = 0; i < 12; i++ )  if( !_months[ i ] )  _months[ i ] = default_month;
    //}

    if( _modified_event_handler )  _modified_event_handler->run_time_modified_event();
}

//----------------------------------------------------------------------------Schedule::dom_element

xml::Element_ptr Schedule::dom_element( const xml::Document_ptr& document ) const
{
    return document.clone( _dom.documentElement() );
}

//---------------------------------------------------------------------------Schedule::dom_document

xml::Document_ptr Schedule::dom_document() const
{
    xml::Document_ptr document;

    document.create();
    document.appendChild( dom_element( document ) );

    return document;
}

//---------------------------------------------------------------------------Schedule::first_period

Period Schedule::first_period( const Time& beginning_time )
{
    return next_period( beginning_time );
}

//------------------------------------------------------------------------------Schedule::is_filled

bool Schedule::is_filled() const
{
    bool result = _at_set  .is_filled() 
               || _date_set.is_filled();

    if( !result )
    {
        for( int i = 0; i < 12; i++ )
        {
            result = _months[ i ]  &&  _months[ i ]->is_filled();
            if( result )  break;
        }
    }

    return result;
}

//----------------------------------------------------------------------------Schedule::next_period

Period Schedule::next_period( const Time& beginning_time, With_single_start single_start )
{
    Period result;
    Time   tim   = beginning_time;
    bool   is_no_function_warning_logged = false; 
    Period last_function_result;
    
    last_function_result.set_single_start( 0 );

    while( tim < Time::never )
    {
        bool something_called = false;

        if( _start_time_function != ""  &&  single_start & ( wss_next_any_start | wss_next_single_start ) )
        {
            if( _spooler->scheduler_script_subsystem()->subsystem_state() != subsys_active  &&  _log  &&  !is_no_function_warning_logged )
            {
                _log->warn( message_string( "SCHEDULER-844", _start_time_function, Z_FUNCTION ) );
                is_no_function_warning_logged = true;
            }
            else
            if( last_function_result.begin() < tim )
            {
                try
                {
                    last_function_result = min( result, call_function( tim ) );
                    result = last_function_result;
                }
                catch( exception& x )
                {
                    _log->error( x.what() );
                    _log->error( message_string( "SCHEDULER-398", _start_time_function ) );
                }
            }

            something_called = true;
        }

        if( !tim.is_never()  ||  is_filled() )
        {
            if( _at_set      .is_filled() )  result = min( result, _at_set      .next_period( tim, single_start ) );
            if( _date_set    .is_filled() )  result = min( result, _date_set    .next_period( tim, single_start ) );

            int m = tim.month_nr() - 1;
            if( _months[ m ] )
            {
                result = min( result, _months[ m ]->next_period( tim, single_start ) );
            }
            else
            {
                if( _weekday_set .is_filled() )  result = min( result, _weekday_set .next_period( tim, single_start ) );
                if( _monthday_set.is_filled() )  result = min( result, _monthday_set.next_period( tim, single_start ) );
                if( _ultimo_set  .is_filled() )  result = min( result, _ultimo_set  .next_period( tim, single_start ) );
            }

            something_called = true;
        }

        if( !something_called )  break;                         // <run_time> ist leer

        if( result.begin() != Time::never )
        {
            if( !_holidays.is_included( result.begin() ) )  break;     // Gefundener Zeitpunkt ist kein Feiertag? Dann ok!

            // Feiertag
            tim    = result.begin().midnight() + 24*60*60;      // Nächsten Tag probieren
            result = Period();
        }
        else
        {
            tim = tim.midnight() + 24*60*60;                    // Keine Periode? Dann nächsten Tag probieren
        }

        if( tim >= beginning_time + 366*24*60*60 )  break;     // Längstens ein Jahr ab beginning_time voraussehen
    }


    return result;
}

//----------------------------------------------------------------------Schedule::next_single_start

Time Schedule::next_single_start( const Time& time )
{ 
    Period period = next_period( time, wss_next_single_start );
    
    return !period.absolute_repeat().is_never()? period.next_repeated( time )
                                               : period.begin();
}

//-------------------------------------------------------------------------Schedule::next_any_start

Time Schedule::next_any_start( const Time& time )
{ 
    Period period = next_period( time, wss_next_any_start );

    return !period.absolute_repeat().is_never()? period.next_repeated( time )
                                               : period.begin();
}

//-----------------------------------------------------------Schedule::append_calendar_dom_elements

void Schedule::append_calendar_dom_elements( const xml::Element_ptr& element, Show_calendar_options* options )
{
    Time t          = options->_from;
    Time last_begin;

    //bool check_for_once = _once;

    while( options->_count < options->_limit )
    {
        Period period = next_period( t, wss_next_any_start );  
        if( period.begin() >= options->_before  ||  period.begin() == Time::never )  break;

        //if( period._start_once  ||
        //    period._single_start  ||     
        //    _host_object->scheduler_type_code() == Scheduler_object::type_order ||
        //    check_for_once 
        {
            //check_for_once = false;

            if( period.begin() >= options->_from  &&  period.begin() != last_begin )
            {
                element.appendChild( period.dom_element( element.ownerDocument() ) );
                options->_count++;

                last_begin = period.begin();
            }
        }

        t = period._single_start? period.begin() + 1 : period.end();
    }
}

//------------------------------------------------------------------------------Period::set_default

void Period::set_default()
{
    _begin           = 0;             // = "00:00:00";
    _end             = 24*60*60;      // = "24:00:00";
    _repeat          = Time::never;
    _absolute_repeat = Time::never;
}

//-------------------------------------------------------------------------Period::set_single_start

void Period::set_single_start( const Time& t )
{
    _begin           = t;
    _end             = t;
    _repeat          = Time::never;
    _absolute_repeat = Time::never;
    _single_start    = true;
    _let_run         = true;
}

//----------------------------------------------------------------------------------Period::set_dom

void Period::set_dom( const xml::Element_ptr& element, Period::With_or_without_date w, const Period* deflt )
{
    if( !element )  return;

    Sos_optional_date_time dt;

    if( deflt )  *this = *deflt;

    //if( _application == application_order  &&  element.getAttribute( "let_run" ) != "" )  z::throw_xc( "SCHEDULER-220", "let_run='yes'" );
    _let_run = element.bool_getAttribute( "let_run", _let_run );

    string single_start = element.getAttribute( "single_start" );
    if( !single_start.empty() )
    {
        dt.set_time( single_start );
        set_single_start( Time( dt ) );
    }
    else
    {
        string begin = element.getAttribute( "begin", "00:00:00" );
        if( !begin.empty() )
        {
            if( w == with_date )  dt = begin;
                            else  dt.set_time( begin );
            _begin = dt;
        }

        string repeat = element.getAttribute( "repeat" );
        if( !repeat.empty() )
        {
            if( repeat.find( ':' ) != string::npos )
            {
                Sos_optional_date_time dt;
                dt.set_time( repeat );
                _repeat = dt.time_as_double();
            }
            else
                _repeat = as_double( repeat );
        }

        if( _repeat == 0 )  _repeat = Time::never;


        string absolute_repeat = element.getAttribute( "absolute_repeat" );
        if( !absolute_repeat.empty() )
        {
            if( absolute_repeat.find( ':' ) != string::npos )
            {
                Sos_optional_date_time dt;
                dt.set_time( absolute_repeat );
                _absolute_repeat = dt.time_as_double();
            }
            else
                _absolute_repeat = as_double( absolute_repeat );

            if( _absolute_repeat == 0 )  z::throw_xc( "SCHEDULER-441", element.nodeName(), "absolute_repeat", "0" );
            if( !_absolute_repeat.is_never() )  _repeat = Time::never;
          //if( !_repeat.is_never()   )  z::throw_xc( "SCHEDULER-442", "absolute_repeat", "repeat" );
        }
    }

    string end = element.getAttribute( "end" , "24:00:00" );
    if( !end.empty() )
    {
        if( w == with_date )  dt = end;
                        else  dt.set_time( end );
        _end = dt;
    }

    _start_once = element.bool_getAttribute( "start_once", _start_once );   // Für Joacim Zschimmer
    //Wird das schon benutzt? Ist nicht berechnet.  if( _start_once  &&  !_spooler->_zschimmer_mode )  z::throw_xc( Z_FUNCTION, "Attribute start_once is not supported" );

    check( w );
}

//------------------------------------------------------------------------------------Period::check

void Period::check( With_or_without_date w ) const
{
    if( _begin < 0      )  goto FEHLER;
    if( _begin > _end   )  goto FEHLER;
    if( w == without_date )  if( _end > 24*60*60 )  goto FEHLER;
    return;

  FEHLER:
    z::throw_xc( "SCHEDULER-104", _begin.as_string(), _end.as_string() );
}

//-------------------------------------------------------------------------------Period::is_comming

bool Period::is_comming( const Time& time_of_day, With_single_start single_start ) const
{
    bool result;

    if( single_start & wss_next_period  &&  !_single_start  &&  time_of_day < _end )  result = true;
                                                                         // ^-- Falls time_of_day == previous_period.end(), sonst Schleife!
    else
    if( single_start & wss_next_begin  &&  !_single_start  &&  time_of_day <= _begin )  result = true;
                                                                        // ^ Falls _begin == 00:00 und time_of_day == 00:00 (Beginn des nächsten Tags)
    else
    if( single_start & wss_next_single_start  &&  _single_start  &&  time_of_day <= _begin )  result = true;
                                                                              // ^ Falls _begin == 00:00 und time_of_day == 00:00 (Beginn des nächsten Tags)
    else
    if( single_start & wss_next_any_start  &&  ( ( _single_start || has_repeat_or_once() ) && time_of_day <= _begin ) )  result = true;
                                                                                                       // ^ Falls _begin == 00:00 und time_of_day == 00:00 (Beginn des nächsten Tags)
    else
    if( single_start & ( wss_next_any_start | wss_next_single_start )  &&  !_absolute_repeat.is_never()  &&  !next_absolute_repeated( time_of_day, 0 ).is_never() )  result = true;
                                                                                                                                                // ^ Falls zwei Perioden direkt aufeinander folgen
    else
        result = false;

    //Z_LOG2( "joacim", *this << ".is_comming(" << time_of_day << ',' << (int)single_start << ") ==> " << result << "\n" );

    return result;
}

//---------------------------------------------------------------------------------Period::next_try

Time Period::next_try( const Time& t )
{
    Time result = t;

    if( result >= end() )  result = Time::never;

    return result;
}

//----------------------------------------------------------------------------Period::next_repeated

Time Period::next_repeated( const Time& t ) const
{
    Time result = next_repeated_allow_after_end( t );

    return result < _end? result : Time::never;  
}

//----------------------------------------------------------------------------Period::next_repeated

Time Period::next_repeated_allow_after_end( const Time& t ) const
{
    Time result = Time::never;
    
    if( !_repeat.is_never() )
    {
        result = t < _begin? _begin 
                           : Time( t + _repeat );
    }
    else
    if( !_absolute_repeat.is_never() )
    {
        result = next_absolute_repeated( t, 1 );
    }

    return result;  
}
//-------------------------------------------------------------------Period::next_absolute_repeated

Time Period::next_absolute_repeated( const Time& t, int next ) const
{
    assert( next == 0  ||  next == 1 );
    assert( !_absolute_repeat.is_never() );


    Time result = Time::never;

    if( t < _begin )
    {
        result = _begin;
    }
    else
    {
        int n = (int)( ( t - _begin ) / _absolute_repeat );
        result = _begin + ( n + 1 ) * _absolute_repeat;
        if( result == t + _absolute_repeat  &&  next == 0 )  result = t;
    }

    assert( next == 0? result >= t : result > t );

    return result < _end? result : Time::never;  
}

//------------------------------------------------------------------------------------Period::print

void Period::print( ostream& s ) const
{
    s << "Period(" << _begin << ".." << _end;
    if( _single_start )  s << " single_start";
    if( !_repeat.is_never() )  s << " repeat=" << _repeat;
    if( !_absolute_repeat.is_never() )  s << " absolute_repeat=" << _absolute_repeat;
    if( _let_run )  s << " let_run";
    s << ")";
}

//------------------------------------------------------------------------------Period::dom_element

xml::Element_ptr Period::dom_element( const xml::Document_ptr& dom_document ) const
{
    xml::Element_ptr result;

    if( _single_start )
    {
        result =  dom_document.createElement( "at" );
        result.setAttribute( "at", _begin.xml_value( Time::without_ms ) );
    }
    else
    {
        result =  dom_document.createElement( "period" );
        result.setAttribute( "begin", _begin.xml_value( Time::without_ms ) );
        result.setAttribute( "end"  , _end  .xml_value( Time::without_ms ) );
        if( _let_run )  result.setAttribute( "let_run", "yes" );

        if( _repeat          != Time::never )  result.setAttribute( "repeat"         , _repeat.as_time_t() );
        if( _absolute_repeat != Time::never )  result.setAttribute( "absolute_repeat", _absolute_repeat.as_time_t() );
    }

    return result;
}

//-----------------------------------------------------------------------------------Period::to_xml

string Period::to_xml() const
{
    S result;

    result << "<period";
    result << " begin=\"" << _begin.as_string() << "\"";
    result << " end=\""   << _end  .as_string() << "\"";
    if( !_repeat         .is_never() )  result << " repeat=\""          << _repeat         .as_string() << "\"";
    if( !_absolute_repeat.is_never() )  result << " absolute_repeat=\"" << _absolute_repeat.as_string() << "\"";
    result << "/>";

    return result;
}

//---------------------------------------------------------------------------------Period::obj_name

string Period::obj_name() const
{
    ostrstream s;
    print( s );
    s << '\0';
    return s.str();
}

//-----------------------------------------------------------------------------Day::set_dom_periods

void Day::set_dom_periods( const xml::Element_ptr& element, const Day* default_day, const Period* default_period )
{
    if( !element )  return;

    if( default_day )  _period_set = default_day->_period_set;

    bool first = true;

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( first )  first = false, _period_set.clear();
        _period_set.insert( Period( e, default_period ) );
    }
}

//---------------------------------------------------------------------------------Day::set_default

void Day::set_default()
{
    Period period;
    period.set_default();

    _period_set.clear();
    _period_set.insert( period );
}

//---------------------------------------------------------------------------------------Day::merge

void Day::merge( const Day& other_day )
{
    _period_set.insert( other_day._period_set.begin(), other_day._period_set.end() );
}

//------------------------------------------------------------------------------------Day::has_time

bool Day::has_time( const Time& time_of_day )
{
    FOR_EACH( Period_set, _period_set, it )
    {
        if( it->begin() >= time_of_day || it->end() > time_of_day )  return true;
    }

    return false;
}

//--------------------------------------------------------------------------------Day::next_period_

Period Day::next_period_( const Time& time_of_day, With_single_start single_start ) const
{
    FOR_EACH_CONST( Period_set, _period_set, it )
    {
        if( it->is_comming( time_of_day, single_start ) )  return *it;
    }

    return Period();
}

//---------------------------------------------------------------------------------------Day::print

void Day::print( ostream& s ) const
{
    s << "Day(";

    FOR_EACH_CONST( Period_set, _period_set, it )
    {
        s << *it << ' ';
    }

    s << ")";
}

//---------------------------------------------------------------------------------Day_set::Day_set

Day_set::Day_set( int minimum, int maximum )
: 
    _zero_(this+1), 
    _minimum(minimum), 
    _maximum(maximum),
    _days(1+maximum)
{
} 

//---------------------------------------------------------------------------------Day_set::Day_set
    
Day_set::Day_set( int minimum, int maximum, const xml::Element_ptr& e )  
: 
    _zero_(this+1), 
    _minimum(minimum), 
    _maximum(maximum),
    _days(1+maximum)
{ 
    set_dom(e); 
}

//-----------------------------------------------------------------------------------Day_set::print

void Day_set::print( ostream& s ) const
{
    for( int i = 0; i < _days.size(); i++ )
    {
        if( _days[i] )  s << ' ' << i << "=" << *_days[i];
    }
}

//-------------------------------------------------------------------Weekday_set::fill_with_default

void Weekday_set::fill_with_default( const Day& default_day )
{ 
    ptr<Day> day = Z_NEW( Day( default_day ) );

    for( int i = 0; i < 7; i++ )  _days[i] = day;

    _is_day_set_filled = true;
}

//--------------------------------------------------------------------------------Weekday_set::next

Period Weekday_set::next_period( const Time& tim, With_single_start single_start )
{
    if( _is_day_set_filled )
    {
        Time time_of_day = tim.time_of_day();
        int  day_nr      = tim.day_nr();
        int  weekday     = weekday_of_day_number( day_nr );

        for( int i = weekday; i <= weekday+7; i++ )
        {
            if( const Day* day = _days[ i % 7 ] )
            {
                Period period = day->next_period( time_of_day, single_start );
                if( !period.empty() )  return day_nr*(24*60*60) + period;
            }

            day_nr++;
            time_of_day = 0;
        }
    }

    return Period();
}

//----------------------------------------------------------------------------Monthday_set::set_dom

void Monthday_set::set_dom( const xml::Element_ptr& monthdays_element, const Day* default_day, const Period* default_period )
{
    if( monthdays_element )
    {
        DOM_FOR_EACH_ELEMENT( monthdays_element, element )
        {
            if( element.nodeName_is( "day" ) )
            {
                Day       my_default_day ( element, default_day, default_period );
                list<int> day_numbers    = get_day_numbers( element.getAttribute( "day" ) );
                ptr<Day>  day            = Z_NEW( Day( element, &my_default_day, default_period ) );
                
                Z_FOR_EACH( list<int>, day_numbers, d )
                {
                    if( !_days[ *d ] )  _days[ *d ] = Z_NEW( Day( *day ) );
                                  else  _days[ *d ]->merge( *day );
                }

                _is_day_set_filled = true;
            }
            else
            if( element.nodeName_is( "weekday" ) )
            {
                Day       my_default_day ( element, default_day, default_period );
                list<int> weekdays       = get_weekday_numbers( element.getAttribute( "day" ) );
                int       which          = element.int_getAttribute( "which" );

                if( which == 0  ||  abs( which ) > max_weekdays_per_month )  z::throw_xc( Z_FUNCTION, S() << "Invalid value for which=" << which );    // XML-Schema hat schon geprüft

                Month_weekdays* m   = which > 0? &_month_weekdays : &_reverse_month_weekdays;
                ptr<Day>        day = Z_NEW( Day( element, &my_default_day, default_period ) );
                
                Z_FOR_EACH( list<int>, weekdays, w )
                {
                    if( m->day( abs(which), *w ) )  z::throw_xc( "SCHEDULER-446", *w, which );
                    m->day( abs(which), *w ) = day;
                }

                m->_is_filled = true;
            }
        }
    }
}

//------------------------------------------------------------------------Monthday_set::next_period

Period Monthday_set::next_period( const Time& tim, With_single_start single_start )
{
    Period result;

    if( is_filled() )
    {
        Time                    time_of_day = tim.time_of_day();
        int                     day_nr      = tim.day_nr();
        Sos_optional_date_time  date        = tim.as_time_t();
        int                     weekday     = weekday_of_day_number( day_nr );

        for( int i = 0; i < 31; i++ )
        {
            Period period;

            if( _is_day_set_filled )
            {
                if( const Day* day = _days[ date.day() ] )
                {
                    period = day->next_period( time_of_day, single_start );
                }
            }

            if( _month_weekdays._is_filled )
            {
                int which =  1 + ( date.day() - 1 ) / 7;

                if( Day* day = _month_weekdays.day( which, weekday ) )
                {
                    Period period2 = day->next_period( time_of_day, single_start );
                    if( period > period2 )  period = period2;
                }
            }

            if( _reverse_month_weekdays._is_filled )
            {
                int reverse_which = 1 + ( last_day_of_month( date ) - date.day() ) / 7;

                if( Day* day = _reverse_month_weekdays.day( reverse_which, weekday ) )
                {
                    Period period3 = day->next_period( time_of_day, single_start );
                    if( period > period3 )  period = period3;
                }
            }

            if( !period.empty() )
            {
                result = day_nr*(24*60*60) + period;
                break;
            }

            day_nr++;
            date.add_days(1);
            if( ++weekday == 7 )  weekday = 0;
            time_of_day = 0;
        }
    }

    return result;
}

//----------------------------------------------------------------Monthday_set::Month_weekdays::day

ptr<Day>& Monthday_set::Month_weekdays::day( int which, int weekday_number ) 
{ 
    assert( which != 0  && which <= max_weekdays_per_month );
    assert( weekday_number >= 0  &&  weekday_number < 7 );

    int index = 7 * ( which - 1 ) + weekday_number;

    return _days[ index ]; 
}

//--------------------------------------------------------------------------Ultimo_set::next_period

Period Ultimo_set::next_period( const Time& tim, With_single_start single_start )
{
    if( _is_day_set_filled )
    {
        Time     time_of_day = tim.time_of_day();
        int      day_nr      = tim.day_nr();
        Sos_date date        = Sos_optional_date_time( (time_t)tim );

        for( int i = 0; i < 31; i++ )
        {
            if( const Day* day = _days[ last_day_of_month( date ) - date.day() ] )
            {
                const Period& period = day->next_period( time_of_day, single_start );
                if( !period.empty() )  return day_nr*(24*60*60) + period;
            }

            day_nr++;
            date.add_days(1);
            time_of_day = 0;
        }
    }

    return Period();
}

//--------------------------------------------------------------------------------Holidays::set_dom

void Holidays::set_dom( File_based* source_file_based, const xml::Element_ptr& e, int include_nesting )
{
    if( e.nodeName_is( "holidays" ) )
    {
        DOM_FOR_EACH_ELEMENT( e, e2 )
        {
            if( e2.nodeName_is( "holiday" ) )
            {
                Sos_optional_date_time dt;
                dt.assign( e2.getAttribute( "date" ) );
                include( dt.as_time_t() );
            }
            else
            if( e2.nodeName_is( "include" ) )
            {
                if( include_nesting >= max_include_nesting )  z::throw_xc( "SCHEDULER-390", max_include_nesting, "<holidays>" );

                Include_command include_command ( _spooler, source_file_based, e2, _spooler->_configuration_file_path.directory() );

                try
                {
                    string xml_text = include_command.read_content();
                    Z_LOG2( "scheduler", Z_FUNCTION << "  " << xml_text << "\n" );

                    xml::Document_ptr doc;
                    doc.load_xml( xml_text );
                    if( _spooler->_validate_xml )  _spooler->_schema.validate( doc );

                    if( !doc.documentElement() )  z::throw_xc( "SCHEDULER-239", "holidays" );

                    set_dom( source_file_based, doc.documentElement(), include_nesting+1 );
                }
                catch( exception& x )
                {
                    z::throw_xc( "SCHEDULER-399", include_command.obj_name(), x );
                }
            }
            else
                z::throw_xc( "SCHEDULER-319", e2.nodeName(), e.nodeName() );
        }
    }
    else
    if( e.nodeName_is( "holiday" ) )
    {   
        Sos_optional_date dt;
        dt.assign( e.getAttribute( "date" ) );
        include( dt.as_time_t() );
    }
    else
        z::throw_xc( "SCHEDULER-319", e.nodeName(), Z_FUNCTION );
}

//--------------------------------------------------------------------------------------Date::print

void Date::print( ostream& s ) const
{
    Sos_optional_date dt;
    dt.set_time_t( _day_nr * (24*60*60) );

    s << "Date(" << dt << " " << _day << ')';
}

//----------------------------------------------------------------------------Date_set::next_period

Period Date_set::next_period( const Time& tim, With_single_start single_start )
{
    Time     time_of_day = tim.time_of_day();
    int      day_nr      = tim.day_nr();

    FOR_EACH( set<Date>, _date_set, it )
    {
        const Date& date = *it;

        if( date._day_nr >= day_nr )
        {
            const Period& period = date._day.next_period( date._day_nr == day_nr? time_of_day : Time(0), single_start );
            if( !period.empty() )  return date._day_nr*(24*60*60) + period;
        }
    }

    return Period();
}

//---------------------------------------------------------------------------------Date_set::print

void Date_set::print( ostream& s ) const
{
    s << "Date_set(";

    FOR_EACH_CONST( set<Date>, _date_set, it )
    {
        s << *it << " ";
    }

    s << ")";
}

//------------------------------------------------------------------------------At_set::next_period

Period At_set::next_period( const Time& tim, With_single_start single_start )
{
    if( single_start & wss_next_single_start )
    {
        FOR_EACH( set<Time>, _at_set, it )
        {
            const Time& time = *it;

            if( time >= tim )
            {
                Period result;
                
                result._begin           = time;
                result._end             = time;
                result._single_start    = true;
                result._let_run         = true;
                result._repeat          = Time::never;
                result._absolute_repeat = Time::never;

                return result;
            }
        }
    }

    return Period();
}

//---------------------------------------------------------------------------------Date_set::print

void At_set::print( ostream& s ) const
{
    s << "At_set(";

    FOR_EACH_CONST( set<Time>, _at_set, it )
    {
        s << *it << " ";
    }

    s << ")";
}

//---------------------------------------------------------------------------------Day_set::set_dom

void Day_set::set_dom( const xml::Element_ptr& element, const Day* default_day, const Period* default_period )
{
    if( !element )  return;

    //Period my_default_period ( element, default_period );

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "day" ) )
        {
            Day       my_default_day ( e, default_day, default_period );
            ptr<Day>  day            = Z_NEW( Day( e, &my_default_day, default_period ) );
            list<int> day_numbers    = get_day_numbers( e.getAttribute( "day" ) );

            Z_FOR_EACH( list<int>, day_numbers, d )
            {
                if( !_days[ *d ] )  _days[ *d ] = Z_NEW( Day( *day ) );
                              else  _days[ *d ]->merge( *day );
            }

            _is_day_set_filled = true;
        }
    }
}

//---------------------------------------------------------------------------------Day::set_dom_day

int Day_set::get_day_number( const string& day_string )
{
    int result;

    if( _minimum == 0  &&  _maximum == 6 )      // Es ist ein Wochentag
    {
        result = get_weekday_number( day_string );
    }
    else
    {
        result = as_int( day_string );

        if( result < _minimum  ||  result > _maximum )  z::throw_xc( "SCHEDULER-221", day_string, as_string( _minimum ), as_string( _maximum ) );
        //if( (uint)result >= NO_OF(_days) )  z::throw_xc( "SCHEDULER-INVALID-DAY", result );
    }

    return result;
}

//-------------------------------------------------------------------------Day_set::get_day_numbers

list<int> Day_set::get_day_numbers( const string& days_string )
{
    list<int>      result;
    vector<string> names  = vector_split( " +", days_string );

    Z_FOR_EACH( vector<string>, names, it )  result.push_back( get_day_number( *it ) );

    return result;
}

//----------------------------------------------------------------------Day_set::get_weekday_number

int Day_set::get_weekday_number( const string& weekday_name )
{
    int    result     = -1;
    string day_string = lcase( weekday_name );
    
    for( const char** p = weekday_names; *p && result == -1; p++ )
        if( day_string == *p )  result = ( p - weekday_names ) % 7;

    if( result == -1 )
    {
        result = as_int( weekday_name );
        if( result == 7 )  result = 0;      // Sonntag darf auch 7 sein
    }

    if( result < 0  ||  result > 6 )  z::throw_xc( "SCHEDULER-445", weekday_name );

    return result;
}

//---------------------------------------------------------------------Day_set::get_weekday_numbers

list<int> Day_set::get_weekday_numbers( const string& weekday_names )
{
    list<int>      result;
    vector<string> names  = vector_split( " +", weekday_names );

    Z_FOR_EACH( vector<string>, names, it )  result.push_back( get_weekday_number( *it ) );

    return result;
}

//-------------------------------------------------------------------------------Month::set_default

void Month::set_default()
{
    Day default_day;

    default_day.set_default();
    _weekday_set.fill_with_default( default_day );
}

//-----------------------------------------------------------------------------------Month::set_dom

void Month::set_dom( const xml::Element_ptr& element )
{
    Period  default_period;
    Day     default_day;
    bool    period_seen    = false;

    default_period.set_dom( element );
    default_day = default_period;

    bool a_day_is_set = false;

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "period" ) )
        {
            if( !period_seen )  period_seen = true, default_day = Day();
            default_day.add( Period( e, &default_period ) );
        }
        else
        if( e.nodeName_is( "weekdays" ) )
        {
            a_day_is_set = true;
            _weekday_set.set_dom( e, &default_day, &default_period );
        }
        else
        if( e.nodeName_is( "monthdays" ) )
        {
            a_day_is_set = true;
            _monthday_set.set_dom( e, &default_day, &default_period );
        }
        else
        if( e.nodeName_is( "ultimos" ) )
        {
            a_day_is_set = true;
            _ultimo_set.set_dom( e, &default_day, &default_period );
        }
    }

    if( !a_day_is_set )  _weekday_set.fill_with_default( default_day );
}

//-------------------------------------------------------------------------------Month::next_period

Period Month::next_period( const Time& tim_, With_single_start single_start )
{
    Time   tim           = tim_;
    Period result;
    int    current_month = tim.month_nr();

    while( tim < Time::never )
    {
        int m = tim.month_nr();
        if( m != current_month )  break;

        if( _weekday_set .is_filled() )  result = min( result, _weekday_set .next_period( tim, single_start ) );
        if( _monthday_set.is_filled() )  result = min( result, _monthday_set.next_period( tim, single_start ) );
        if( _ultimo_set  .is_filled() )  result = min( result, _ultimo_set  .next_period( tim, single_start ) );
        
        if( result.begin() != Time::never )  break;

        tim = tim.midnight() + 24*60*60;                        // Keine Periode? Dann nächsten Tag probieren
    }

    if( !result.begin().is_never()  &&  result.begin().month_nr() != current_month )  result = Period();

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace schedule
} //namespace scheduler
} //namespace sos
#endif
