package com.sos.scheduler.engine.tests.excluded.ss.json;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.module.scala.DefaultScalaModule$;
import com.sos.scheduler.engine.data.event.Event;
import com.sos.scheduler.engine.data.folder.JobPath;
import com.sos.scheduler.engine.data.job.TaskEndedEvent;
import com.sos.scheduler.engine.data.job.TaskStartedEvent;
import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.data.order.OrderKey;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.job.UnmodifiableTask;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.ByteArrayOutputStream;
import java.io.IOException;

import static org.junit.Assert.assertEquals;

public class SimpleTest extends SchedulerTest {
    private static final JobPath jobPath = JobPath.of("/job1");
	private static final Logger logger = LoggerFactory.getLogger(SimpleTest.class);
    private static final OrderKey orderKey = OrderKey.of("jobchain1", "jobchain1");
	
	private final CommandBuilder util = new CommandBuilder();

    // This object is needed for serializing and deserializing of the event objects
    private final ObjectMapper mapper = newObjectMapper();

    private static ObjectMapper newObjectMapper() {
        ObjectMapper result = new ObjectMapper();
        result.registerModule(DefaultScalaModule$.MODULE$);
        return result;
    }

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
        assertEquals("{\"TYPE\":\"TaskStartedEvent\",\"taskId\":1,\"jobPath\":\"/job1\"}", outputStream.toString());

        // deserialize the event from JSON into the TaskStartEvent object
        TaskStartedEvent ev = mapper.readValue(outputStream.toString(), TaskStartedEvent.class);
        assertEquals(jobPath, ev.jobPath());

        // deserialize the event from JSON into a JsonNode
        JsonNode n = mapper.readTree(outputStream.toString());
        assertEquals(TaskStartedEvent.class.getSimpleName(),n.get("TYPE").asText());
        assertEquals("1",n.get("taskId").asText());
        assertEquals(jobPath.asString(), n.get("jobPath").asText());
	}
    
	/** Das Objekt t.getOrder() ist hier null. */
	@HotEventHandler
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
        String expected = "{\"TYPE\":\"OrderFinishedEvent\",\"orderKey\":{\"jobChainPath\":\"/jobchain1\",\"id\":\"jobchain1\"}}";
        assertEquals(expected,outputStream.toString());

        // deserialize the event from JSON into the TaskStartEvent object
        OrderFinishedEvent ev = mapper.readValue(outputStream.toString(), OrderFinishedEvent.class);
        assertEquals(new OrderFinishedEvent(orderKey), ev);

        // deserialize the event from JSON into a JsonNode
        //TODO keine anderen Felder als TYPE werden geliefert. Warum?
        JsonNode n = mapper.readTree(outputStream.toString());
        logger.debug("UNMARSHALL: " + n.get("TYPE").asText());

		if (order.getId().asString().equals("jobchain1"))
			controller().terminateScheduler();
	}
}
