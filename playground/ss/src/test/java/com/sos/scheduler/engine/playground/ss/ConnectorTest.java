package com.sos.scheduler.engine.playground.ss;

import com.google.common.io.Files;
import com.sos.JSHelper.Logging.Log4JHelper;

import java.io.File;
import javax.jms.TopicConnection;
import javax.jms.Topic;
import javax.jms.TextMessage;

//import org.apache.log4j.Logger;
import org.junit.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;

public class ConnectorTest {
	
	
	private static final String tmpDir = Files.createTempDir().getAbsolutePath();
	
	/* start this module with -Djms.providerUrl=tcp://localhost:61616 to test with an external JMS server */
	private static Connector connector;
    
//	private final Logger logger = LoggerFactory.getLogger(Connector.class);
	private static Logger logger;
    
	@BeforeClass
	public static void beforeClass() throws Exception {
		
		// this file contains appender for ActiveMQ logging
		new Log4JHelper("src/test/resources/log4j.properties");
		
		logger = LoggerFactory.getLogger(Connector.class);
		connector = Connector.newInstance(Configuration.vmProviderUrl, "c:/temp/amq");
		connector.start();
	}

	@AfterClass
	public static void afterClass() throws Exception {
		try {
			connector.close();
		} finally {
			// TODO Nicht löschbar: Files.deleteRecursively(tmpDir);
		}
	}

	@Test
	public void testPublish() throws Exception {
		TextMessage m = connector.newTextMessage();
		m.setText(getClass().getName());
		connector.publish(m);
	}

	@Test
	public void testGetTopicConnection() {
		TopicConnection result = connector.getTopicConnection();
		assertThat(result, notNullValue());
	}

	@Test
	public void testGetTopic() {
		Topic result = connector.getTopic();
		assertThat(result, notNullValue());
	}
}