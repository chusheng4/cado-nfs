#!/bin/bash
# iterates 100 times
# executes 1000 builds/factor trials
i="0"

while [ $i -lt 100 ]
do
 cd /home/cado-nfs
 sh ./build.bot.log.Loop.10.sh
 i=$[$i+1]
 sleep 5
 #ready
done
