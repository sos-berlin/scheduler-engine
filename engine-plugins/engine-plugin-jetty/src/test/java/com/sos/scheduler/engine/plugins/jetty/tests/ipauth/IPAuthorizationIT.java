package com.sos.scheduler.engine.plugins.jetty.tests.ipauth;

import com.google.common.base.Charsets;
import com.google.common.io.Files;
import com.sos.scheduler.engine.common.utils.JavaResource;
import com.sos.scheduler.engine.plugins.jetty.configuration.Config;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.Sockets;
import com.sun.jersey.api.client.Client;
import com.sun.jersey.api.client.ClientResponse;
import com.sun.jersey.api.client.WebResource;
import java.io.File;
import java.io.IOException;
import java.net.URI;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import static org.hamcrest.Matchers.equalTo;
import static org.junit.Assert.assertThat;

public class IPAuthorizationIT extends SchedulerTest {

	private static final Logger logger = LoggerFactory.getLogger(IPAuthorizationIT.class);

    private static final String xmlCommand = "<show_state />";
    private static final int tcpPort = Sockets.findAvailablePort();

    private static final JavaResource jettyXmlTemplateResource = JavaResource.apply("com/sos/scheduler/engine/plugins/jetty/tests/ipauth/jetty-template.xml");

    private final String ipToTest;
    private final ClientResponse.Status expectedStatus;

    protected IPAuthorizationIT(String ipToTest, ClientResponse.Status expectedStatus) {
        this.ipToTest = ipToTest;
        this.expectedStatus = expectedStatus;
    }

    protected void doTest() throws Exception {
        controller().prepare();
        prepareEnvironment();
        controller().activateScheduler();
        assertThat(doHttpRequest().getStatus(), equalTo(expectedStatus.getStatusCode()));
        controller().terminateScheduler();
    }

    private ClientResponse doHttpRequest() throws Exception {
        URI uri = new URI("http://localhost:"+ tcpPort + Config.contextPath() + Config.cppPrefixPath());
        Client c = Client.create();
        WebResource webResource = c.resource(uri);
        ClientResponse response = webResource.post(ClientResponse.class, xmlCommand);
        logger.debug("Response for " + uri.toASCIIString() + "/" + xmlCommand + ": " + response.getStatusInfo());
        c.destroy();
        return response;
    }

    private void prepareEnvironment() throws Exception {
        File tempDir = controller().environment().configDirectory();
        prepareAndWriteJettyXml(tempDir);
    }

    private void prepareAndWriteJettyXml(File tempDir) throws IOException {
        String content = jettyXmlTemplateResource.asUTF8String();
        String newContent = content.replace("${tcp.port}", String.valueOf(tcpPort));
        newContent = newContent.replace("${ip.number}", ipToTest);
        newContent = newContent.replace("${method.name}", expectedStatus == ClientResponse.Status.OK? "white" : "black");
        File targetFile = new File(tempDir, "jetty.xml");
        Files.write(newContent, targetFile, Charsets.UTF_8);
        logger.debug("file " + targetFile.getAbsolutePath() + " created");
    }
}
