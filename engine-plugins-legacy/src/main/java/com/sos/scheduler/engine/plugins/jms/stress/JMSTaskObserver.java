package com.sos.scheduler.engine.plugins.jms.stress;

import static java.util.Arrays.asList;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageListener;
import javax.jms.TextMessage;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.sos.scheduler.engine.data.EventList;
import com.sos.scheduler.engine.data.configuration.EngineJacksonConfiguration;
import com.sos.scheduler.engine.data.event.Event;
import com.sos.scheduler.engine.data.job.TaskEndedEvent;
import com.sos.scheduler.engine.data.job.TaskStartedEvent;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.plugins.jms.JMSConnection;

public class JMSTaskObserver extends JMSConnection implements MessageListener, TaskInfo {

	private static final Logger logger = LoggerFactory.getLogger(JMSTaskObserver.class);

    private static final List<String> eventsToListen = asList("TaskStartedEvent", "TaskEndedEvent");
    private final List<TaskInfoListener> listener = new ArrayList<TaskInfoListener>();

	private int endedTasks = 0;
	private int runningTasks = 0;
	private int maxParallelTasks = 0;

    // This object is needed for serializing and deserializing of the event objects
    private final ObjectMapper mapper;

	public JMSTaskObserver(String providerUrl) throws Exception {
		super(providerUrl, eventsToListen);
		setMessageListener(this);

        mapper = EngineJacksonConfiguration.newObjectMapper();
        mapper.registerSubtypes(EventList.eventClassArray());

		listener.clear();
	}

	public void addListener(TaskInfoListener addListener) {
		listener.add(addListener);
	}

	// runs in an own thread
	@Override
	public void onMessage(Message message) {
        String jsonContent = null;
		try {
			TextMessage textMessage = (TextMessage) message;
			jsonContent = textMessage.getText();
            Event ev = mapper.readValue(jsonContent, Event.class);
            if (ev instanceof TaskStartedEvent) {
                TaskStartedEvent te = (TaskStartedEvent)ev;
                runningTasks++;
                if (runningTasks > maxParallelTasks)
                    maxParallelTasks = runningTasks;
                logger.debug("TASKSTART: " + te.taskId());
            }
            if (ev instanceof TaskEndedEvent) {
                TaskEndedEvent te = (TaskEndedEvent)ev;
                logger.debug("TASKEND: " + te.taskId());
                runningTasks--;
                endedTasks++;
            }
			textMessage.acknowledge();
        } catch (IOException e) {
			throw new SchedulerException("could not desesrialize " + jsonContent, e);
		} catch (JMSException e) {
            throw new SchedulerException("could not acknowledge message", e);
        }
    }

	@Override
	public int currentlyRunningTasks() {
		return runningTasks;
	}

	@Override
	public int endedTasks() {
		return endedTasks;
	}

	@Override
	public int highwaterTasks() {
		return maxParallelTasks;
	}

	@Override
	public void onInterval(TaskInfo info) {
		for (TaskInfoListener l : listener) 
			l.onInterval(info);
	}

}
