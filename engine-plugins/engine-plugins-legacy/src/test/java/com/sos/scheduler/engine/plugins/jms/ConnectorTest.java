package com.sos.scheduler.engine.plugins.jms;

import static org.junit.Assert.assertTrue;

import javax.jms.TextMessage;
import javax.jms.Topic;
import javax.jms.TopicConnection;

import org.junit.BeforeClass;
import org.junit.Test;


public class ConnectorTest {

	private static final Connector connector = Connector.newInstance(ActiveMQConfiguration.vmProviderUrl, "c:/temp/amq");

	@BeforeClass
	public static void beforeClass() throws Exception {
		connector.start();
	}

	@Test
	public void testPublish() throws Exception {
		TextMessage m = connector.newTextMessage();
		m.setText(getClass().getName());
		connector.publish(m);
	}

	@Test
	public void testGetTopicConnection() throws Exception {
		TopicConnection result = connector.getTopicConnection();
		assertTrue("TopicConnection is null",result != null);
	}

	@Test
	public void testGetTopic() throws Exception {
		Topic result = connector.getTopic();
		assertTrue("Topic is null",result != null);
	}
}