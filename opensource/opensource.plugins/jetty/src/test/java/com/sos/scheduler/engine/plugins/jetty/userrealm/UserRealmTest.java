package com.sos.scheduler.engine.plugins.jetty.userrealm;

import com.google.common.base.Charsets;
import com.google.common.io.Files;
import com.google.common.io.Resources;
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
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.equalTo;

public class UserRealmTest extends SchedulerTest {

	private static Logger logger = Logger.getLogger(UserRealmTest.class);

    private static final String packageName = "com/sos/scheduler/engine/plugins/jetty/userrealm/";
    private static final String jettyXmlTemplateResourcePath = packageName + "jetty-template.xml";
    private static final String realmPropertiesName = "realm.properties";
    private static final String realmPropertiesPath = packageName + realmPropertiesName;

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
        logger.debug("Response_code: " + response.getClientResponseStatus());
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

    private void prepareAndWriteJettyXml(File tempDir, File realmFile) throws IOException, URISyntaxException {
        URL sourceFile = Resources.getResource(jettyXmlTemplateResourcePath);
        String content = Resources.toString(sourceFile, Charsets.UTF_8);
        String newContent = content.replace("${tcp.port}", String.valueOf(tcpPort));
        newContent = newContent.replace("${realm.properties}", realmFile.getAbsolutePath() );
        File targetFile = new File(tempDir, "jetty.xml");
        Files.write(newContent, targetFile, Charsets.UTF_8);
        logger.debug("File " + targetFile.getAbsolutePath() + " created with reference to " + realmFile.getAbsolutePath());
    }

    private File copyRealmFileToConfiguration(File tempDir) throws IOException, URISyntaxException {
        URL sourceFile = Resources.getResource(realmPropertiesPath);
        String sourceFileContent = Resources.toString(sourceFile, Charsets.UTF_8);
        File targetFile = new File(tempDir, realmPropertiesName);
        Files.write( sourceFileContent, targetFile, Charsets.UTF_8 );
        logger.debug("File " + targetFile.getAbsolutePath() + " created");
        return targetFile;
    }
}
