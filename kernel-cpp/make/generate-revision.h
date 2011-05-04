# $Id$

LANG=en_US.iso88591 svn info | gawk -f make/generate-revision.h.awk >revision.h
