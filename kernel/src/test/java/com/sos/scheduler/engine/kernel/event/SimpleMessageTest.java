package com.sos.scheduler.engine.kernel.event;

import org.junit.Test;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;

public final class SimpleMessageTest {
    @Test public void test0() {
        SimpleMessage m = new SimpleMessage("CODE");
        assertThat(m.getCode(), equalTo("CODE"));
        assertThat(m.toString(), equalTo("CODE"));
    }

    @Test public void test1() {
        SimpleMessage m = new SimpleMessage("CODE", "INS1");
        assertThat(m.getCode(), equalTo("CODE"));
        assertThat(m.toString(), containsString("CODE"));
        assertThat(m.toString(), containsString("INS1"));
    }

    @Test public void test2() {
        SimpleMessage m = new SimpleMessage("CODE", "INS1", "INS2");
        assertThat(m.getCode(), equalTo("CODE"));
        assertThat(m.toString(), containsString("CODE"));
        assertThat(m.toString(), containsString("INS1"));
        assertThat(m.toString(), containsString("INS2"));
    }
}
