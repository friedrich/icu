#!/bin/sh

echo Rebuilding Makefile.res

PKG=$1
shift

echo "# Rebuild by $0 from $*" `date` > Makefile.res
for FILE in $*; do
    CODING=`fgrep '*- Coding:' $FILE | sed -e 's/^.*Coding: \([^ ;]*\).*/\1/' | sed -e 's/euc-jp/ibm-954/'` 
    if [ "$CODING" = "" ];
        then
          echo 1>&2 "# Warning: $FILE didn't have a proper coding tag - NOT setting encoding! " 
        else
#         echo "# $FILE : $CODING"
         RES=`echo $FILE | sed -e 's/\.txt/.res/'`
         (
            echo "${PKG}_${RES} : $FILE Makefile.res"
            echo '	@echo Rebuilding $@ - '$CODING
            echo '	@$(INVOKE) $(GENRB) --package-name '$PKG' -d . -e '$CODING' $<'
            echo
         ) >> Makefile.res
    fi
done