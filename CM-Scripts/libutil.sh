#!/bin/bash
#
# Jan 2005, M. Spiekermann. This is a small library
# of functions useful for several shell scripts.

if [ -z "$BASH" ]; then
  printf "%s\n" "Error: You need a bash shell to run this script!"
  exit 1
fi

if [ "$OSTYPE" == "msys" ]; then
   prefix=/c
   platform="win32"
else 
   prefix=$HOME
   platform="linux"
fi


# recognize aliases also in an non interactive shell
shopt -s expand_aliases

# getTimeStamp
function getTimeStamp() {

date_TimeStamp=$(date "+%y%m%d-%H%M%S")
date_ymd=${date_TimeStamp%-*}
date_HMS=${date_TimeStamp#*-}

}

# printSep $1
#
# $1 message
#
# print a separator with a number and a message
declare -i stepCtr=1
function printSep() {

  printf "\n%s\n" "Step ${stepCtr}: ${1}"
  printf "%s\n" "------------------------------------"
  let stepCtr++
}

# checkCmd $1
#
# $1 command
#
# execute a command. In case of an error display the
# returncode
declare -i rc=0
function checkCmd() {

  eval ${1}  # useful if $1 contains quotes or variables
  let rc=$?  # save returncode

  if [ $rc -ne 0 ]; then
    printf "\n Failure! Command {${1}} returned with value ${rc} \n"
  fi
}

# sendMail $1 $2 $3 [$4]
#
# $1 subject
# $2 recipients
# $3 body
# $4 attached file
#
# Sends a mail (with a given attachment) to the list of
# recipients.
sendMail_deliver="true"
function sendMail() {

  if [ "${4}" != "" ]; then
    attachment="-a ${4}"
  fi

  if [ sendMail_Deliver == "true" ]; then
  mail -s"$1" ${attachment} "$2" <<-EOFM
$3
EOFM

  else
    printf "%s\n" "Mail Command:"
    printf "%s\n" "mail -s $1 $attchament $2"

  fi

}

#showGPL
#
function showGPL() {

  printf "%s\n"   "Copyright (C) 2004, University in Hagen,"
  printf "%s\n"   "Department of Computer Science,"
  printf "%s\n\n" "Database Systems for New Applications."
  printf "%s\n"   "This is free software; see the source for copying conditions."
  printf "%s\n"   "There is NO warranty; not even for MERCHANTABILITY or FITNESS"
  printf "%s\n"   "FOR A PARTICULAR PURPOSE."
}

#uncompressFolders
#
# $1 list of directories
#
# For each direcory all *.gz files are assumed to be a tar archive and
# all *.zip files a zip archive

function uncompressFolders() {

for folder in $1; do
  zipFiles=$(find $folder -maxdepth 1 -name "*.zip")
  gzFiles=$(find $folder -maxdepth 1 -name "*.*gz")
  for file in $zipFiles; do
    printf "\n  processing $file ..."
    if { ! unzip -q -o $file; }; then
      exit 21 
    fi
  done
  for file in $gzFiles; do
    printf "\n  processing $file ..."
    if { ! tar -xzf $file; }; then
      exit 22 
    fi
  done
done

}

# runTTYBDB
#
# $1 SECONDO command
#
# Starts SecondoTTYBDB runs command $1

declare -i rc_ttybdb=0
function runTTYBDB() {

SecondoTTYBDB <<< "$1"

let rc_ttybdb=$?

}

# mapStr
#
# $1 file
# $2 name1
# $3 separator
#
# reads file $1 which contains a list of "name1 name2" entries
# and returns name2 if "$1"=0"name1". The parameter name1 should 
# be unique otherwise the first occurence will be used.
function mapStr() {

  sep=$3
  if [ "$sep" == "" ]; then
    sep=" "
  fi

  mapStr_line=$(grep $2 $1) 
  mapStr_name1=${mapStr_line%%${sep}*}
  if [ "$mapStr_name1" == "$2" ]; then
    #cut off name1 
    mapStr_name2=${mapStr_line#*${sep}}
    # remove trailing blanks
    mapStr_name2=${mapStr_name2%% *} 
  else
    mapStr_name2=""
  fi 
} 

# define some environment variables
TEMP="/tmp"
if [ ! -d $TEMP ]; then
  printf "%s\n" "creating directory ${TEMP}"
fi 

buildDir=${SECONDO_BUILD_DIR}
scriptDir=${buildDir}/CM-Scripts
binDir=${buildDir}/bin
optDir=${buildDir}/Optimizer


PATH="${PATH}:${binDir}:${optDir}:${scriptDir}"
LD_LIBRARY_PATH="/lib:${LD_LIBRARY_PATH}"

#initialize date_ variables
getTimeStamp

####################################################################################
#
# Test functions
#
####################################################################################

if [ "$1" == "test" ]; then  

for msg in "hallo" "dies" "ist" "ein" "test"
do
  printSep $msg
done 

checkCmd "echo 'hallo' > test.txt 2>&1"
checkCmd "dfhsjhdfg > test.txt 2>&1"

XmailBody="This is a generated message!  

  Users who comitted to CVS yesterday:
  $recipients

  You will find the output of make in the attached file.
  Please fix the problem as soon as possible."

sendMail "Test Mail!" "spieker root" "$XmailBody" "test.txt"

fi

if [ "$1" == "mapTest" ]; then

   cat $2
   mapStr "$2" "$3" "$4"
   printf "%s\n" "\"$3\" -> \"$mapStr_name2\""
fi

if [ "$1" == "tty" ]; then

runTTYBDB "list algebras;
q;"

fi
