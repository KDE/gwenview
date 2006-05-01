#!/bin/sh
check() {
	cat > src.txt <<EOF
$1
EOF
	cat > tmp.txt <<EOF
$2
EOF
	sort tmp.txt > wanted.txt
	./gwenview_1.4_osdformat.sh < src.txt | sort > dst.txt
	if diff -uwB --brief wanted.txt dst.txt ; then
		echo ok
	else
		echo "
--------------
# From:
$1
# To:
$2
# Got:
"
		cat dst.txt
		echo "# Wanted:"
		cat wanted.txt
		echo "
--------------
"
	fi
}

check "osd mode=0" "osdFormat="
check "osd mode=1" "osdFormat=%p"
check "osd mode=2" "osdFormat=%c"
check "osd mode=3" "osdFormat=%p\\n%c"
check "osd mode=3
something else" "osdFormat=%p\\n%c
something else"
check "osd mode=4
free output format=zog\\nzog" "osdFormat=zog\\nzog"
