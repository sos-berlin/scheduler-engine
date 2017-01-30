# Exports environment variables
# - JAVA_HOME

# And sets variables
# - SCHEDULER_HOME
# - SCHEDULER_DATA
# - jarDir

pathSeparator=$(if [ "$OSTYPE" = "cygwin" ]; then echo ";"; else echo ":"; fi)

declare JAVA_HOME
if [ -z "$JAVA_HOME" ]; then
    export JAVA_HOME=$(dirname $(dirname $(readlink --canonicalize $(which java || kill $$))))
fi

declare SCHEDULER_HOME
if [ -z "$SCHEDULER_HOME" ]; then :
    SCHEDULER_HOME=$(cd "$(dirname -- "$0")/.." && pwd || kill $$)
fi

declare OSTYPE
if [ "$OSTYPE" = "cygwin" ]; then
    jarDir=$(cygpath -w "$SCHEDULER_HOME/jar" || kill $$)
    javaHome=""
    [ -n "$JAVA_HOME" ] && javaHome=$(cygpath "$JAVA_HOME" || kill $$)
else
    jarDir="$SCHEDULER_HOME/jar"
    javaHome="$JAVA_HOME"
fi

declare SCHEDULER_DATA
if [ "$SCHEDULER_DATA" = '$SCHEDULER_HOME/data' ]; then :
    export SCHEDULER_DATA="$SCHEDULER_HOME"/data
fi
if [ -z "$SCHEDULER_DATA" ]; then :
    export SCHEDULER_DATA="$SCHEDULER_HOME"/data
    echo "WARN  Please set environment variable SCHEDULER_DATA with your data directory"
    echo "WARN  Using SCHEDULER_DATA=$SCHEDULER_DATA (for demonstration only)"
fi
if [ ! -d "$SCHEDULER_DATA" ]; then :
    echo "ERROR No such directory SCHEDULER_DATA=$SCHEDULER_DATA"
    exit 1
fi
