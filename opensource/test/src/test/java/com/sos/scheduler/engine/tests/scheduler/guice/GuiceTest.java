package com.sos.scheduler.engine.tests.scheduler.guice;

import static org.hamcrest.Matchers.instanceOf;
import static org.junit.Assert.assertThat;

import org.junit.Test;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;

public class GuiceTest {
    @Test public void test() {
        Injector injector = Guice.createInjector(new MyModule());
        X x = injector.getInstance(X.class);
        assertThat(x.getI(), instanceOf(AI.class));
    }

    static class MyModule extends AbstractModule {
        @Override protected void configure() {
            bind(I.class).to(AI.class);
        }
    }
}
