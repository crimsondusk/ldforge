#!/bin/bash

QRCFILE=ldforge.qrc
printf "" > $QRCFILE

printf "<!DOCTYPE RCC>\n<RCC version=\"1.0\">\n<qresource>\n" >> $QRCFILE

for img in ./icons/*.* ./docs/*.* LICENSE; do
	printf "\t<file>$img</file>\n" >> $QRCFILE
done

printf "</qresource>\n</RCC>\n" >> $QRCFILE