#!/bin/bash
# This script detects if there is one of wget/curl downloaders availabe and downloads
# three raw files from the digikam's store.
# If there is none of the two present in the system, the script suggests manual download.

DOWNLOADER=""

which wget &>/dev/null && DOWNLOADER="wget -O "
[ "$DOWNLOADER" == "" ] && which curl &>/dev/null && DOWNLOADER="curl -o "

if [ "$DOWNLOADER" != "" ] ; then
    echo "Fetching CANON-EOS350D-02.CR2 (1/3)"
    $DOWNLOADER CANON-EOS350D-02.CR2 http://digikam3rdparty.free.fr/TEST_IMAGES/RAW/HORIZONTAL/CANON-EOS350D-02.CR2
    echo "Fetching dsc_0093.nef (2/3)"
    $DOWNLOADER dsc_0093.nef http://digikam3rdparty.free.fr/TEST_IMAGES/RAW/HORIZONTAL/dsc_0093.nef
    echo "Fetching dsd_1838.nef (3/3)"
    $DOWNLOADER dsd_1838.nef http://digikam3rdparty.free.fr/TEST_IMAGES/RAW/VERTICAL/dsd_1838.nef
else
    echo "Unable to detect downloader. Please install one of wget/curl and try again or"
    echo "fetch manually the following files:"
    echo "    http://digikam3rdparty.free.fr/TEST_IMAGES/RAW/HORIZONTAL/CANON-EOS350D-02.CR2"
    echo "    http://digikam3rdparty.free.fr/TEST_IMAGES/RAW/HORIZONTAL/dsc_0093.nef"
    echo "    http://digikam3rdparty.free.fr/TEST_IMAGES/RAW/VERTICAL/dsd_1838.nef"
    echo "in the tests/data directory."
    exit 1
fi
