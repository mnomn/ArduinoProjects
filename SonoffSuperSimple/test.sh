#/bin/bash

#Test script tested on linux. Should wark on mac too.

if [ "$USERPASS" == "" ]; then
USERPASS='-u ADMIN:PASS'
fi

echo "Test SSS. Arg 1 shall be ip nr"

echo "USERPASS: $USERPASS"

[ "$1" ] ||{
    echo "No IP in arg 1"
    exit 1
}

if [ "$2" != "" ]
then
    echo "Set $2"
    curl $USERPASS -X POST -w ", code=%{http_code}"  $1'/switch?set='$2
    echo
    exit 0
fi

echo "Test $1"


# Get HELP
echo
curl $1
echo
echo "RESULT ROOT: $?"

sleep 1
echo
echo "#####"
echo "GET"
curl $1/switch
echo

sleep 1
echo "#####"
echo "Bad arg (expect 40x)"
curl $USERPASS -X POST -w ", code=%{http_code}" $1'/switch?set=r'
echo

sleep 1
echo "#####"
echo "Good arg"
curl $USERPASS -X POST -w ", code=%{http_code}"  $1'/switch?set=t'
echo

sleep 1
echo "#####"
echo "Get again, expect changed"
curl $1/switch
echo
