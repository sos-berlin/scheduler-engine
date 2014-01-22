package com.sos.scheduler.engine.plugins.jetty.ipauth.allowed;

import com.sos.scheduler.engine.plugins.jetty.ipauth.IPAuthorizationIT;
import com.sun.jersey.api.client.ClientResponse;
import org.junit.Test;

public class IPAllowedIT extends IPAuthorizationIT {

    private static final String ipAllowed = "127.0.0.1";

    public IPAllowedIT() {
        super(ipAllowed, ClientResponse.Status.OK);
    }

    @Test
    public void test() throws Exception {
        doTest();
    }

}
