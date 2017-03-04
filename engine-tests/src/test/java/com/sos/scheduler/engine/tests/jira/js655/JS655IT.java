package com.sos.scheduler.engine.tests.jira.js655;

import com.sos.jobscheduler.data.event.Event;
import com.sos.jobscheduler.data.event.KeyedEvent;
import com.sos.scheduler.engine.common.sync.Gate;
import com.sos.scheduler.engine.common.system.Files;
import com.sos.scheduler.engine.data.filebased.FileBasedActivated$;
import com.sos.scheduler.engine.data.filebased.FileBasedRemoved$;
import com.sos.scheduler.engine.data.jobchain.JobChainPath;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration;
import com.sos.scheduler.engine.data.scheduler.SchedulerTerminatedEvent;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sun.jersey.api.client.Client;
import com.sun.jersey.api.client.ClientResponse;
import com.sun.jersey.api.client.WebResource;
import java.io.File;
import java.net.URI;
import org.junit.Test;
import static com.sos.scheduler.engine.tests.jira.js655.JS655IT.M.jobChainActivated;
import static com.sos.scheduler.engine.tests.jira.js655.JS655IT.M.jobChainRemoved;
import static com.sos.scheduler.engine.tests.jira.js655.JS655IT.M.terminated;
import static com.sun.jersey.api.client.ClientResponse.Status.INTERNAL_SERVER_ERROR;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.equalTo;

/** JS-655  "JobScheduler does not start when a webservice entry is assigned to a job chain coming from hot folder" */
public final class JS655IT extends SchedulerTest {
    private static final JobChainPath initialJobChainPath = new JobChainPath("/myJobChain");
    private static final JobChainPath rightJobChainPath = new JobChainPath("/myLazyJobChain");

    private final WebResource webResource;
    enum M { jobChainActivated, jobChainRemoved, terminated }
    private final Gate<M> gate = new Gate<M>();

    public JS655IT() throws Exception {
        controller().activateScheduler();
        int port = instance(SchedulerConfiguration.class).tcpPort();
        URI uri = new URI("http://localhost:"+ port +"/myService");
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
        gate.expect(jobChainActivated, TestTimeout);
        assertThat(webResource.post(String.class, "Hello!"), equalTo("Bye!"));
    }

    private void testRemoveJobChain() throws Exception {
        renameJobChain(rightJobChainPath, initialJobChainPath);
        gate.expect(jobChainRemoved, TestTimeout);
        checkWebServiceIsNotReady();
    }

    private void renameJobChain(JobChainPath a, JobChainPath b) {
        Files.renameFile(jobChainFile(a), jobChainFile(b));
    }

    private File jobChainFile(JobChainPath p) {
        return controller().environment().fileFromPath(p).toFile();
    }

    private void checkWebServiceIsNotReady() {
        ClientResponse response = webResource.post(ClientResponse.class, "Hej!");
        assertThat(response.getStatus(), equalTo(INTERNAL_SERVER_ERROR.getStatusCode()));
    }

    @HotEventHandler public void handleEvent(KeyedEvent<Event> e) throws InterruptedException {
        if (e.event().equals(FileBasedActivated$.MODULE$)) {
            if (e.key().equals(rightJobChainPath)) {
                gate.put(jobChainActivated);
            }
        } else if (e.event().equals(FileBasedRemoved$.MODULE$)) {
            if (e.key().equals(rightJobChainPath)) {
                gate.put(jobChainRemoved);
            }
        } else if (SchedulerTerminatedEvent.class.isAssignableFrom(e.event().getClass())) {
            gate.put(terminated);
        }
    }
}
