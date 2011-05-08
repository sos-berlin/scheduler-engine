// make/release.js
// $Id: release.js 4581 2006-10-24 18:11:54Z jz $

/*
    Freigabe zur SOS
*/

//-------------------------------------------------------------------------------------------------

var prod_dir  = "./";

//-------------------------------------------------------------------------------------------------
// Folgende Variablen sollen von außen gesetzt werden, damit deren nicht in Sourceforge auftauchen
//var remote_host_sos = "user@host";

//-------------------------------------------------------------------------------------------------

var os_base              = "win32/";
var archive_base         = "Archive/";
var rsync_options        = "--delete  --exclude=CVS  --exclude _vti_cnf  --exclude .svn  --exclude .cvsignore  --delete-excluded";
var rsync_remote_options = "-e /usr/bin/ssh  -z --delete --exclude '*.map'  --exclude '*.pdb'  --exclude '*.7z'  --exclude .cvsignore"
//var cygwin_prod_dir      = prod_dir.replace( /^([a-z]):\//i, "/cygdrive/$1/" );

//-------------------------------------------------------------------------------------------------

var release_group_name = "";
var files                = new Array();    
var groups               = new Object();
var hostware             = new ActiveXObject( "hostware.Global" );
var shell                = new ActiveXObject( "WScript.Shell" );
var fso                  = new ActiveXObject( "Scripting.FileSystemObject" );

//-------------------------------------------------------------------------------------------------

if( spooler != undefined )
{
    release_group_name = spooler.variables( "group" );
}
else
{
    if( WScript.Arguments.length > 0 )  release_group_name = WScript.Arguments(0);
    run();
}

//----------------------------------------------------------------------------------------------run

function run()
{
    if( !prepare() )  return;

    build_file_list();
    release();
}

//----------------------------------------------------------------------------------build_file_list

function build_file_list()
{
    if( !release_group_name  ||  release_group_name == "scheduler" )
    {
        add_group( "scheduler", version_of_file( "bin/scheduler.exe" ) );       // ermittelt Versionsnummer (für Zielordner)
        add_file ( "scheduler", "bind/scheduler.dll"             );             // scheduler.dll zuerst, damit scheduler.map überschrieben wird!
        add_file ( "scheduler", "bind/scheduler.exe", "scheduler_dll.exe" );
        add_file ( "scheduler", "bin/scheduler.exe"              );
      //add_file ( "scheduler", "bin/sos.spooler.jar"            );             // 2010-03-25-SS: jetzt über separate JAVA-Verteilung
        add_file ( "scheduler", "../../jar/target/com.sos.scheduler.engine.jar", "sos.spooler.jar");             // 2010-09-28-SS: heisst im release zunächst weiterhin sos.spooler.jar
        add_file ( "scheduler", "scheduler/scheduler.xsd"        );
        add_file ( "scheduler", "scheduler/doc/"                 , "doc" );
      //add_file ( "scheduler", "scheduler/doc/javadoc"          , "doc" );  // Extra wegen .cvsignore
    }

    if( !release_group_name  ||  release_group_name == "hostware" )
    {
        add_group( "hostware" , version_of_file( "bin/hostole.dll"   ) );
        add_file ( "hostware" , "bin/hostole.dll"                );
        add_file ( "hostware" , "bin/sos.mail.jar"               );
        add_file ( "hostware" , "bin/hostcopy.exe"               );
      //add_file ( "hostware" , "bin/document_processor.exe"     );
      //add_group( "hostphp" , version_of_file( "bin/hostphp.dll"   ) );
        add_file ( "hostware" , "bin/hostphp.dll"                );
        add_file ( "hostware" , "hostphp/hostphp.inc"            );
        add_file ( "hostware" , "hostphp/hostware_file.inc"      );
        add_file ( "hostware" , "hostphp/hostware_dynobj.inc"    );
        add_file ( "hostware" , "hostole/RELEASE.TXT"            );
    }

    if( !release_group_name  ||  release_group_name == "hostjava" )
    {
        add_group( "hostjava" , version_of_file( "bin/hostjava.dll"  ) );
        add_file ( "hostjava" , "bin/hostjava.dll"               );
        add_file ( "hostjava" , "bin/sos.hostware.jar"           );
        add_file ( "hostjava" , "hostjava/RELEASE.TXT"           );
    }

    if( !release_group_name  ||  release_group_name == "documentfactory" )
    {
        add_group( "documentfactory", version_of_file( "bin/documentfactory.dll" )  );
        add_file ( "documentfactory", "bin/documentfactory.dll"      );
        add_file ( "documentfactory", "document_factory/RELEASE.TXT" );
    }

    if( !release_group_name  ||  release_group_name == "spidermonkey" )
    {
        add_group( "spidermonkey", version_of_file( "../spidermonkey/bin/spidermonkey.dll" )  );
        add_file ( "spidermonkey", "../spidermonkey/bin/spidermonkey.dll"    );
        add_file ( "spidermonkey", "../spidermonkey/bin/javascript.exe"    );
    }
}

//------------------------------------------------------------------------------------------prepare

function prepare()
{
    hostware.chdir( prod_dir );
    
return true;
    run_cmd( "cvs commit -f -m '' module_version.h" );
    run_cmd( "cvs commit -m ''" );
    
    var button_pressed = shell.Popup( "Jetzt bitte alles übersetzen!", -1, "release", 1 | 48 );
    if( button_pressed != 1 )  { echo( "ABGEBROCHEN." );  return false; }
    return true;
}

//------------------------------------------------------------------------------------------release

function release()
{
    remove_group_files();    
    release_local();
    generate_version_files();
}

//------------------------------------------------------------------------------------release_local

function release_local()
{
    for( var i in groups )
    {    
        var group = groups[ i ];
        run_cmd( "mkdir " + archive_base + group._name + "/" + group._name_with_version );
    }
    
    for( var i in files ) 
    {
        var file = files[i];
        if( i > 0  &&  file._group != files[i-1]._group )  echo( "" );
        
        var dest = archive_base + file._destination;
        
//        if( file._new_path == undefined )
//        {
//            run_cmd( "rsync -a " + rsync_options + " " + file._rel_path, dest + "/" );
//        }
//        else
//        {
            run_cmd( "rsync -a " + rsync_options + " "  + file._rel_path,  dest + "/" + file._new_path );
//        }
        
        if( file._rel_path.match( /^(.*bin\/)(.*\.(exe|dll))$/ ) )       // bin/*.exe oder bin/*.dll?
        {       
            var filename = RegExp.$2;
            run_cmd( "rsync -a " + rsync_options + " " + file._rel_path + ".map ", dest + "/" + file._new_path + ".map" );
            run_cmd( "rsync -a " + rsync_options + " " + file._rel_path + ".pdb ", dest + "/" + file._new_path + ".pdb" );
            //run_cmd( "rsync -a " + rsync_options + " " + file._rel_path + ".map " + file._rel_path + ".pdb", dest + "/" );
            //run_cmd( "bzip2 --force --best --quiet " + dest + "/" + filename + ".map " + dest + "/" + filename + ".pdb" );
        }
    }

    echo( "" );
}

//---------------------------------------------------------------------------generate_version_files

function generate_version_files()
{
    for( var i in groups )
    {    
        var group = groups[ i ];
        var filename = group._version.replace( / +$/, "" ).replace( /:/g, "" );
        
        run_cmd( "touch \"" + archive_base + group._name + "/" + group._name_with_version + "/" + filename + "\"" );
    }
}

//--------------------------------------------------------------------------------remove_group_files

function remove_group_files()
{
    for( var i in groups )
    {    
        var group = groups[ i ];
        
        run_cmd( "rm -rf " + archive_base + group._name + "/" + group._name_with_version );
    }
}

//-----------------------------------------------------------------------------------release_remote

function release_remote( remote_host, remote_dir, zip )
{
    var ext = zip? ".zip" : "";
    
    // prepare_archive_version kopiert die letzte Version auf die neue, damit rsync was zu optimieren hat
    if( !zip )
    {
        var remote_cmds = "cd ~/" + remote_dir + os_base + "; ";
        for( var i in groups )  remote_cmds += "~/bin/prepare_archive_version " + groups[i]._name + " " + filename_of_path( groups[i]._destination ) + " || true; ";
        run_cmd( "ssh " + remote_host + " "+ " \"" + remote_cmds + "\"" );
    }
    echo( "" );


    // rsync und chmod
    hostware.chdir( prod_dir + archive_base );
    {
        var dest = remote_dir + os_base + archive_base
        var dirs = "";
        for( var i in groups )  dirs += groups[i]._destination + ext + " ";
        run_cmd( "rsync -rt --relative " + rsync_remote_options + " " + dirs, remote_host + ":" + dest );
      //run_cmd( "rsync -a --relative " + rsync_remote_options + " " + dirs, remote_host + ":" + dest );
        
        var find_chmod = "cd " + dest + "  &&  " +
                         "find " + dirs + "  -type f  -exec chmod a+r,a-x {} \\\\;  &&  " +
                         "find " + dirs + "  -type d  -exec chmod a+r,a+x {} \\\\;";
                         
        run_cmd( "ssh " + remote_host + " \"" + find_chmod + "\"" );
    }
    hostware.chdir( ".." );
    echo( "" );


    // Links aktualisieren
    {
        var remote_cmds = "cd ~/" + remote_dir + os_base;
        for( var i in groups )  remote_cmds += "  &&  ln -nsf " + archive_base + groups[i]._destination + " " + groups[i]._name;
        run_cmd( "ssh " + remote_host + " "+ " \"" + remote_cmds + "\"" );
    }
    echo( "" );
}

//----------------------------------------------------------------------------------------add_group

function add_group( group_name, version )
{
    groups[ group_name ] = new Group( group_name, version );
}

//--------------------------------------------------------------------------------------------Group

function Group( group_name, version )
{
    this._name        = group_name;
    this._version     = version;
    this._destination = make_destination( group_name, version );
    this._name_with_version = make_filename_with_version( group_name + "*", version );
}

//-----------------------------------------------------------------------------------------add_file

function add_file( group_name, rel_path, new_path )
{
    var file = new File( group_name, rel_path, new_path );
    
    if( file._rel_path.match( /(exe|dll)$/ ) )
    {
        var file_version  = version_of_file( rel_path ).replace( " (Debug)", "" );
        var group_version = file._group._version.replace( " (Debug)", "" );
        
        if( group_version != file_version )  
            throw Error( "Version von " + rel_path + " (" + file_version + ") " + 
                         "stimmt nicht mit Version der Gruppe " + file._group._name + " (" + file._group._version + ") überein" );
    }
    
    files.push( file );
}

//---------------------------------------------------------------------------------------------File

function File( group_name, rel_path, new_path )
{
    var matches = rel_path.match( /^(.+\/)?([^\/]+)$/ );
    var filename = RegExp.$2;

    this._group       = groups[ group_name ];   if( !this._group )  throw Error( "Gruppe " + group_name + " unbekannt" );
    this._rel_path    = rel_path;
    this._new_path    = new_path? new_path : filename;
    this._destination = this._group._destination;
}

//---------------------------------------------------------------------------------make_destination

function make_destination( group_name, version )
{
    var destination = make_filename_with_version( group_name + "/" + group_name + "*", version );

    hostware.Remove_directory( archive_base + group_name, true );    // Alte Versionen löschen
    hostware.Make_path( archive_base + destination );
    
    return destination;
}

//----------------------------------------------------------------------------------version_of_file

function version_of_file( path )
{
    return hostware.file_version_info( prod_dir + path ).productVersion + "";
}

//-----------------------------------------------------------------------make_filename_with_version

function make_filename_with_version( path, product_version )
{
    var matches;
    
    matches = product_version.match( /((\d|\.)*).*(\d\d\d\d-\d\d-\d\d)/ );
    if( !matches || matches.length != 4 )  throw Error( "Version nicht erkennbar in " + product_version );
    
    var version  = RegExp.$1;
    var date     = RegExp.$3;

    matches = path.match( /^([^*]*)\*([^*]*)$/ );
    if( !matches || matches.length != 1+2 )  throw Error( "Dateiname nicht zerlegbar" );

    var prefix = RegExp.$1;
    var suffix = RegExp.$2
    
    return prefix + "-" + version + "-" + date + suffix;
}

//---------------------------------------------------------------------------generate_version_files

function generate_version_files()
{
    for( var i in groups )
    {    
        var group = groups[ i ];
        var filename = group._version.replace( / +$/, "" ).replace( /:/g, "" );
        
        run_cmd( "touch \"" + archive_base + group._name + "/" + group._name_with_version + "/" + filename + "\"" );
    }
}

//------------------------------------------------------------------------------------------run_cmd

function run_cmd( cmd_begin, cmd_rest )
{
    var blanks = "                                                                                      ";
    var cmd = cmd_rest == undefined? cmd_begin 
                                   : cmd_begin + blanks.substring( cmd_begin.length ) + " " + cmd_rest;
    echo( cmd );

    var subprocess = spooler_task.create_subprocess();
    subprocess.start( cmd );
    subprocess.wait_for_termination();
    if( subprocess.exit_code != 0 )  throw new Error( "Hat exit_code=" + subprocess.exit_code + " geliefert: " + cmd_begin );
    if( subprocess.termination_signal != 0 )  throw new Error( "Mit Signal " + subprocess.termination_signal + " abgebrochen: " + cmd_begin );

//    var process = shell.Exec( cmd );

//    wait_for_process( process );
//    
//    if( process.ExitCode != 0 )  throw Error( "Fehler bei " + cmd );
}

//---------------------------------------------------------------------------------wait_for_process

//function wait_for_process( process )
//{
//    while( process.Status == 0 ) 
//    {
//        if( !process.StdOut.AtEndOfStream )  WScript.stdout.write( process.stdout.ReadAll() );
//        else
//        if( !process.StdErr.AtEndOfStream )  WScript.stderr.write( process.stderr.ReadAll() );
//        else
//            WScript.Sleep( 1 );
//    }
//    
//    WScript.stdout.write( process.StdOut.ReadAll() );
//    WScript.stderr.write( process.StdOut.ReadAll() );
//}

//--------------------------------------------------------------------------------directory_of_path

function directory_of_path( path )
{
    var matches = path.match( /^(.*)\/[^\/]*$/ );
    if( matches.length != 1+1)  throw Error( "Dateiname nicht zerlegbar" );
    
    return RegExp.$1;
}

//---------------------------------------------------------------------------------filename_of_path

function filename_of_path( path )
{
    var matches = path.match( /\/([^\/]*)$/ );
    if( matches.length != 1+1)  throw Error( "Dateiname nicht zerlegbar" );
    
    return RegExp.$1;
}

//---------------------------------------------------------------------------------------------echo

function echo( text )
{
    if( spooler_log != undefined )  spooler_log.info( text );
                              else  WScript.echo( text );
}

//-------------------------------------------------------------------------------------------------
