package com.sos.scheduler.engine.plugins.jetty.tests.userrealm;

import com.google.common.base.Charsets;
import com.google.common.io.Files;
import com.sos.scheduler.engine.common.utils.JavaResource;
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
import org.junit.Test;

import static com.sun.jersey.api.client.ClientResponse.Status.OK;
import static com.sun.jersey.api.client.ClientResponse.Status.UNAUTHORIZED;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.equalTo;

public class UserRealmIT extends SchedulerTest {

    private static final JavaResource jettyXmlTemplateResource = JavaResource.apply("com/sos/scheduler/engine/plugins/jetty/tests/userrealm/jetty-template.xml");
    private static final JavaResource realmPropertiesResource = JavaResource.apply("com/sos/scheduler/engine/plugins/jetty/tests/userrealm/realm.properties");

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

    private static ClientResponse doHttpRequest(String user, String password) throws Exception {
        URI uri = new URI("http://localhost:"+ tcpPort + Config.contextPath() + Config.cppPrefixPath() );
        Client c = Client.create();
        // c.addFilter( new LoggingFilter(System.out));
        c.addFilter(new HTTPBasicAuthFilter(user, password));
        WebResource webResource = c.resource(uri);
        ClientResponse response = webResource.post(ClientResponse.class, xmlCommand);
        c.destroy();
        return response;
    }

    private void prepareEnvironment() throws Exception {
        File tempDir = controller().environment().configDirectory();
        File target = copyRealmFileToConfiguration(tempDir);
        prepareAndWriteJettyXml(tempDir, target);
    }

    private static void prepareAndWriteJettyXml(File tempDir, File realmFile) throws IOException {
        String content = jettyXmlTemplateResource.asUTF8String()
                .replace("${tcp.port}", String.valueOf(tcpPort))
                .replace("${realm.properties}", realmFile.getAbsolutePath() );
        File targetFile = new File(tempDir, "jetty.xml");
        Files.write(content, targetFile, Charsets.UTF_8);
    }

    private static File copyRealmFileToConfiguration(File tempDir) throws IOException {
        String content = realmPropertiesResource.asUTF8String();
        File targetFile = new File(tempDir, new File(realmPropertiesResource.path()).getName());
        Files.write(content, targetFile, Charsets.UTF_8);
        return targetFile;
    }
}
