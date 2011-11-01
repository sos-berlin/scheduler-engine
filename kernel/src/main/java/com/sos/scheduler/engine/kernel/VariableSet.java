package com.sos.scheduler.engine.kernel;

import javax.annotation.Nullable;

import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.cppproxy.Variable_setC;

@ForCpp public class VariableSet implements Sister, UnmodifiableVariableSet {
    private final Variable_setC cppProxy;

    VariableSet(Variable_setC cppProxy) {
        this.cppProxy = cppProxy;
    }

    @Override public void onCppProxyInvalidated() {
    }

    @Override @Nullable public final String tryGet(String name) {
        return cppProxy.get_string(name);
    }

    @Override public final String get(String name) {
        String result = tryGet(name);
        if (result == null)  throw new SchedulerException("Missing parameter '" + name + "'");
        return result;
    }

    public final void put(String name, String value) { 
        cppProxy.set_var(name, value);
    }

    public static class Type implements SisterType<VariableSet, Variable_setC> {
        @Override public final VariableSet sister(Variable_setC proxy, Sister context) {
            assert context == null;
            return new VariableSet(proxy);
        }
    }
}
