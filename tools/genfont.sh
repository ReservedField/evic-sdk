#!/bin/sh

if [ $# -eq 0 ] ;  then
    echo "No arguments supplied;, usage: genfont.sh <location of font> <output point size>"
fi

LIST=(\! \" \# \$ \% \& \' \( \) '*' "+" \, \- \. '/' \0 \1 \2 \3 \4 \5 \6 \7 \8 \9 \: \; \< \= \> \? \@ \A \B \C \D \E \F \G \H 
\I \J \K \L \M \N \O \P \Q \R \S \T \U \V \W \X \Y \Z \[ '\\' \] \^ \_ \` \a \b \c \d \e \f \g \h \i \j \k \l \m \n \o \p \q \r 
\s \t \u \v \w \x \y \z \{ \| \} \~)
FONT=$1
SIZE=$2
NAME=$3

printf "#include <stdint.h>\n#include <Font_Data.h>\n\n"

printf "// Auto generated file \n\n"
printf "const uint8_t Font_${NAME}_Bitmaps[] = { \n\n"

for i in "${LIST[@]}"; do
    # at the end, we remove the last 4 bytes to clear out the extra curly brace
    convert -font $FONT +antialias $4 -pointsize $SIZE -rotate 270 -flip -endian LSB label:"$i" xbm:- | grep -o '0x\w\w' | awk '{ if (NR%8==0) printf $1 ", "; else printf $1 ", "; }'
    printf "\n\n"
done

printf "};"

printf "\n\nconst Font_CharInfo_t Font_${NAME}_Descriptors[] = {\n\n"
COUNT=0
MAXWIDTH=1
MAXHEIGHT=1
for i in "${LIST[@]}"; do
    WIDTH=`convert -font $FONT +antialias $4 -pointsize $SIZE -rotate 270 -flip -endian LSB label:"$i" xbm:- | grep "height" | cut -d ' ' -f3`
    HEIGHT=`convert -font $FONT +antialias $4 -pointsize $SIZE -rotate 270 -flip -endian LSB label:"$i" xbm:- | grep "width" | cut -d ' ' -f3`
    BYTESIZE=`convert -font $FONT +antialias $4 -pointsize $SIZE -rotate 270 -flip -endian LSB label:"$i" xbm:- | grep -o "," | wc -l`

    if [ "$MAXWIDTH" -lt "$WIDTH" ]; then
        MAXWIDTH=$WIDTH
    fi

    if [ "$MAXHEIGHT" -lt "$HEIGHT" ]; then
        MAXHEIGHT=$HEIGHT
    fi

    echo "{$WIDTH, $COUNT}, /* "$i" w: $WIDTH h: $HEIGHT */"
    COUNT=$(( $COUNT + $BYTESIZE ))
done

printf "};\n\n"


printf "const Font_Info_t Font_${NAME}_FontInfo = {\n"
printf "$MAXHEIGHT, /* Character height */\n"
printf "'!', /* Start character */\n"
printf "'~', /* End character */\n"
printf "$MAXWIDTH, /* Width, in pixels, of space character */\n"
printf "Font_${NAME}_Descriptors, \n"
printf "Font_${NAME}_Bitmaps, \n"
printf "};"
