#!/bin/bash
# iterates 10 times
i="0"

while [ $i -lt 10 ]
do
 cd /home/cado-nfs
 sh ./build.bot.log.sh
 i=$[$i+1]
 sleep 5
 #ready
done

service mpss restart
scl enable python33 bash
cd /home/cado-nfs
#install on mic0-7
sh ./build.mic_rpms.sh
#ready