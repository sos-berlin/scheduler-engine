package com.sos.scheduler.engine.kernel;

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

	public final String get(String name) {
        return cppProxy.get_string(name);
    }

	public final String getStrictly(String name) {
        String result = get(name);
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
