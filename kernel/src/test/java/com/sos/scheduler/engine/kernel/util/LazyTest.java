package com.sos.scheduler.engine.kernel.util;

import static org.hamcrest.CoreMatchers.equalTo;
import static org.hamcrest.MatcherAssert.assertThat;

import org.junit.Test;

public class LazyTest {
    private int i = 0;

    @Test public void testApply() throws Exception {
        Lazy<Integer> lazy = new Lazy<Integer>() {
            protected Integer compute() { return ++i; }
        };
        assertThat(lazy.apply(), equalTo(1));
        assertThat(lazy.apply(), equalTo(1));
    }
}
