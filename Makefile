# Java tools Makefile

# "make all"       - Make the various tools
# "make jdep"      - Make the Java class file dependency analyzer tool
# "make clean"     - Remove object and executable files

# C++ compiler
CPP = g++ -g

# The directory where built executables go
BIN_DIR = ./bin

O_DIR = ./o

DIRS = $(BIN_DIR) $(O_DIR)

all: jdep touchp test

jdep: $(DIRS) $(BIN_DIR)/jdep

touchp: $(BIN_DIR) $(BIN_DIR)/touchp

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(O_DIR):
	mkdir -p $(O_DIR)

$(O_DIR)/%.o : %.cpp
	$(CPP) -c -ferror-limit=6 -o $@ $^

$(BIN_DIR)/jdep: $(O_DIR)/jdep.o $(O_DIR)/BytesDecoder.o  $(O_DIR)/FileReader.o
	$(CPP) -o $@ $^

$(BIN_DIR)/touchp: touchp.sh
	cp touchp.sh $@
	chmod +x $@

.PHONY: all jdep touchp clean

clean:
	rm -rf $(BIN_DIR)/jdep $(BIN_DIR)/touchp

DEVROOT = badger_exp

EXCLUDES = -e org.springframework

test: jdep
	rm -rf testdata/deps/*
	rm -rf testdata/origdeps/*
	jdep $(EXCLUDES) -d testdata/deps -c $(DEVROOT)/test-classes -j $(DEVROOT)/server/test \
		$(DEVROOT)/test-classes/com/redsealsys/srm/server/analysis/AbstractTestByConfigFile.class
	origjdep $(EXCLUDES) -d testdata/origdeps -c $(DEVROOT)/test-classes -j $(DEVROOT)/server/test \
		$(DEVROOT)/test-classes/com/redsealsys/srm/server/analysis/AbstractTestByConfigFile.class
	diff -q testdata/deps/com/redsealsys/srm/server/analysis/AbstractTestByConfigFile.d \
	        testdata/origdeps/com/redsealsys/srm/server/analysis/AbstractTestByConfigFile.d
