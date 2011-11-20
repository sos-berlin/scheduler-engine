package com.sos.scheduler.engine.kernel;

import static org.apache.commons.collections.ListUtils.unmodifiableList;

import java.util.Collection;

import javax.annotation.Nullable;

import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.cppproxy.Variable_setC;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;

@ForCpp public final class VariableSet implements Sister, UnmodifiableVariableSet {
    private final Variable_setC cppProxy;

    VariableSet(Variable_setC cppProxy) {
        this.cppProxy = cppProxy;
    }

    @Override public void onCppProxyInvalidated() {}

    public int size() {
        return cppProxy.count();
    }

    @SuppressWarnings("unchecked")
    public Collection<String> getNames() {
        return unmodifiableList(cppProxy.java_names());
    }

    @Override @Nullable public String tryGet(String name) {
        return cppProxy.get_string(name);
    }

    @Override public String get(String name) {
        String result = tryGet(name);
        if (result == null)  throw new SchedulerException("Missing parameter '" + name + "'");
        return result;
    }

    public void put(String name, String value) {
        cppProxy.set_var(name, value);
    }

//    public final Map<String,String> asMap() {
//        final VariableSet variableSet = this;
//
//        return new Map<String,String>() {
//            @Override public int size() {
//                return variableSet.size();
//            }
//
//            @Override public boolean isEmpty() {
//                return size() > 0;
//            }
//
//            @Override public boolean containsKey(Object key) {
//                return get(key) != null;
//            }
//
//            @Override public String get(Object key) {
//                return key instanceof String? variableSet.tryGet((String)key) : null;
//            }
//
//            @Override public String put(String key, String value) {
//                variableSet.put(key, value);
//            }
//        };
//    }

    public static class Type implements SisterType<VariableSet, Variable_setC> {
        @Override public final VariableSet sister(Variable_setC proxy, Sister context) {
            assert context == null;
            return new VariableSet(proxy);
        }
    }
}
