package com.sos.scheduler.engine.kernelcpptest.guice;

import javax.inject.Inject;

public class X {
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
