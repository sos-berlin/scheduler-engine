package com.sos.scheduler.engine.plugins.jetty.ipauth;

import com.google.common.io.CharStreams;
import com.sos.scheduler.engine.plugins.jetty.Config;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sun.jersey.api.client.Client;
import com.sun.jersey.api.client.ClientResponse;
import com.sun.jersey.api.client.WebResource;
import org.apache.log4j.Logger;
import org.junit.Test;

import java.io.InputStreamReader;
import java.net.URI;

import static org.hamcrest.Matchers.equalTo;
import static org.junit.Assert.assertThat;

public class IPAuthTest extends SchedulerTest {

	@SuppressWarnings("unused")
	private static Logger logger = Logger.getLogger(IPAuthTest.class);

    private static final String xmlCommand = "<show_state />";
    //TODO jetty portnummer ist fest eingestellt - mit Sockets.findAvailable port einen variablen port verwenden. Der gleiche Port
    // muss in jetty.xml eingestellt sein,
    private static final String tcpPort = "44441";
    private static final String ipAllowed = "127.0.0.1";

    //TODO die verwendete IpForbidden ist nicht allgemeingültig
    private static final String ipForbidden = "192.11.0.79";


    @Test
    public void testAllowed() throws Exception {
        controller().activateScheduler();
        assertThat(doHttpRequest(ipAllowed).getClientResponseStatus(), equalTo(ClientResponse.Status.OK));
        assertThat(doHttpRequest(ipForbidden).getClientResponseStatus(), equalTo(ClientResponse.Status.FORBIDDEN));
        controller().terminateScheduler();
    }

    private ClientResponse doHttpRequest(String server) throws Exception {
        URI uri = new URI("http://" + server + ":"+ tcpPort + Config.contextPath() + Config.cppPrefixPath());
        WebResource webResource = Client.create().resource(uri);
        ClientResponse response = webResource.post(ClientResponse.class, xmlCommand);
        logger.info("response for " + uri.toASCIIString() + "/" + xmlCommand + ": " + response.getClientResponseStatus());
        // logResult(response);
        return response;
    }


    private void logResult(ClientResponse r) throws Exception {
        String s = CharStreams.toString((new InputStreamReader(r.getEntityInputStream(), "UTF-8")));
        logger.debug(s);
    }
}
