JAVA_HOME ?= /usr/java/default
JAVAC ?= $(JAVA_HOME)/bin/javac

AM_JFLAGS = -cp ../libsipc-1.0.jar

SRC = MQ_Creator.java \
	MQ_Destroyer.java \
	MQ_Reader.java \
	MQ_Sender.java \
	SHM_Creator.java \
	SHM_Destroyer.java \
	SHM_Reader.java \
	SHM_Sender.java
CLASSES = $(patsubst %.java,%.class,$(SRC))

SIPC_KEYS=sipc_mq_key sipc_shm_key

all:

tests: $(CLASSES) $(SIPC_KEYS)

check: $(SIPC_KEYS) tests
	sh run_tests.sh

$(SIPC_KEYS):
	touch $@

%.class: %.java
	$(JAVAC) $(AM_JFLAGS) $(JFLAGS) $<

clean:
	rm -f $(CLASSES) $(SIPC_KEYS) out.dat out.trimmed

.PHONY: all tests check clean
	rm -f *.class core $(SIPC_KEYS)
