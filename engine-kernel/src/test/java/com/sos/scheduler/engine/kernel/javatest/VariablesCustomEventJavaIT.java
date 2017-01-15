package com.sos.scheduler.engine.kernel.javatest;

import com.sos.scheduler.engine.data.event.KeyedEvent;
import com.sos.scheduler.engine.data.events.custom.VariablesCustomEvent;
import com.sos.scheduler.engine.eventbus.JavaEventSubscription;
import com.sos.scheduler.engine.eventbus.SchedulerEventBus;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import org.junit.Test;
import static org.junit.Assert.assertEquals;

/**
 * @author Joacim Zschimmer
 */
public class VariablesCustomEventJavaIT {

    @Test
    public void test() throws ExecutionException, InterruptedException, TimeoutException {
        SchedulerEventBus eventBus = new SchedulerEventBus();
        CompletableFuture<KeyedEvent<VariablesCustomEvent>> future = new CompletableFuture<>();
        eventBus.subscribeHot(new JavaEventSubscription<>(VariablesCustomEvent.class, future::complete));  // Not unsubscribed in this test.
        KeyedEvent<VariablesCustomEvent> keyedEvent = newKeyedEvent();
        eventBus.publishJava(keyedEvent);
        assertEquals(future.get(0, TimeUnit.SECONDS), keyedEvent);
    }

    private KeyedEvent<VariablesCustomEvent> newKeyedEvent() {
        Map<String,String> variables = new HashMap<>();
        variables.put("test-key", "test-variable");
        return VariablesCustomEvent.keyed("KEY", variables);
    }
}
