# Java tools Makefile

# "make all"       - Make the various tools
# "make jdep"      - Make the Java class file dependency analyzer tool
# "make clean"     - Remove object and executable files

# C++ compiler
CPP = g++ -g
CPPFLAGS = -Wall -Werror -g -O0 -ferror-limit=6

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
	$(CPP) -c $(CPPFLAGS) -o $@ $^

OBJS = $(O_DIR)/jdep.o \
	$(O_DIR)/BytesDecoder.o  \
	$(O_DIR)/ClassFile.o \
	$(O_DIR)/ClassFileAnalyzer.o \
	$(O_DIR)/FileReader.o

$(BIN_DIR)/jdep: $(OBJS)
	$(CPP) -o $@ $^

$(BIN_DIR)/touchp: touchp.sh
	cp touchp.sh $@
	chmod +x $@

.PHONY: all jdep touchp clean

clean:
	rm -rf $(OBJS) $(BIN_DIR)/jdep $(BIN_DIR)/touchp

test: jdep
	./test.sh badger_exp/test-classes badger_exp/server/test com/redsealsys/srm/server/analysis AbstractTestByConfigFile
	./test.sh badger_exp/server/classes badger_exp/server/src com/redsealsys/srm/server/analysis NetmapWorker
	./test.sh badger_exp/server/classes badger_exp/server/src com/redsealsys/srm/server/analysis/compactTree CompactTreeTrafficFlow



