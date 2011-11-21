package com.sos.scheduler.engine.tests.jira.js795;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.equalTo;
import static org.hamcrest.core.StringContains.containsString;

import java.io.IOException;

import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.apache.log4j.Logger;
import org.eclipse.jetty.client.ContentExchange;
import org.eclipse.jetty.client.HttpClient;
import org.eclipse.jetty.client.HttpExchange;
import org.eclipse.jetty.io.ByteArrayBuffer;
import org.eclipse.jetty.server.Request;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.handler.AbstractHandler;
import org.junit.Test;

import com.google.common.io.CharStreams;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.test.SchedulerTest;

public class JS795Test extends SchedulerTest {
    private static final Logger logger = Logger.getLogger(JS795Test.class);
    private static final int cppPortNumber = 44441;  // TODO Ports sind willkürlich gewählt und nicht sicher verfügbar
    private static final int jettyPortNumber = 44440;
    private static final String jettyTestString = "**test**";

    @Test public void testJetty() throws Exception {
        controller().startScheduler("-port=" + cppPortNumber, "-e", "-log-level=debug9");

        Server server = new Server(jettyPortNumber);    // Server meldet nicht, wenn die Portnummer belegt ist. //
        server.setHandler(new MyHandler(scheduler()));
        server.start();

        String responseXml = executeXml("<show_state/>");
        assertThat(responseXml, containsString("<state"));
        assertThat(responseXml, containsString(jettyTestString));

        server.stop();
        server.join();
        controller().terminateScheduler();
    }

    private String executeXml(String commandXml) throws Exception {
        HttpClient httpClient = new HttpClient();
        httpClient.setConnectorType(HttpClient.CONNECTOR_SELECT_CHANNEL);
        httpClient.start();
        ContentExchange c = new ContentExchange();
        c.setMethod("POST");
        c.setURL("http://localhost:" + jettyPortNumber + "/");
        c.setRequestContentType("text/xml;charset=utf-8");
        c.setRequestContent(new ByteArrayBuffer(commandXml));
        httpClient.send(c);
        int sendStatus = c.waitForDone();
        assertThat(sendStatus, equalTo(HttpExchange.STATUS_COMPLETED));
        assertThat(c.getResponseStatus(), equalTo(200));
        return c.getResponseContent();
    }

    private static class MyHandler extends AbstractHandler {
        private final Scheduler scheduler;

        MyHandler(Scheduler scheduler) {
            this.scheduler = scheduler;
        }

        public void handle(String target, Request baseRequest, HttpServletRequest request, HttpServletResponse response)
                throws IOException, ServletException {
            String commandXml = CharStreams.toString(request.getReader());
            String responseXml = scheduler.executeXml(commandXml);
            response.setContentType("text/xml;charset=utf-8");
            response.setStatus(HttpServletResponse.SC_OK);
            response.getWriter().append(responseXml).append(jettyTestString);
            baseRequest.setHandled(true);
        }
    }
}
