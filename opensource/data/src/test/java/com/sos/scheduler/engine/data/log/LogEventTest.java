package com.sos.scheduler.engine.data.log;

import org.junit.Test;

import static com.sos.scheduler.engine.data.log.LogEvent.messageCodeFromLineOrNull;
import static org.hamcrest.CoreMatchers.equalTo;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.core.IsNull.nullValue;

public class LogEventTest {
    @Test public void test() {
        assertThat(messageCodeFromLineOrNull(""), nullValue());
        assertThat(messageCodeFromLineOrNull(" ABC-123 x"), nullValue());
        assertThat(messageCodeFromLineOrNull("ABC-123"), equalTo("ABC-123"));
        assertThat(messageCodeFromLineOrNull("ABC-123 "), equalTo("ABC-123"));
        assertThat(messageCodeFromLineOrNull("ABC-X123-Y123 "), equalTo("ABC-X123-Y123"));
        assertThat(messageCodeFromLineOrNull("ABC-123 x"), equalTo("ABC-123"));
    }
}
