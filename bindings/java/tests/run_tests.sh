if test -z ${JAVA_HOME}; then
  JAVA_HOME=/usr/java/default
fi
if test -z ${JAVA}; then
  JAVA=${JAVA_HOME}/bin/java
fi
JFLAGS='-cp ../libsipc-1.0.jar:. -Djava.library.path=..'

MQ_TEST_FILE=Makefile
SHM_TEST_FILE=../sipc_wrap.c

set -e

echo "Testing Message Queues:"

echo -n "  creating..."
${JAVA} ${JFLAGS} MQ_Creator && echo " pass"

echo -n "  sending..."
${JAVA} ${JFLAGS} MQ_Sender ${MQ_TEST_FILE}
echo " pass"

echo -n "  reading..."
${JAVA} ${JFLAGS} MQ_Reader
diff ${MQ_TEST_FILE} out.dat || (echo " diff failed" ; exit 1)
echo " pass"

echo -n "  destroying..."
${JAVA} ${JFLAGS} MQ_Destroyer && echo " pass"
exit 0
echo "Testing Shared Memory:"

echo -n "  creating..."
${JAVA} ${JFLAGS} SHM_Creator && echo " pass"

echo "  sending (as a background process)..."
${JAVA} ${JFLAGS} SHM_Sender ${SHM_TEST_FILE} &

sleep 1
echo -n "  reading..."
${JAVA} ${JFLAGS} SHM_Reader
# because shm does not set the proper number of bytes read, truncate the
# output here
bytes=$(wc -c ${SHM_TEST_FILE} | cut -f 1 -d ' ')
dd if=out.dat of=out.trimmed count=1 bs=${bytes} > /dev/null 2>&1
result=$(diff ${SHM_TEST_FILE} out.trimmed)
if test ${result}; then
  echo " diff failed"
  exit 1
fi
echo " pass"

echo -n "  destroying..."
${JAVA} ${JFLAGS} SHM_Destroyer && echo " pass"
