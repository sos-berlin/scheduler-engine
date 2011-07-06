package com.sos.scheduler.engine.plugins.event;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;

import javax.jms.TextMessage;
import javax.jms.Topic;
import javax.jms.TopicConnection;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.io.Files;
import com.sos.JSHelper.Logging.Log4JHelper;
import com.sos.scheduler.engine.plugins.event.Configuration;
import com.sos.scheduler.engine.plugins.event.Connector;

public class ConnectorTest {
	
	
	@SuppressWarnings("unused")
	private static final String tmpDir = Files.createTempDir().getAbsolutePath();
	
	/* start this module with -Djms.providerUrl=tcp://localhost:61616 to test with an external JMS server */
	private static Connector connector;
    
//	private final Logger logger = LoggerFactory.getLogger(Connector.class);
	@SuppressWarnings("unused")
	private static Logger logger;
    
	@BeforeClass
	public static void beforeClass() throws Exception {
		
		// this file contains appender for ActiveMQ logging
		new Log4JHelper("src/test/resources/log4j.properties");
		
		logger = LoggerFactory.getLogger(Connector.class);
		connector = Connector.newInstance(Configuration.vmProviderUrl, "c:/temp/amq");
		connector.start();
	}

//	@AfterClass
//	public static void afterClass() throws Exception {
//		try {
//			connector.close();
//		} finally {
//			// TODO Nicht löschbar: Files.deleteRecursively(tmpDir);
//		}
//	}

	@Test
	public void testPublish() throws Exception {
		TextMessage m = connector.newTextMessage();
		m.setText(getClass().getName());
		connector.publish(m);
	}

	@Test
	public void testGetTopicConnection() throws Exception {
		TopicConnection result = connector.getTopicConnection();
		assertThat(result, notNullValue());
	}

	@Test
	public void testGetTopic() throws Exception {
		Topic result = connector.getTopic();
		assertThat(result, notNullValue());
	}
}