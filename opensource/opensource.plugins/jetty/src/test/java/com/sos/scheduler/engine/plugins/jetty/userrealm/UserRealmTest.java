package com.sos.scheduler.engine.plugins.jetty.userrealm;

import com.google.common.base.Charsets;
import com.google.common.io.CharStreams;
import com.google.common.io.Files;
import com.sos.scheduler.engine.plugins.jetty.Config;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.Sockets;
import com.sun.jersey.api.client.Client;
import com.sun.jersey.api.client.ClientResponse;
import com.sun.jersey.api.client.WebResource;
import com.sun.jersey.api.client.filter.HTTPBasicAuthFilter;
import org.apache.log4j.Logger;
import org.junit.Test;

import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.URI;
import java.net.URISyntaxException;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.equalTo;

public class UserRealmTest extends SchedulerTest {

	@SuppressWarnings("unused")
	private static Logger logger = Logger.getLogger(UserRealmTest.class);

    private static final String xmlCommand = "<show_state />";
    private static final int tcpPort = Sockets.findAvailablePort();

    @Test
    public void test() throws Exception {
        controller().prepare();
        prepareEnvironment();
        controller().activateScheduler();
        assertThat(doHttpRequest("testuser","testpassword").getClientResponseStatus(), equalTo(ClientResponse.Status.OK));
        assertThat(doHttpRequest("invaliduser","invalidpassword").getClientResponseStatus(), equalTo(ClientResponse.Status.UNAUTHORIZED));
    }

    private ClientResponse doHttpRequest(String user, String password) throws Exception {
        URI uri = new URI("http://localhost:"+ tcpPort + Config.contextPath() + Config.cppPrefixPath() );
        logger.debug(uri.toASCIIString() + "/" + xmlCommand);
        Client c = Client.create();
        // c.addFilter( new LoggingFilter(System.out));
        c.addFilter( new HTTPBasicAuthFilter(user, password));
        WebResource webResource = c.resource(uri);
        ClientResponse response = webResource.post(ClientResponse.class, xmlCommand);
        logger.debug("response_code: " + response.getClientResponseStatus());
        // logResult(response);
        c.destroy();
        return response;
    }

    private void logResult(ClientResponse r) throws Exception {
        String s = CharStreams.toString((new InputStreamReader(r.getEntityInputStream(), "UTF-8")));
        logger.debug(s);
    }

    private void prepareEnvironment() throws Exception {
        File tempDir = controller().environment().configDirectory();
        File target = copyRealmFileToConfiguration(tempDir);
        prepareAndWriteJettyXml(tempDir, target);
    }

    private void prepareAndWriteJettyXml(File tempDir, File realmFile) throws IOException, URISyntaxException {
        String sourceFilename = this.getClass().getResource("jetty.xml.template").toExternalForm().replace("file:/", "");
        String targetFilename = tempDir.getAbsolutePath() + "/jetty.xml";
        String content = Files.toString(new File(sourceFilename), Charsets.UTF_8);
        String newContent = content.replace("${tcp.port}", String.valueOf(tcpPort));
        newContent = newContent.replace("${realm.properties}", realmFile.getAbsolutePath() );
        Files.write(newContent, new File(targetFilename), Charsets.UTF_8);
        logger.debug("file " + targetFilename + " created with reference to " + realmFile.getAbsolutePath());
    }

    private File copyRealmFileToConfiguration(File tempDir) throws IOException {
        String sourceFilename = this.getClass().getResource("realm.properties").toExternalForm().replace("file:/","");
        String targetFilename = tempDir.getAbsolutePath() + "/realm.properties";
        File targetFile = new File(targetFilename);
        Files.copy( new File(sourceFilename), targetFile );
        logger.debug("file " + targetFilename + " created");
        return targetFile;
    }
}
