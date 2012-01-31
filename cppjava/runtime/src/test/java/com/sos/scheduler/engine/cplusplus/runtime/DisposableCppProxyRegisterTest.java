package com.sos.scheduler.engine.cplusplus.runtime;

import org.junit.Test;

import java.util.HashSet;
import java.util.Set;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.contains;
import static org.hamcrest.Matchers.containsInAnyOrder;
import static org.junit.Assert.assertTrue;

public final class DisposableCppProxyRegisterTest {
    private final DisposableCppProxyRegister register = new DisposableCppProxyRegister();
    private final Set<MockCppProxy> releasedProxies = new HashSet<MockCppProxy>();
    private final MockCppProxy a = new MockCppProxy("a");
    private final MockCppProxy b = new MockCppProxy("b");
    private final CppReference<MockCppProxy> aRef = register.reference(a);
    private final CppReference<MockCppProxy> bRef = register.reference(b);

    @Test public void testDispose() {
        assertTrue(releasedProxies.isEmpty());
        register.dispose(aRef);
        assertThat(releasedProxies, contains(a));
    }

    @Test public void testDisposeAll() {
        register.tryDisposeAll();
        assertThat(releasedProxies, containsInAnyOrder(a, b));
    }

    @Test(expected=RuntimeException.class) public void testDuplicateDispose() {
        register.dispose(aRef);
        register.dispose(aRef);
    }

    @Test(expected=RuntimeException.class) public void afterDisposeAllReferenceShouldBeRejected() {
        register.tryDisposeAll();
        register.reference(new MockCppProxy("new"));
    }

    @Test public void afterDisposeAllDisposeShouldBeIgnored() {
        register.tryDisposeAll();
        register.dispose(aRef);
    }

    class MockCppProxy implements ReleasableCppProxy {
        private final String name;

        MockCppProxy(String name) {
            this.name = name;
        }

        @Override public void Release() {
            releasedProxies.add(this);
        }

        @Override public boolean cppReferenceIsValid() {
            return true;
        }

        @Override public String toString() {
            return name;
        }
    }
}
