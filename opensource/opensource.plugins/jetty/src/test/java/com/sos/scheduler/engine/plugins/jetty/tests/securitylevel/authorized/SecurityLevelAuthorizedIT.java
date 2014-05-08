package com.sos.scheduler.engine.plugins.jetty.tests.securitylevel.authorized;

import com.google.common.io.Files;
import com.google.common.io.Resources;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import com.sun.jersey.api.client.Client;
import com.sun.jersey.api.client.ClientResponse;
import com.sun.jersey.api.client.WebResource;
import com.sun.jersey.api.client.filter.HTTPBasicAuthFilter;
import org.junit.Test;

import java.io.File;
import java.io.IOException;
import java.net.URI;
import java.net.URL;

import static com.google.common.base.Charsets.UTF_8;
import static com.google.common.io.Resources.getResource;
import static com.sos.scheduler.engine.plugins.jetty.test.JettyPluginTests.contextUri;
import static com.sun.jersey.api.client.ClientResponse.Status.*;
import static org.hamcrest.CoreMatchers.equalTo;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.containsString;


public class SecurityLevelAuthorizedIT extends SchedulerTest {

    private static final String generalPassword = "testpassword";
    private static final String jettyXmlTemplateResourcePath = "com/sos/scheduler/engine/plugins/jetty/tests/securitylevel/authorized/jetty-template.xml";
    private static final String realmPropertiesPath = "com/sos/scheduler/engine/plugins/jetty/tests/securitylevel/authorized/realm.properties";
    private static final String anonymousUser = "anonymous";
    private static final String xmlCommand = new CommandBuilder().startJobImmediately("a").getCommand();

    @Test
    public void test() throws Exception {
        controller().prepare();
        prepareEnvironment();
        controller().activateScheduler();
        testUser("infouser", OK, "SCHEDULER-121");
        testUser("alluser", OK, "task enqueued");
        testUser("nonuser", NOT_FOUND, "");                     // scheduler detects bad request
        testUser("unknownsecleveluser", FORBIDDEN, "");         // role SecurityLevel.unknown not defined in web.xml
        testUser("normaluser", OK, "SCHEDULER-121");
        testUser(anonymousUser, UNAUTHORIZED, "");
    }

    public void testUser(String userName, ClientResponse.Status expectedStatus, String expectedResult) throws Exception {
        Client webClient = Client.create();
        webClient.addFilter(new HTTPBasicAuthFilter(userName, generalPassword));
        WebResource webResource = webClient.resource(new URI(contextUri(scheduler().injector()) + "/jobscheduler/engine-cpp/command"));
        ClientResponse response = webResource.post(ClientResponse.class, xmlCommand);
        assertThat(response.getClientResponseStatus(), equalTo(expectedStatus));
        assertThat(response.getEntity(String.class), containsString(expectedResult));
        webClient.destroy();
    }

    private void prepareEnvironment() throws Exception {
        File tempDir = controller().environment().configDirectory();
        File target = copyRealmFileToConfiguration(tempDir);
        writeJettyXml(jettyXmlTemplateResourcePath, tempDir, target);
    }


    private static File copyRealmFileToConfiguration(File tempDir) throws IOException {
        URL sourceFile = getResource(realmPropertiesPath);
        String sourceFileContent = Resources.toString(sourceFile, UTF_8);
        File targetFile = new File(tempDir, "realm.properties");
        Files.write(sourceFileContent, targetFile, UTF_8);
        return targetFile;
    }

    private static void writeJettyXml(String jettyXmlTemplateResourcePath, File outputDirectory, File realmFile) throws IOException {
        URL sourceFile = getResource(jettyXmlTemplateResourcePath);
        String content = Resources.toString(sourceFile, UTF_8);
        String newContent = content.replace("${realm.properties}", realmFile.getAbsolutePath());
        File targetFile = new File(outputDirectory, "jetty.xml");
        Files.write(newContent, targetFile, UTF_8);
    }
}
