package com.sos.scheduler.engine.plugins.jms.stress;

import static java.util.Arrays.asList;

import java.io.File;
import java.util.List;
import java.util.Timer;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;

import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.TextMessage;

import org.apache.log4j.Logger;

import com.sos.scheduler.engine.plugins.jms.JMSConnection;
import com.sos.scheduler.engine.test.util.JSFileUtils;
import com.sos.scheduler.model.SchedulerObjectFactory;
import com.sos.scheduler.model.events.Event;

public class TaskObserverPlugin extends JMSConnection implements javax.jms.MessageListener, TaskInfo {

	private static final Logger logger = Logger.getLogger(TaskObserverPlugin.class);

	private static SchedulerObjectFactory objFactory;
	private static TaskObserverPlugin instance = null;
	private static final String resultfileName = "stresstest_result.csv";
    private static final List<String> eventsToListen = asList("EventTaskStarted","EventTaskEnded");

	private static int estimatedTasks = 0;
	private int endedTasks = 0;
	private int runningTasks = 0;
	private int maxParallelTasks = 0;

	private static TaskObserver loggerTask = null;
	private static Timer timer;

	private TaskObserverPlugin(String providerUrl, int estimated) throws Exception {
		super(providerUrl, eventsToListen);
		setMessageListener(this);
		estimatedTasks = estimated;

		// TODO connect with the scheduler configuration fired the messages
		objFactory = new SchedulerObjectFactory("localhost", 4444);
		objFactory.initMarshaller(com.sos.scheduler.model.events.Event.class);

		File logfile = JSFileUtils.getLocalFile(getClass(), resultfileName);
		timer = new Timer();
		loggerTask = new TaskObserver(logfile.getAbsolutePath(), this);

	}

	public static TaskObserverPlugin getInstance(String providerUrl, int estimated) throws Exception {
		if (instance == null)
			instance = new TaskObserverPlugin(providerUrl, estimated);
		return instance;
	}

	public void start(long delay, long period) {
		timer.schedule(loggerTask, delay, period); // schedule the timer task
	}

	public void stop() {
		loggerTask.close();
		timer.cancel();
		timer.purge();
	}

	// runs in an own thread
	@Override
	public void onMessage(Message message) {
		try {
			TextMessage textMessage = (TextMessage) message;
			String xmlContent = textMessage.getText();
			Event ev = (Event) objFactory.unMarshall(xmlContent); // get the event object
			// logger.info("subscribe " + ev.getName());
			// logger.debug(xmlContent);
			if (ev.getEventTaskStarted() != null) {
				runningTasks++;
				if (runningTasks > maxParallelTasks)
					maxParallelTasks = runningTasks;
				logger.debug("TASKSTART: " + ev.getEventTaskStarted().getInfoTask().getId());
			}
			if (ev.getEventTaskEnded() != null) {
				logger.debug("TASKEND: " + ev.getEventTaskEnded().getInfoTask().getId());
				runningTasks--;
				endedTasks++;
			}
			textMessage.acknowledge();
		} catch (JMSException e) {
			throw new RuntimeException("error getting a message from " + getProviderUrl(),e);
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
	public int estimatedTasks() {
		return estimatedTasks;
	}

}
