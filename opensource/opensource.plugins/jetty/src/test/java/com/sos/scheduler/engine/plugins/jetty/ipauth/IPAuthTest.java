package com.sos.scheduler.engine.plugins.jetty.ipauth;

import com.google.common.base.Charsets;
import com.google.common.io.CharStreams;
import com.google.common.io.Files;
import com.sos.scheduler.engine.plugins.jetty.Config;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.Sockets;
import com.sun.jersey.api.client.Client;
import com.sun.jersey.api.client.ClientResponse;
import com.sun.jersey.api.client.WebResource;
import org.apache.log4j.Logger;
import org.junit.Test;

import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.InetAddress;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;

import static org.hamcrest.Matchers.equalTo;
import static org.junit.Assert.assertThat;

public class IPAuthTest extends SchedulerTest {

	@SuppressWarnings("unused")
	private static Logger logger = Logger.getLogger(IPAuthTest.class);

    private static final String xmlCommand = "<show_state />";
    private static final int tcpPort = Sockets.findAvailablePort();

    private static final String ipAllowed = "127.0.0.1";

    //TODO die verwendete IpForbidden ist nicht allgemeingültig
    //private static final String ipForbidden = "192.11.0.79";
    private static String ipForbidden = null;



    @Test
    public void test() throws Exception {
        controller().prepare();
        prepareEnvironment();
        controller().activateScheduler();
        ipForbidden = InetAddress.getLocalHost().getHostAddress();
        logger.debug("the external IP is " + ipForbidden);
        assertThat(doHttpRequest(ipAllowed).getClientResponseStatus(), equalTo(ClientResponse.Status.OK));
        assertThat(doHttpRequest(ipForbidden).getClientResponseStatus(), equalTo(ClientResponse.Status.FORBIDDEN));
        controller().terminateScheduler();
    }

    private ClientResponse doHttpRequest(String clientIp) throws Exception {
        URI uri = new URI("http://localhost:"+ tcpPort + Config.contextPath() + Config.cppPrefixPath());
        //TODO wie kann man dem Client sagen, mit welcher IP er sich am Server anmelden soll?
        Client c = Client.create();
        WebResource webResource = c.resource(uri);
        ClientResponse response = webResource.post(ClientResponse.class, xmlCommand);
        logger.info("response for " + uri.toASCIIString() + "/" + xmlCommand + ": " + response.getClientResponseStatus());
        c.destroy();
        // logResult(response);
        return response;
    }

    private void prepareEnvironment() throws Exception {
        File tempDir = controller().environment().configDirectory();
        prepareAndWriteJettyXml(tempDir);
    }

    private void prepareAndWriteJettyXml(File tempDir) throws IOException, URISyntaxException {
        URL sourceFile = this.getClass().getResource("jetty.xml.template");
        String targetFilename = tempDir.getAbsolutePath() + "/jetty.xml";
        String content = Files.toString(new File(sourceFile.toURI()), Charsets.UTF_8);
        String newContent = content.replace("${tcp.port}", String.valueOf(tcpPort));
        Files.write(newContent, new File(targetFilename), Charsets.UTF_8);
        logger.debug("file " + targetFilename + " created");
    }

    private void logResult(ClientResponse r) throws Exception {
        String s = CharStreams.toString((new InputStreamReader(r.getEntityInputStream(), "UTF-8")));
        logger.debug(s);
    }
}
