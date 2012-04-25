package com.sos.scheduler.engine.data.log;

import com.sos.scheduler.engine.data.event.AbstractEvent;

import javax.annotation.Nullable;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public abstract class LogEvent extends AbstractEvent {
    private static final Pattern codePattern = Pattern.compile("([A-Z]+(-[0-9A-Z]+)+)( .*)?");
    private final String line;

    protected LogEvent(String message) {
        this.line = message;
    }

    @Nullable public final String getCodeOrNull() {
        return messageCodeFromLineOrNull(line);
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
}
