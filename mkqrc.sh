#!/bin/bash

QRCFILE=ldforge.qrc
FILES=$(echo ./icons/*.* data/*.* LICENSE LICENSE.icons)

printf "" > $QRCFILE

printf "<!DOCTYPE RCC>\n<RCC version=\"1.0\">\n<qresource>\n" >> $QRCFILE

for f in $FILES; do
	printf "\t<file>$f</file>\n" >> $QRCFILE
done

printf "</qresource>\n</RCC>\n" >> $QRCFILE
