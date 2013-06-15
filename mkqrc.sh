#!/bin/bash

QRCFILE=ldforge.qrc
FILES=$(echo ./icons/*.* data/*.* LICENSE LICENSE.icons)

printf "" > $QRCFILE

printf "<!DOCTYPE RCC>\n<RCC version=\"1.0\">\n<qresource>\n" >> $QRCFILE

# Make sure that whatever goes to QRC is added to the repo.
# I keep forgetting to do this myself.
for line in $(hg status $FILES |grep "?"); do
	if [ "$line" != "?" ]; then
		echo "hg add $line"
		hg add $line;
		
		echo "pngout $line"
		pngout $line
	fi
done

for f in $FILES; do
	printf "\t<file>$f</file>\n" >> $QRCFILE
done

printf "</qresource>\n</RCC>\n" >> $QRCFILE
