/*
 * <div class="sos_branding">
 *   <p>(c) 2012 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

package com.sos.scheduler.engine.tests.excluded.ss.json;

import com.sos.scheduler.engine.data.event.Event;
import com.sos.scheduler.engine.data.folder.JobPath;
import com.sos.scheduler.engine.data.job.TaskEndedEvent;
import com.sos.scheduler.engine.data.job.TaskStartedEvent;
import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.job.UnmodifiableTask;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.apache.log4j.Logger;
import org.codehaus.jackson.JsonNode;
import org.codehaus.jackson.map.ObjectMapper;
import org.junit.BeforeClass;
import org.junit.Test;

import java.io.ByteArrayOutputStream;
import java.io.IOException;

import static org.junit.Assert.assertEquals;

public class SimpleTest extends SchedulerTest {
    private static final JobPath jobPath = JobPath.of("/job1");
	private static final Logger logger = Logger.getLogger(SimpleTest.class);
	
	private final CommandBuilder util = new CommandBuilder();

    // This object is needed for serializing and deserializing of the event objects
    private final ObjectMapper mapper = new ObjectMapper();

	@BeforeClass
    public static void setUpBeforeClass() throws Exception {
        logger.debug("starting test for " + SimpleTest.class.getName());
	}

	@Test
	public void test() throws InterruptedException {
        controller().activateScheduler();
        util.addOrder("jobchain1");
		controller().scheduler().executeXml( util.getCommand() );
        controller().waitForTermination(shortTimeout);
	}
	
	@HotEventHandler
	public void handleEvent(Event e) throws IOException {
        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        mapper.writeValue(outputStream, e);         // serialize the event into JSON
        logger.debug(outputStream.toString());
	}
	
	@HotEventHandler
	public void handleTaskStartedEvent(TaskStartedEvent e, UnmodifiableTask t) throws IOException {

        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();

        // serialize the event into JSON
        mapper.writeValue(outputStream, e);
        assertEquals("{\"TYPE\":\"TaskStartedEvent\",\"id\":\"1\",\"jobPath\":\"/job1\"}",outputStream.toString());

        // deserialize the event from JSON into the TaskStartEvent object
        TaskStartedEvent ev = mapper.readValue(outputStream.toString(), TaskStartedEvent.class);
        assertEquals(jobPath, ev.getJobPath());

        // deserialize the event from JSON into a JsonNode
        JsonNode n = mapper.readTree(outputStream.toString());
        assertEquals(TaskStartedEvent.class.getSimpleName(),n.get("TYPE").asText());
        assertEquals("1",n.get("id").asText());
        assertEquals(jobPath.asString(), n.get("jobPath").asText());
	}
    
	@HotEventHandler
	/**
	 * Das Objekt t.getOrder() ist hier null.
	 * 
	 * @param e
	 * @param t
	 * @throws java.io.IOException
	 */
	public void handleTaskEndedEvent(TaskEndedEvent e, UnmodifiableTask t) throws IOException {

        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();

        // serialize the event into JSON
        mapper.writeValue(outputStream, e);

        // deserialize the event from JSON into the TaskStartEvent object
        TaskEndedEvent ev = mapper.readValue(outputStream.toString(), TaskEndedEvent.class);
        logger.debug("UNMARSHALL: " + ev.getClass().getName());

        // deserialize the event from JSON into a JsonNode
        JsonNode n = mapper.readTree(outputStream.toString());
        logger.debug("UNMARSHALL: " + n.get("TYPE").asText());
	}
	
	@HotEventHandler
	public void handleOrderEnd(OrderFinishedEvent e, UnmodifiableOrder order) throws IOException, InterruptedException {

        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();

        // serialize the event into JSON
        mapper.writeValue(outputStream, e);
        String expected = "{\"TYPE\":\"OrderFinishedEvent\",\"key\":{\"jobChainPath\":\"/jobchain1\",\"id\":\"jobchain1\"}}";
        assertEquals(expected,outputStream.toString());

        // deserialize the event from JSON into the TaskStartEvent object
        OrderFinishedEvent ev = mapper.readValue(outputStream.toString(), OrderFinishedEvent.class);
        assertEquals("OrderFinishedEvent JobChain /jobchain1:jobchain1",ev.toString());

        // deserialize the event from JSON into a JsonNode
        //TODO keine anderen Felder als TYPE werden geliefert. Warum?
        JsonNode n = mapper.readTree(outputStream.toString());
        logger.debug("UNMARSHALL: " + n.get("TYPE").asText());

		if (order.getId().asString().equals("jobchain1"))
			controller().terminateScheduler();

	}

}
