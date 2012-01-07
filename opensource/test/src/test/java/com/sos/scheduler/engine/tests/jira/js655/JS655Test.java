package com.sos.scheduler.engine.tests.jira.js655;

import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.folder.AbsolutePath;
import com.sos.scheduler.engine.kernel.folder.Path;
import com.sos.scheduler.engine.kernel.folder.events.FileBasedActivatedEvent;
import com.sos.scheduler.engine.kernel.folder.events.FileBasedRemovedEvent;
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.kernel.util.Files;
import com.sos.scheduler.engine.kernel.util.sync.Gate;
import com.sos.scheduler.engine.main.event.TerminatedEvent;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sun.jersey.api.client.Client;
import com.sun.jersey.api.client.ClientResponse;
import com.sun.jersey.api.client.WebResource;
import org.junit.Test;

import java.io.File;
import java.net.URI;

import static com.sos.scheduler.engine.kernel.util.Util.ignore;
import static com.sos.scheduler.engine.tests.jira.js655.JS655Test.M.*;
import static com.sun.jersey.api.client.ClientResponse.Status.INTERNAL_SERVER_ERROR;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.equalTo;

/** JS-655  "JobScheduler does not start when a webservice entry is assigned to a job chain coming from hot folder" */
public class JS655Test extends SchedulerTest {
    private static final Path initialJobChainPath = new AbsolutePath("/myJobChain");
    private static final Path rightJobChainPath = new AbsolutePath("/myLazyJobChain");

    private final WebResource webResource;
    enum M { jobChainActivated, jobChainRemoved, terminated }
    private final Gate<M> gate = new Gate<M>();

    public JS655Test() throws Exception {
        controller().activateScheduler("-e");
        URI uri = new URI("http://localhost:"+ scheduler().getTcpPort() +"/myService");
        webResource = Client.create().resource(uri);
    }

    @Test public void test() throws Exception {
        checkWebServiceIsNotReady();
        testAddJobChain();
        testRemoveJobChain();
        testAddJobChain();
    }

    private void testAddJobChain() throws Exception {
        renameJobChain(initialJobChainPath, rightJobChainPath);
        gate.expect(jobChainActivated, shortTimeout);
        assertThat(webResource.post(String.class, "Hello!"), equalTo("Bye!"));
    }

    private void testRemoveJobChain() throws Exception {
        renameJobChain(rightJobChainPath, initialJobChainPath);
        gate.expect(jobChainRemoved, shortTimeout);
        checkWebServiceIsNotReady();
    }

    private void renameJobChain(Path a, Path b) {
        Files.renameFile(jobChainFile(a), jobChainFile(b));
    }

    private File jobChainFile(Path p) {
        return controller().environment().fileFromPath(p, ".job_chain.xml");
    }

    private void checkWebServiceIsNotReady() throws Exception {
        ClientResponse response = webResource.post(ClientResponse.class, "Hej!");
        assertThat(response.getClientResponseStatus(), equalTo(INTERNAL_SERVER_ERROR));
    }

    @HotEventHandler public void handleEvent(FileBasedActivatedEvent e, JobChain jobChain) throws InterruptedException {
        ignore(e);
        if (jobChain.getPath().equals(rightJobChainPath))
            gate.put(jobChainActivated);
    }

    @HotEventHandler public void handleEvent(FileBasedRemovedEvent e, JobChain jobChain) throws InterruptedException {
        ignore(e);
        if (jobChain.getPath().equals(rightJobChainPath))
            gate.put(jobChainRemoved);
    }

    @EventHandler public void handleEvent(TerminatedEvent e) throws InterruptedException {
        // Nur für Fehlerfall
        ignore(e);
        gate.put(terminated);
    }
}
