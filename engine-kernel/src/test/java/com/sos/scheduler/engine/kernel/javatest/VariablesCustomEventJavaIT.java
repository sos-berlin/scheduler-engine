package com.sos.scheduler.engine.kernel.javatest;

import com.sos.scheduler.engine.data.event.KeyedEvent;
import com.sos.scheduler.engine.data.events.custom.VariablesCustomEvent;
import com.sos.scheduler.engine.eventbus.ColdEventBus;
import com.sos.scheduler.engine.eventbus.EventBus;
import com.sos.scheduler.engine.eventbus.EventSubscription;
import com.sos.scheduler.engine.eventbus.JavaEventSubscription;
import com.sos.scheduler.engine.eventbus.SchedulerEventBus;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingDeque;
import org.junit.Test;
import static java.util.concurrent.TimeUnit.SECONDS;
import static org.junit.Assert.assertEquals;

/**
 * @author Joacim Zschimmer
 */
public class VariablesCustomEventJavaIT {

    @Test
    public void test() throws InterruptedException {
        SchedulerEventBus eventBus = new SchedulerEventBus();
        BlockingQueue<KeyedEvent<VariablesCustomEvent>> queue = new LinkedBlockingDeque<>();

        EventSubscription subscription = subscribe(eventBus, queue);
        KeyedEvent<VariablesCustomEvent> keyedEvent = newKeyedEvent();

        publishEvent(eventBus.coldEventBus(), keyedEvent);
        emulateSchedulerAction(eventBus);
        assertEquals(keyedEvent, queue.poll(3, SECONDS));

        eventBus.unsubscribe(subscription);
    }

    // Publisher side
    private static void publishEvent(ColdEventBus eventBus, KeyedEvent<VariablesCustomEvent> keyedEvent) {
        eventBus.publishJava(keyedEvent);
    }

    private KeyedEvent<VariablesCustomEvent> newKeyedEvent() {
        Map<String,String> variables = new HashMap<>();
        variables.put("test-key", "test-variable");
        return VariablesCustomEvent.keyed("KEY", variables);
    }

    private static void emulateSchedulerAction(SchedulerEventBus eventBus) {
        eventBus.dispatchEvents();
    }

    // Subscription side
    private static EventSubscription subscribe(EventBus eventBus, BlockingQueue<KeyedEvent<VariablesCustomEvent>> queue) {
        EventSubscription subscription = new JavaEventSubscription<>(
            VariablesCustomEvent.class,
            keyedEvent -> {
                try {
                    queue.put(keyedEvent);
                } catch (InterruptedException e) {
                    throw new RuntimeException(e);  // Java enforces this. Nobody will kill the SchedulerEventBus thread
                }
            });
        eventBus.subscribe(subscription);
        return subscription;
    }
}
