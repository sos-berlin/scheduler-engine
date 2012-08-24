package com.sos.scheduler.engine.plugins.jetty.ipauth.forbidden;

import com.sos.scheduler.engine.plugins.jetty.ipauth.IPAuthorizationTest;
import com.sun.jersey.api.client.ClientResponse;
import org.junit.Test;

public class IPForbiddenTest extends IPAuthorizationTest {

    private static final String ipForbidden = "127.0.0.1";

    public IPForbiddenTest() {
        super(ipForbidden, ClientResponse.Status.FORBIDDEN);
    }

    @Test
    public void test() throws Exception {
        doTest();
    }

}
