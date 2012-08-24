package com.sos.scheduler.engine.plugins.jetty.ipauth.allowed;

import com.sos.scheduler.engine.plugins.jetty.ipauth.IPAuthorizationTest;
import com.sun.jersey.api.client.ClientResponse;
import org.junit.Test;

public class IPAllowedTest extends IPAuthorizationTest {

    private static final String ipAllowed = "127.0.0.1";

    public IPAllowedTest() {
        super(ipAllowed, ClientResponse.Status.OK);
    }

    @Test
    public void test() throws Exception {
        doTest();
    }

}
