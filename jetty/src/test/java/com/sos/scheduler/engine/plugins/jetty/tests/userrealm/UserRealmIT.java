package com.sos.scheduler.engine.plugins.jetty.tests.userrealm;

import com.google.common.base.Charsets;
import com.google.common.io.Files;
import com.google.common.io.Resources;
import com.sos.scheduler.engine.plugins.jetty.configuration.Config;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.Sockets;
import com.sun.jersey.api.client.Client;
import com.sun.jersey.api.client.ClientResponse;
import com.sun.jersey.api.client.WebResource;
import com.sun.jersey.api.client.filter.HTTPBasicAuthFilter;
import java.io.File;
import java.io.IOException;
import java.net.URI;
import java.net.URL;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import static com.sun.jersey.api.client.ClientResponse.Status.OK;
import static com.sun.jersey.api.client.ClientResponse.Status.UNAUTHORIZED;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.equalTo;

public class UserRealmIT extends SchedulerTest {

	private static final Logger logger = LoggerFactory.getLogger(UserRealmIT.class);

    private static final String jettyXmlTemplateResourcePath = "com/sos/scheduler/engine/plugins/jetty/tests/userrealm/jetty-template.xml";
    private static final String realmPropertiesPath = "com/sos/scheduler/engine/plugins/jetty/tests/userrealm/realm.properties";

    private static final String xmlCommand = "<show_state />";
    private static final int tcpPort = Sockets.findAvailablePort();

    @Test
    public void test() throws Exception {
        controller().prepare();
        prepareEnvironment();
        controller().activateScheduler();
        assertThat(doHttpRequest("testuser","testpassword").getStatus(), equalTo(OK.getStatusCode()));
        assertThat(doHttpRequest("invaliduser","invalidpassword").getStatus(), equalTo(UNAUTHORIZED.getStatusCode()));
    }

    private ClientResponse doHttpRequest(String user, String password) throws Exception {
        URI uri = new URI("http://localhost:"+ tcpPort + Config.contextPath() + Config.cppPrefixPath() );
        logger.debug("{}/{}", uri.toASCIIString(), xmlCommand);
        Client c = Client.create();
        // c.addFilter( new LoggingFilter(System.out));
        c.addFilter( new HTTPBasicAuthFilter(user, password));
        WebResource webResource = c.resource(uri);
        ClientResponse response = webResource.post(ClientResponse.class, xmlCommand);
        logger.debug("Response_code: {}", response.getStatusInfo());
        // logResult(response);
        c.destroy();
        return response;
    }

    /*
    private void logResult(ClientResponse r) throws Exception {
        String s = CharStreams.toString((new InputStreamReader(r.getEntityInputStream(), "UTF-8")));
        logger.debug(s);
    }
    */

    private void prepareEnvironment() throws Exception {
        File tempDir = controller().environment().configDirectory();
        File target = copyRealmFileToConfiguration(tempDir);
        prepareAndWriteJettyXml(tempDir, target);
    }

    private void prepareAndWriteJettyXml(File tempDir, File realmFile) throws IOException {
        URL sourceFile = Resources.getResource(jettyXmlTemplateResourcePath);
        String content = Resources.toString(sourceFile, Charsets.UTF_8);
        String newContent = content.replace("${tcp.port}", String.valueOf(tcpPort));
        newContent = newContent.replace("${realm.properties}", realmFile.getAbsolutePath() );
        File targetFile = new File(tempDir, "jetty.xml");
        Files.write(newContent, targetFile, Charsets.UTF_8);
        logger.debug("File " + targetFile.getAbsolutePath() + " created with reference to " + realmFile.getAbsolutePath());
    }

    private File copyRealmFileToConfiguration(File tempDir) throws IOException {
        URL sourceFile = Resources.getResource(realmPropertiesPath);
        String sourceFileContent = Resources.toString(sourceFile, Charsets.UTF_8);
        File targetFile = new File(tempDir, new File(realmPropertiesPath).getName());
        Files.write( sourceFileContent, targetFile, Charsets.UTF_8 );
        logger.debug("File " + targetFile.getAbsolutePath() + " created");
        return targetFile;
    }
}
