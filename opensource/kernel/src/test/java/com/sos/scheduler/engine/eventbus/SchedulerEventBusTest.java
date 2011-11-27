package com.sos.scheduler.engine.eventbus;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.equalTo;
import static org.hamcrest.Matchers.nullValue;

import org.junit.Test;

public final class SchedulerEventBusTest {
    private final SchedulerEventBus eventBus = new SchedulerEventBus();
    private final Annotated annotated = new Annotated();

    public SchedulerEventBusTest() {
        eventBus.registerAnnotated(annotated);
    }

    @Test public void testSuperclassEvent() {
        eventBus.publish(new A());
        assertCounters(1, 0, 0, 0, 0, 0, 0, 0);
        eventBus.dispatchEvents();
        assertCounters(1, 1, 0, 0, 0, 0, 0, 0);
    }

    @Test public void testUnregisterAnnotated() {
        eventBus.unregisterAnnotated(annotated);
        eventBus.publish(new A());
        assertCounters(0, 0, 0, 0, 0, 0, 0, 0);
        eventBus.dispatchEvents();
        assertCounters(0, 0, 0, 0, 0, 0, 0, 0);
    }

    @Test public void testSubclassEvent() {
        eventBus.publish(new AA());
        assertCounters(1, 0, 1, 0, 0, 0, 0, 0);
        eventBus.dispatchEvents();
        assertCounters(1, 1, 1, 1, 0, 0, 0, 0);
    }

    @Test public void testB() {
        eventBus.publish(new B());
        assertCounters(0, 0, 0, 0, 1, 0, 0, 0);
        eventBus.dispatchEvents();
        assertCounters(0, 0, 0, 0, 1, 1, 0, 0);
    }

    @Test public void testSuperclassEventSource() {
        eventBus.publish(new A(), new S());
        assertCounters(1, 0, 0, 0, 0, 0, 1, 0);
        eventBus.dispatchEvents();
        assertCounters(1, 1, 0, 0, 0, 0, 1, 0);
    }

    @Test public void testSubclassEventSource() {
        eventBus.publish(new A(), new T());
        assertCounters(1, 0, 0, 0, 0, 0, 1, 1);
        eventBus.dispatchEvents();
        assertCounters(1, 1, 0, 0, 0, 0, 1, 1);
    }

    private void assertCounters(int aHot, int a, int aaHot, int aa, int bHot, int b, int aSHot, int aTHot) {
        assertThat(annotated.throwable, nullValue());
        assertThat(annotated.aHot, equalTo(aHot));
        assertThat(annotated.a, equalTo(a));
        assertThat(annotated.aaHot, equalTo(aaHot));
        assertThat(annotated.aa, equalTo(aa));
        assertThat(annotated.bHot, equalTo(bHot));
        assertThat(annotated.b, equalTo(b));
        assertThat(annotated.aSHot, equalTo(aSHot));
        assertThat(annotated.aTHot, equalTo(aTHot));
    }

    static final class Annotated implements EventHandlerAnnotated {
        private int aHot = 0;
        private int a = 0;
        private int aaHot = 0;
        private int aa = 0;
        private int bHot = 0;
        private int b = 0;
        private int aSHot = 0;
        private int aTHot = 0;
        private Throwable throwable = null;

        @HotEventHandler public void handleHot(A event) {
            aHot += 1;
        }

        @EventHandler public void handle(A event) {
            a += 1;
        }

        @HotEventHandler public void handleHot(AA event) {
            aaHot += 1;
        }

        @EventHandler public void handle(AA event) {
            aa += 1;
        }

        @HotEventHandler public void handleHot(B event) {
            bHot += 1;
        }

        @EventHandler public void handle(B event) {
            b += 1;
        }

        @HotEventHandler public void handleHot(A event, S s) {
            aSHot += 1;
        }

        @HotEventHandler public void handleHot(A event, T t) {
            aTHot += 1;
        }

        @HotEventHandler @EventHandler public void handleError(EventHandlerFailedEvent e) {
            throwable = e.getThrowable();
        }
    }

    static class A extends AbstractEvent {}
    static class AA extends A {}
    static class B extends AbstractEvent {}

    static class S implements EventSource {}
    static class T extends S {}
}
