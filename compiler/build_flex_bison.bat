rem build the lex files
rem win_flex_bison\win_flex --wincompat -ocore/lex-asn1.c core/lex-asn1.l
win_flex_bison\win_flex -ocore/lex-asn1.c core/lex-asn1.l
rem build the yacc / bison files
win_flex_bison\win_bison -t -d -v -ocore\parse-asn1.c core\parse-asn1.y
