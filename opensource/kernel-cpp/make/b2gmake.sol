if [ "$1" != "" ]; then $0 <$1 >$1.gnumake; 
else

if [ -e $1~gnumake ]; then rm $1~gnumake; fi

/usr/sos/e/b2g.pl  

fi
