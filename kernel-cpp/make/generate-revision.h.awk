# $Id$

BEGIN {
    FS = "[ ]+";
    print "// Generiert von generate-revision.h.awk";
    print "//";
    print "// svn commit -m \"\"";
    print "// svn update  (aktualisiert die Revisionsnummer für svn info)";
    print "// svn info | gawk generate-revision.h >revision.h";
    print "";
}

/^Revision:/ { 
    gsub( /[\r\n]/, "", $2 );
    print "#define VER_REVISION      " $2;
    print "#define VER_REVISION_STR \"" $2 "\"";
}

/^Last Changed Date:/ { 
    print "#define VER_DATE         \"" $4 "\"";
    print "#define VER_TIME         \"" $5 "\""; 
}

# SS-2010-02-03: Für die deutsche Variante des SVN-Tools.
/^Letztes .nderungsdatum:/ { 
    print "#define VER_DATE         \"" $3 "\"";
    print "#define VER_TIME         \"" $4 "\""; 
}


END {
    print "";
    print "";
    print "#ifdef _DEBUG";
    print "#   define VER_PRODUCTVERSION_DEBUG \" (Debug)\"";
    print "#else"
    print "#   define VER_PRODUCTVERSION_DEBUG \"\"";
    print "#endif";
    print "";
    print "#define VER_PRODUCTVERSION_TAIL  VER_REVISION_STR \"  (\" VER_DATE \" \" VER_TIME \") \" VER_PRODUCTVERSION_DEBUG "
}
