if [ -z "$SCHEDULER_AGENT_HOME" ]; then :
    SCHEDULER_AGENT_HOME=$(cd "$(dirname "$0")/.."; pwd)
fi

if [ "$OSTYPE" = "cygwin" ]; then
    jarDir=$(cygpath -w "$SCHEDULER_AGENT_HOME/jar")
    javaHome=""
    [ -n "$JAVA_HOME" ] && javaHome=$(cygpath "$JAVA_HOME")
else
    jarDir="$SCHEDULER_AGENT_HOME/jar"
    javaHome="$JAVA_HOME"
fi

java=java
if [ -n "$javaHome" ]; then :
    java="$javaHome/bin/java"
fi
