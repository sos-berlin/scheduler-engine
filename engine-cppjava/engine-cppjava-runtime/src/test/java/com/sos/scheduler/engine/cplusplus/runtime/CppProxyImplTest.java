package com.sos.scheduler.engine.cplusplus.runtime;

import org.junit.*;
import static org.junit.Assert.*;

public class CppProxyImplTest {
    private final CppProxyWithSister<MySister> p = new CppProxyImpl<MySister>();
    private final MySister sister = new MySister();

    @Test public void testCppReferenceIsValid() {
        assertFalse(p.cppReferenceIsValid());
    }
    
    @Test public void testHasSister() {
        assertFalse(p.hasSister());
        p.setSister(sister);
        assertTrue(p.hasSister());
    }

    @Test(expected=RuntimeException.class) public void testGetSister() {
        MySister s = p.getSister();
    }

    @Test public void testGetSister2() {
        p.setSister(sister);
        assert sister == p.getSister();
    }

    static class MySister implements Sister {
        @Override public void onCppProxyInvalidated() {}
    }
}
