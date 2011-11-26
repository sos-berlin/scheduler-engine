package com.sos.scheduler.engine.tests.jira.js655;

import static com.google.common.base.Charsets.UTF_8;
import static com.sos.scheduler.engine.tests.jira.js655.JS655Test.M.jobChainActivated;
import static com.sos.scheduler.engine.tests.jira.js655.JS655Test.M.jobChainRemoved;
import static com.sos.scheduler.engine.tests.jira.js655.JS655Test.M.terminated;
import static java.lang.Thread.sleep;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.equalTo;

import java.io.File;
import java.io.IOException;
import java.net.URI;
import java.nio.charset.Charset;

import org.apache.log4j.Logger;
import org.eclipse.jetty.client.ContentExchange;
import org.eclipse.jetty.client.HttpClient;
import org.eclipse.jetty.client.HttpExchange;
import org.eclipse.jetty.io.ByteArrayBuffer;
import org.junit.Test;

import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.folder.AbsolutePath;
import com.sos.scheduler.engine.kernel.folder.events.FileBasedActivatedEvent;
import com.sos.scheduler.engine.kernel.folder.events.FileBasedRemovedEvent;
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.kernel.util.Files;
import com.sos.scheduler.engine.kernel.util.sync.Gate;
import com.sos.scheduler.engine.main.event.TerminatedEvent;
import com.sos.scheduler.engine.test.SchedulerTest;

/** JS-655  "JobScheduler does not start when a webservice entry is assigned to a job chain coming from hot folder" */
public class JS655Test extends SchedulerTest {
    private static final Logger logger = Logger.getLogger(JS655Test.class);
    private static final String initialJobChainName = "myJobChain";
    private static final String rightJobChainName = "myLazyJobChain";

    private final HttpClient httpClient = new HttpClient();
    private final URI uri;
    enum M { jobChainActivated, jobChainRemoved, terminated }
    private final Gate<M> gate = new Gate<M>();

    public JS655Test() throws Exception {
        controller().activateScheduler();
        uri = new URI("http://localhost:"+ scheduler().getTcpPort() +"/myService");
        httpClient.setConnectorType(HttpClient.CONNECTOR_SELECT_CHANNEL);
        httpClient.start();
    }

    @Test public void test() throws Exception {
        checkWebServiceIsNotReady();
        testAddJobChain();
        testRemoveJobChain();
        testAddJobChain();
    }

    private void testAddJobChain() throws Exception {
        renameJobChain(initialJobChainName, rightJobChainName);
        gate.expect(jobChainActivated, shortTimeout);
        assertThat(webClient().post("Hello!"), equalTo("Bye!"));
    }

    private void testRemoveJobChain() throws Exception {
        renameJobChain(rightJobChainName, initialJobChainName);
        gate.expect(jobChainRemoved, shortTimeout);
        checkWebServiceIsNotReady();
    }

    private void renameJobChain(String oldName, String newName) {
        Files.renameFile(jobChainFile(oldName), jobChainFile(newName));
    }

    private File jobChainFile(String name) {
        return controller().environment().fileFromPath(new AbsolutePath("/" + name), ".job_chain.xml");
    }

    private void checkWebServiceIsNotReady() throws Exception {
        WebClient webClient = webClient();
        webClient.tryPost("Hej!");
        assertThat(webClient.contentExchange.getResponseStatus(), equalTo(500));
    }

    private WebClient webClient() throws Exception {
        return new WebClient(uri.toString());
    }

    @HotEventHandler public void handleEvent(FileBasedActivatedEvent e) throws InterruptedException {
        if (e.getObject() instanceof JobChain) {
            JobChain jobChain = (JobChain)e.getObject();
            if (jobChain.getName().equals(rightJobChainName))
                gate.put(jobChainActivated);
        }
    }

    @HotEventHandler public void handleEvent(FileBasedRemovedEvent e) throws InterruptedException {
        if (e.getObject() instanceof JobChain) {
            JobChain jobChain = (JobChain)e.getObject();
            if (jobChain.getName().equals(rightJobChainName))
                gate.put(jobChainRemoved);
        }
    }

    @EventHandler public void handleEvent(TerminatedEvent e) throws InterruptedException {
        // Nur für Fehlerfall
        gate.put(terminated);
    }

    private class WebClient {
        private final Charset encoding = UTF_8;
        private final ContentExchange contentExchange = new ContentExchange();

        WebClient(String url) throws Exception {
            contentExchange.setURL(url);
            contentExchange.setRequestContentType("text/xml;charset=" + encoding.name());
        }

        private String post(String request) throws IOException, InterruptedException {
            String response = tryPost(request);
            assertThat(contentExchange.getResponseStatus(), equalTo(200));
            return response;
        }

        private String tryPost(String request) throws IOException, InterruptedException {
            contentExchange.setMethod("POST");
            contentExchange.setRequestContent(new ByteArrayBuffer(request, encoding.name()));
            httpClient.send(contentExchange);
            int sendStatus = contentExchange.waitForDone();
            assertThat(sendStatus, equalTo(HttpExchange.STATUS_COMPLETED));
            return contentExchange.getResponseContent();
        }
    }
}
