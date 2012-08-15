package com.sos.scheduler.engine.plugins.jms.stress;

import com.sos.scheduler.engine.data.EventList;
import com.sos.scheduler.engine.data.event.Event;
import com.sos.scheduler.engine.data.job.TaskEndedEvent;
import com.sos.scheduler.engine.data.job.TaskStartedEvent;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.plugins.jms.JMSConnection;
import org.apache.log4j.Logger;
import org.codehaus.jackson.map.ObjectMapper;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.TextMessage;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import static java.util.Arrays.asList;

public class JMSTaskObserver extends JMSConnection implements javax.jms.MessageListener, TaskInfo {

	private static final Logger logger = Logger.getLogger(JMSTaskObserver.class);

    private static final List<String> eventsToListen = asList("TaskStartedEvent","TaskEndedEvent");
    private final List<TaskInfoListener> listener = new ArrayList<TaskInfoListener>();

	private int endedTasks = 0;
	private int runningTasks = 0;
	private int maxParallelTasks = 0;

    // This object is needed for serializing and deserializing of the event objects
    private final ObjectMapper mapper;

	public JMSTaskObserver(String providerUrl) throws Exception {
		super(providerUrl, eventsToListen);
		setMessageListener(this);

        mapper = new ObjectMapper();
        mapper.registerSubtypes( EventList.eventClassList );

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
            Event ev = mapper.readValue(jsonContent, com.sos.scheduler.engine.data.event.Event.class);
            if (ev instanceof TaskStartedEvent) {
                TaskStartedEvent te = (TaskStartedEvent)ev;
                runningTasks++;
                if (runningTasks > maxParallelTasks)
                    maxParallelTasks = runningTasks;
                logger.debug("TASKSTART: " + te.getId());
            }
            if (ev instanceof TaskEndedEvent) {
                TaskEndedEvent te = (TaskEndedEvent)ev;
                logger.debug("TASKEND: " + te.getId());
                runningTasks--;
                endedTasks++;
            }
			textMessage.acknowledge();
        } catch (IOException e) {
			throw new SchedulerException("could not desesrialize " + jsonContent,e);
		} catch (JMSException e) {
            throw new SchedulerException("could not acknowledge message",e);
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
