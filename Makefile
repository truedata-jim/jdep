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
	rm -rf $(BIN_DIR)/jdep $(BIN_DIR)/touchp

EXCLUDES = -e org.springframework

test: jdep
	rm -rf testdata/deps/*
	rm -rf testdata/origdeps/*
	jdep $(EXCLUDES) -d testdata/deps -c badger_exp/test-classes -j badger_exp/server/test \
		badger_exp/test-classes/com/redsealsys/srm/server/analysis/AbstractTestByConfigFile.class
	origjdep $(EXCLUDES) -d testdata/origdeps -c badger_exp/test-classes -j badger_exp/server/test \
		badger_exp/test-classes/com/redsealsys/srm/server/analysis/AbstractTestByConfigFile.class
	diff -q testdata/deps/com/redsealsys/srm/server/analysis/AbstractTestByConfigFile.d \
	        testdata/origdeps/com/redsealsys/srm/server/analysis/AbstractTestByConfigFile.d

	jdep $(EXCLUDES) -d testdata/deps -c badger_exp/server/classes -j badger_exp/server/src \
		badger_exp/server/classes/com/redsealsys/srm/server/analysis/NetmapWorker.class
	origjdep $(EXCLUDES) -d testdata/origdeps -c badger_exp/server/classes -j badger_exp/server/src \
		badger_exp/server/classes/com/redsealsys/srm/server/analysis/NetmapWorker.class
	diff -q testdata/deps/com/redsealsys/srm/server/analysis/NetmapWorker.d \
	        testdata/origdeps/com/redsealsys/srm/server/analysis/NetmapWorker.d

	jdep $(EXCLUDES) -d testdata/deps -c badger_exp/server/classes -j badger_exp/server/src \
		badger_exp/server/classes/com/redsealsys/srm/server/analysis/compactTree/CompactTreeTrafficFlow.class
	origjdep $(EXCLUDES) -d testdata/origdeps -c badger_exp/server/classes -j badger_exp/server/src \
		badger_exp/server/classes/com/redsealsys/srm/server/analysis/compactTree/CompactTreeTrafficFlow.class
	diff -q testdata/deps/com/redsealsys/srm/server/analysis/compactTree/CompactTreeTrafficFlow.d \
	        testdata/origdeps/com/redsealsys/srm/server/analysis/compactTree/CompactTreeTrafficFlow.d








