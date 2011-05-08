'$Id$
'
'Prüft den Zugriff auf Blobs mit der Hostware

dim dbname
dim db

set hostware = createobject( "hostware.global" )

tmp_dir = "c:\tmp"
dim filename

'--------------------------------------------------------------------------------main

sub main

if wscript.arguments.count < 0 then 
    wscript.echo "Der Datenbankname fehlt"
    exit sub
end if

dbname = wscript.arguments(0) 
if instr( dbname, " " ) = 0 then dbname = "odbc -create " & dbname 
dbname = dbname & " "

set db = createobject( "hostware.file" )
db.open dbname

on error resume next
db.put_line "create table test_blob (a integer not null, b blob); commit"
db.put_line "create table test_clob (a integer not null, b clob); commit"
db.put_line "insert into test_blob (a) value (0); commit"
db.put_line "insert into test_clob (a) value (0); commit"
on error goto 0

filename  = tmp_dir & "\test_blob.tmp"
hostware.copy_file "select i loop i between 0 and 99999", "file -b " & filename

test "test_blob", "-binary "
test "test_clob", ""

end sub

'---------------------------------------------------------------------------------test

sub test( table_name, options )

    wscript.echo optiont & " " & table_name

    filename2 = tmp_dir & "\test_blob2.tmp"
    blob = " -table=" & table_name & " -blob=b where id=0"

    hostware.copy_file "file -b " & filename, options & dbname & " -commit " & blob
    hostware.copy_file options & dbname & blob, "file -b " & filename2
    if files_equal( filename, filename2 ) then
        wscript.echo options & " " & table_name & "  OK"
     else
        wscript.echo options & " " & table_name & "  FEHLER"
    end if

end sub

'--------------------------------------------------------------------------equal_files

function equal_files( fn1, fn2 )

    set f1 = createobject( "hostware.file" )
    set f2 = createobject( "hostware.file" )

    f1.open "-in -binary file -fixed-length=4096 " & fn1
    f2.open "-in -binary file -fixed-length=4096 " & fn2

    while not f1.eof()
        r1 = f1.get_line()
        r2 = f2.get_line()
        if r1 <> r2 then equal_files = false : exit function
        if len(r1) <> 4096 and not f1.eof() then equal_files = false : exit function
        if len(r2) <> 4096 and not f2.eof() then equal_files = false : exit function
    wend

    equal_files = f2.eof()

end function

'-----------------------------------------------------------------------------clean_up

sub clean_up

on error resume next

if db.opened then
    db.put_line "rollback"
    db.put_line "drop table test_blob; commit"
    db.put_line "drop table test_clob; commit"
    hostware.remove_file filename
end if

end sub

'-------------------------------------------------------------------------------------

on error resume next
main
if err then wscript.echo err.description 
clean_up 
