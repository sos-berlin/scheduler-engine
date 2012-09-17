package com.sos.scheduler.engine.kernel.command;

import org.w3c.dom.Element;

/** Typsichere abstrakte Implementierung von ResultXmlizer. */
public abstract class GenericResultXmlizer<R extends Result> implements ResultXmlizer {
    private final Class<R> resultClass;
    
    protected GenericResultXmlizer(Class<R> resultClass) {
        this.resultClass = resultClass;
    }

    @Override public final Class<? extends Result> getResultClass() {
        return resultClass;
    }

    @SuppressWarnings("unchecked")
    @Override public final Element toElement(Result r) {
        assert resultClass.isInstance(r);
        return doToElement((R)r);
    }

    protected abstract Element doToElement(R r);
}
