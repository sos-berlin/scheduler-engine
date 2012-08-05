package com.sos.scheduler.engine.tests.scheduler.guice;

import javax.inject.Inject;

public final class X {
    private final I i;
    private final BI b;

    @Inject public X(I i, BI b) {
        this.i = i;
        this.b = b;
    }

    I getI() {
        return i;
    }

    public BI getB() {
        return b;
    }
}
