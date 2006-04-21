#! /bin/sh
while read line; do
    if echo "$line" | grep '^thumbnail size=small' >/dev/null 2>/dev/null; then
        echo "thumbnail size=48"
    elif echo "$line" | grep '^thumbnail size=med' >/dev/null 2>/dev/null; then
        echo "thumbnail size=96"
    elif echo "$line" | grep '^thumbnail size=large' >/dev/null 2>/dev/null; then
        echo "thumbnail size=128"
    else
        echo "$line"
    fi
done
