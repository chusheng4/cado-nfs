#!/bin/bash
# iterates 100 times
i="0"

while [ $i -lt 100 ]
do
 service mpss restart
 cd /home/cado-nfs
 #install on mic0-7
 sh ./build.mic_rpms.sh
 #ready
 sh ./build.trial-run.sh
 #done
 i=$[$i+1]
 sleep 5
 #ready
done
