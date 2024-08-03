# CS111 Assignment 3 Solution Makefile Hooks

PROGS = stsh
EXTRA_PROGS = spin split sigsegv conduit fpe


CXX = clang++-10

LIB_SRC = stsh-parser/scanner.cc stsh-parser/parser.cc stsh-parser/stsh-parse.cc stsh-parser/stsh-readline.cc

WARNINGS = -Wall -Werror -pedantic -Wno-unused-function -Wno-vla-extension -Wno-sign-compare
DEPS = -MMD -MF $(@:.o=.d)
DEFINES =
INCLUDES =

CXXFLAGS = -g -fPIE $(WARNINGS) -O0 -std=c++20 $(DEFINES) $(INCLUDES)
LDFLAGS = -lreadline -ll -lutil

LIB_OBJ = $(patsubst %.cc,%.o,$(patsubst %.S,%.o,$(LIB_SRC)))
LIB_DEP = $(patsubst %.o,%.d,$(LIB_OBJ))
LIB = stsh.a

PROGS_SRC = $(patsubst %,%.cc,$(PROGS))
PROGS_OBJ = $(patsubst %.cc,%.o,$(patsubst %.S,%.o,$(PROGS_SRC)))
PROGS_DEP = $(patsubst %.o,%.d,$(PROGS_OBJ))

EXTRA_PROGS_SRC = $(patsubst %,%.cc,$(EXTRA_PROGS))
EXTRA_PROGS_OBJ = $(patsubst %.cc,%.o,$(patsubst %.S,%.o,$(EXTRA_PROGS_SRC)))
EXTRA_PROGS_DEP = $(patsubst %.o,%.d,$(EXTRA_PROGS_OBJ))

# This auto-commits changes on a successful make and if the tool_run environment variable is not set (it is set
# by tools like sanitycheck, which run make on the student's behalf, and which already commmit).
# The very long piped git command is a hack to get the "tools git username" used
# when we make the project, and use that same git username when committing here.
all:: $(PROGS) $(EXTRA_PROGS)
	@retval=$$?;\
	if [ -z "$$tool_run" ]; then\
		if [ $$retval -eq 0 ]; then\
			git diff --quiet --exit-code;\
			retval=$$?;\
			if [ $$retval -eq 1 ]; then\
				git log tools/create | grep 'Author' -m 1 | cut -d : -f 2 | cut -c 2- | xargs -I{} git commit -a -m "successful make." --author={} --quiet;\
				git push --quiet;\
				fi;\
		fi;\
	fi

stsh-parser/parser.cc stsh-parser/scanner.cc:
	make -C stsh-parser

stsh-parser/parser.o: stsh-parser/parser.cc
stsh-parser/scanner.o: stsh-parser/scanner.cc

$(PROGS) $(EXTRA_PROGS): %:%.o $(LIB)
	$(CXX) $^ $(LDFLAGS) -o $@

clean::
	make -C stsh-parser clean
	rm -f $(PROGS) $(PROGS_OBJ) $(PROGS_DEP)
	rm -f $(EXTRA_PROGS) $(EXTRA_PROGS_OBJ) $(EXTRA_PROGS_DEP)
	rm -f $(LIB) $(LIB_DEP) $(LIB_OBJ)

$(LIB): $(LIB_OBJ)
	rm -f $@
	ar r $@ $^
	ranlib $@

spartan: clean
	make -C stsh-parser spartan
	\rm -fr *~

.PHONY: all clean spartan

-include $(LIB_DEP) $(PROGS_DEP) $(EXTRA_PROGS_DEP)

