BINDINGSDIR = $(DATAROOTDIR)/sipc/bindings/java
JARDIR = $(DATAROOTDIR)/java

JAVA_HOME ?= /usr/java/default
JAVAC ?= $(JAVA_HOME)/bin/javac
JAR ?= $(JAVA_HOME)/bin/jar
JAVADOC ?= $(JAVA_HOME)/bin/javadoc

TOP_SRCDIR ?= ../..
TOP_BUILDDIR ?= ../..
INCLUDEDIRS = -I$(TOP_SRCDIR)/include
LIBDIRS = -L$(TOP_BUILDDIR)/src

SRC = com/tresys/sipc/ConnectionException.java \
	com/tresys/sipc/Sipc.java \
	com/tresys/sipc/SipcMqueue.java \
	com/tresys/sipc/SipcShm.java
SWIG_SRC = libsipc_t.java \
	sipcwrapperConstants.java \
	sipcwrapper.java \
	sipcwrapperJNI.java \
	SWIGTYPE_p_sipc_t.java
CLASSES = $(patsubst %.java,%.class,$(SRC))
SWIG_CLASSES = $(patsubst %.java,com/tresys/sipc/%.class,$(SWIG_SRC))

SOVERSION = 1
RELEASE = 0
JARTARGET = libsipc-$(SOVERSION).$(RELEASE).jar

# Change the following to be appropriate for your system:
#
# JNI_CFLAGS:	Location of JNI include files.
#		Only set this value if JNI's include files
#		are installed outside of the compiler's search path.

#JNI_CFLAGS=-I/usr/lib/jvm/java/include -I/usr/lib/jvm/java/include/linux
JNI_CFLAGS=-I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux -fno-strict-aliasing

TARGET = libsipcwrapper.so
SONAME = $(TARGET).$(SOVERSION)
LIBSO = $(TARGET).$(SOVERSION).$(RELEASE)
SWIG_WRAP = sipc_wrap.c
OBJS = sipc_wrap.o

all: $(LIBSO) $(JARTARGET)

install: $(JARTARGET) $(LIBSO)
	test -d $(DESTDIR)/$(BINDINGSDIR) || install -m 755 -d $(DESTDIR)/$(BINDINGSDIR)
	test -d $(DESTDIR)/$(BINDINGSDIR)/tests || install -m 755 -d $(DESTDIR)/$(BINDINGSDIR)/tests
	install -m 644 README $(DESTDIR)/$(BINDINGSDIR)
	install -m 644 tests/* $(DESTDIR)/$(BINDINGSDIR)/tests
	install -m 644 $(JARTARGET) $(DESTDIR)/$(BINDINGSDIR)
	test -d $(DESTDIR)/$(JARDIR) || install -m 755 -d $(DESTDIR)/$(JARDIR)
	cd $(DESTDIR)/$(JARDIR) && ln -sf $(BINDINGSDIR)/$(JARTARGET) libsipc.jar
	cd $(DESTDIR)/$(JARDIR) && ln -sf $(BINDINGSDIR)/$(JARTARGET) $(JARTARGET)
	test -d $(DESTDIR)/$(LIBDIR) || install -m 755 -d $(DESTDIR)/$(LIBDIR)
	install -m 644 $(LIBSO) $(DESTDIR)/$(LIBDIR)
	cd $(DESTDIR)/$(LIBDIR) && ln -sf $(LIBSO) $(TARGET)
	cd $(DESTDIR)/$(LIBDIR) && ln -sf $(LIBSO) $(TARGET).$(SOVERSION)

$(JARTARGET): $(SWIG_CLASSES) $(CLASSES)
	$(JAR) cf $@ $^

%.class: %.java
	$(JAVAC) $(AM_JFLAGS) $(JFLAGS) $<

$(firstword $(SWIG_CLASSES)): $(SWIG_SRC)
	$(JAVAC) $(AM_JFLAGS) $(JFLAGS) -d . $^

$(wordlist 2,$(words $(SWIG_CLASSES)), $(SWIG_CLASSES)): $(firstword $(SWIG_CLASSES))

$(CLASSES): $(SWIG_CLASSES)

$(LIBSO): $(OBJS)
	$(CC) $(LDFLAGS) $(LIBDIRS) -Wl,-soname,$(SONAME) -lsipc -shared -o $@ $<
	ln -sf $@ $(TARGET)

$(SWIG_SRC) $(SWIG_WRAP): sipc.i
	swig -java -package com.tresys.sipc -o $(SWIG_WRAP) $(INCLUDEDIRS) $<

%.o: %.c
	$(CC) $(AM_CFLAGS) $(CFLAGS) $(INCLUDEDIRS) $(JNI_CFLAGS) -fPIC -c -o $@ $<

javadoc: $(SWIG_CLASSES)
	$(JAVADOC) -d javadoc $(SRC) $(SWIG_SRC)

clean:
	-rm -f $(SWIG_WRAP) $(SWIG_SRC) $(CLASSES) $(SWIG_CLASSES) $(JARTARGET) $(TARGET) $(LIBSO) $(OBJS)
	$(MAKE) -C tests clean

.PHONY: all install clean javadoc
