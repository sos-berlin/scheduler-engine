@rem $Id$
REM auszuführen aus prod
setlocal
ping subversion.sos.
set LANG=en_US
svn commit -m "%1"
svn update
svn info | gawk -f ../prod/make/generate-revision.h.awk >revision.h
endlocal
