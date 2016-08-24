package com.sos.scheduler.engine.eventbus.annotated;

import com.sos.scheduler.engine.eventbus.HotEventHandler;
import java.lang.annotation.Annotation;

/** Für mit {@link HotEventHandler} annotierte Methoden, die im zweiten Parameter eine
 * {@link com.sos.scheduler.engine.eventbus.EventSource} bekommen können. */
public final class HotMethodEventSubscriptionFactory extends MethodEventSubscriptionFactory {
    public static final HotMethodEventSubscriptionFactory singleton = new HotMethodEventSubscriptionFactory();

    private HotMethodEventSubscriptionFactory() {}

    @Override public Class<? extends Annotation> getAnnotation() {
        return HotEventHandler.class;
    }
}
