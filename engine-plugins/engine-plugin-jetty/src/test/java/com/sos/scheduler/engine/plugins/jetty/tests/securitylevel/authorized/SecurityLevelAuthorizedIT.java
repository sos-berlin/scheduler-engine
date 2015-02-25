package com.sos.scheduler.engine.plugins.jetty.tests.securitylevel.authorized;

import com.google.common.io.Files;
import com.sos.scheduler.engine.common.utils.JavaResource;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.configuration.TestConfigurationBuilder;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import com.sun.jersey.api.client.Client;
import com.sun.jersey.api.client.ClientResponse;
import com.sun.jersey.api.client.WebResource;
import com.sun.jersey.api.client.filter.HTTPBasicAuthFilter;
import java.io.File;
import java.io.IOException;
import java.net.URI;
import org.junit.Test;

import static com.google.common.base.Charsets.UTF_8;
import static com.sos.scheduler.engine.plugins.jetty.test.JettyPluginTests.contextUri;
import static com.sun.jersey.api.client.ClientResponse.Status.FORBIDDEN;
import static com.sun.jersey.api.client.ClientResponse.Status.NOT_FOUND;
import static com.sun.jersey.api.client.ClientResponse.Status.OK;
import static com.sun.jersey.api.client.ClientResponse.Status.UNAUTHORIZED;
import static org.hamcrest.CoreMatchers.equalTo;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.containsString;


public class SecurityLevelAuthorizedIT extends SchedulerTest {

    private static final String generalPassword = "testpassword";
    private static final JavaResource jettyXmlTemplateResource = JavaResource.apply("com/sos/scheduler/engine/plugins/jetty/tests/securitylevel/authorized/jetty-template.xml");
    private static final JavaResource realmProperties = JavaResource.apply("com/sos/scheduler/engine/plugins/jetty/tests/securitylevel/authorized/realm.properties");
    private static final String anonymousUser = "anonymous";
    private static final String xmlCommand = new CommandBuilder().startJobImmediately("a").getCommand();

    public SecurityLevelAuthorizedIT() {
        super(new TestConfigurationBuilder(SecurityLevelAuthorizedIT.class)
                .terminateOnError(false)  // SCHEDULER-121  The security settings do not allow this operation
                .build());
    }

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
        assertThat(response.getStatus(), equalTo(expectedStatus.getStatusCode()));
        assertThat(response.getEntity(String.class), containsString(expectedResult));
        webClient.destroy();
    }

    private void prepareEnvironment() throws Exception {
        File tempDir = controller().environment().configDirectory();
        File target = copyRealmFileToConfiguration(tempDir);
        writeJettyXml(jettyXmlTemplateResource, tempDir, target);
    }


    private static File copyRealmFileToConfiguration(File tempDir) throws IOException {
        File file = new File(tempDir, "realm.properties");
        Files.write(realmProperties.asUTF8String(), file, UTF_8);
        return file;
    }

    private static void writeJettyXml(JavaResource resource, File outputDirectory, File realmFile) throws IOException {
        File file = new File(outputDirectory, "jetty.xml");
        String content = resource.asUTF8String().replace("${realm.properties}", realmFile.getAbsolutePath());
        Files.write(content, file, UTF_8);
    }
}
