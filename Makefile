.PHONY: all clean bin svg

CFLAGS=-ggdb -Og -DHASH_DEBUG=1

bin: lexer parser scratch

svg: parser.svg testdot.svg testdotfiltered.svg

all: bin parser.svg

clean:
	rm -f *.o lexer.c lexer.h parser.c parser.h lexer parser

scratch: scratch.o external/lib/libtalloc.so.2.4.2

lexer: lexer.o lexer_driver.o token.o external/lib/libtalloc.so.2.4.2

parser: parser.o lexer.o parser_driver.o token.o symtab.o walker.o dot_filter.o parser_helper.o external/lib/libtalloc.so.2.4.2 codegen.o nothing_filter.o

lexer.o: lexer.c parser.h

parser.o: parser.c lexer.h
	$(CC) $(CFLAGS) -c -DYYDEBUG=1 -o parser.o parser.c

parser_driver.o: parser.h

lexer.c lexer.h: lexer.l
	flex \
		--bison-bridge \
		--noyywrap \
		--debug \
		-Caef \
		--header-file=lexer.h \
		--outfile=lexer.c \
		lexer.l

parser.c parser.h parser.dot: parser.y
	bison -gt --debug --defines --report=all --report-file=parser.report.txt --graph=parser.dot --output=parser.c parser.y

testdot.dot testdotfiltered.dot: parser test.x
	TALLOC_FREE_FILL=240 ./parser < test.x

%.svg: %.dot
	dot -Tsvg $^ > $@
