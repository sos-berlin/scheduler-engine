package com.sos.scheduler.engine.plugins.jetty.tests.ipauth.forbidden;

import com.sos.scheduler.engine.plugins.jetty.tests.ipauth.IPAuthorizationIT;
import com.sun.jersey.api.client.ClientResponse;
import org.junit.Test;

public class IPForbiddenIT extends IPAuthorizationIT {

    private static final String ipForbidden = "127.0.0.1";

    public IPForbiddenIT() {
        super(ipForbidden, ClientResponse.Status.FORBIDDEN);
    }

    @Test
    public void test() throws Exception {
        doTest();
    }

}
