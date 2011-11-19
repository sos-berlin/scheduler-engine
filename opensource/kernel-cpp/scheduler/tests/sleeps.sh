#! /bin/bash -v
echo Hallo
echo Zweite Zeile
/bin/sleep 11&
sh -c "/bin/sleep 12&  wait"&
/bin/sleep 13&
sleep 14&
sh -c "sh -c '/bin/sleep 15&  sh -c \"/bin/sleep 16\"  wait'&  wait" &
sh -c "sh -c '/bin/sleep 17'&  /bin/sleep 18&  wait" &
ps faxu
sh -c "sh -c '/bin/sleep 19'"

