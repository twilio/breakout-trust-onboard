#!/bin/bash
 
#run this script only when a message was received.
if [ "$1" != "RECEIVED" ]; then exit; fi;
   
#Extract data from the SMS file
SENDER=`formail -zx From: < $2`
TEXT=`formail -I "" < $2 | sed -e"1d"`

if [ "${SENDER}" == "2936" ]; then
  PARAMS=($TEXT)
  echo "PARAMS: ${PARAMS[1]}"
  if [ "${PARAMS[0]}" == "Register" ]; then
    echo "${PARAMS[1]}" > ~pi/azure_id_scope.txt
  fi
fi
