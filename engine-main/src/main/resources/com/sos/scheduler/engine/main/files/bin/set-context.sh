if [ -z "$SCHEDULER_HOME" ]; then :
    SCHEDULER_HOME=$(cd "$(dirname "$0")/.."; pwd)
fi

if [ "$OSTYPE" = "cygwin" ]; then
    jarDir=$(cygpath -w "$SCHEDULER_HOME/jar")
    javaHome=""
    [ -n "$JAVA_HOME" ] && javaHome=$(cygpath "$JAVA_HOME")
else
    jarDir="$SCHEDULER_HOME/jar"
    javaHome="$JAVA_HOME"
fi

if [ ! -d "$SCHEDULER_DATA" ]; then :
    echo Please set environment variable SCHEDULER_DATA with your data directory
    exit 1
fi
