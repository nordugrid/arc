java -Xdebug -Xnoagent -Djava.compiler=none -Xrunjdwp:transport=dt_socket,server=y,suspend=y -classpath ../arc.jar:.  -Djava.library.path=../.libs:/usr/lib:../../src/hed/libs/common/.libs/:../../src/hed/libs/message/.libs test
#java -classpath ../arc.jar:.  -Djava.library.path=../.libs:/usr/lib:../../src/hed/libs/common/.libs/:../../src/hed/libs/message/.libs test
