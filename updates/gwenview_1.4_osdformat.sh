#!/bin/sh

# Escape backslashes. Shell sucks.
sed s'/\\/BACKSLASH/g' | ( 

osdMode=
while read line ; do
	if echo $line | grep '^osd mode' >/dev/null 2>/dev/null ; then
		osdMode=$(echo $line | sed 's/^.*=//')
		continue
	fi
	if echo $line | grep '^free output format' >/dev/null 2>/dev/null ; then
		freeOutputFormat=$(echo $line | sed -e 's/^.*=//')
		continue
	fi
	echo $line
done

case $osdMode in
	0)
		osdFormat=''
		;;
	1)
		osdFormat='%p'
		;;
	2)
		osdFormat='%c'
		;;
	3)
		osdFormat='%p\\n%c'
		;;
	4)
		osdFormat="$freeOutputFormat"
		;;
	*)
		osdFormat=''
		;;
esac

echo osdFormat=$osdFormat

) | sed 's/BACKSLASH/\\/g'
# Escape backslashes. Shell sucks.
