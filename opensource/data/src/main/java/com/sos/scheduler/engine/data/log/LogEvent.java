package com.sos.scheduler.engine.data.log;

import com.sos.scheduler.engine.data.event.AbstractEvent;

import javax.annotation.Nullable;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class LogEvent extends AbstractEvent {
    private static final Pattern codePattern = Pattern.compile("([A-Z]+(-[0-9A-Z]+)+)( .*)?");

    private final LogLevel level;
    private final String line;

    protected LogEvent(LogLevel level, String message) {
        this.level = level;
        this.line = message;
    }

    @Nullable public final String getCodeOrNull() {
        return messageCodeFromLineOrNull(line);
    }

    public final LogLevel level() {
        return level;
    }

    public final String getLine() {
        return line;
    }

    @Override public String toString() {
        return super.toString() +", line="+ line;
    }

    @Nullable static String messageCodeFromLineOrNull(String line) {
        Matcher m = codePattern.matcher(line);
        return m.matches()? m.group(1) : null;
    }

    public static LogEvent of(LogLevel level, String line) {
        switch (level) {
            case info: return new InfoLogEvent(line);
            case warning: return new WarningLogEvent(line);
            case error: return new ErrorLogEvent(line);
            default: return new LogEvent(level, line);
        }
    }
}
