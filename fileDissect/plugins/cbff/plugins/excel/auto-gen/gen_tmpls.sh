#!/bin/sh
#

OUTDIR=to_c

# generate the record define list
awk -F: '{printf "#define WBOOK_RT_"$2" \t"$1"\n"}' xls97-recs.txt > $OUTDIR/record-defs.tmpl
awk -F: '{printf "#define WBOOK_RT_CH"$2" \t"$1"\n"}' xls97-chart-recs.txt > $OUTDIR/chart-defs.tmpl

# generate long string switch cases
awk -F: '{printf "\t\tcase WBOOK_RT_"$2":\n\t\t\treturn wxT(\""$3"\");\n"}' xls97-recs.txt > $OUTDIR/record_type_long.tmpl
awk -F: '{printf "\t\tcase WBOOK_RT_CH"$2":\n\t\t\treturn wxT(\""$3"\");\n"}' xls97-chart-recs.txt > $OUTDIR/record_type_ch_long.tmpl

# generate short string switch cases
awk -F: '{printf "\t\tcase WBOOK_RT_"$2":\n\t\t\treturn wxT(\""$2"\");\n"}' xls97-recs.txt > $OUTDIR/record_type_short.tmpl
awk -F: '{printf "\t\tcase WBOOK_RT_CH"$2":\n\t\t\treturn wxT(\"CH"$2"\");\n"}' xls97-chart-recs.txt > $OUTDIR/record_type_ch_short.tmpl

