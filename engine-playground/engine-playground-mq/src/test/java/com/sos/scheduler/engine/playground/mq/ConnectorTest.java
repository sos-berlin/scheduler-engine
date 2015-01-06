package com.sos.scheduler.engine.playground.mq;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.notNullValue;

import javax.jms.TextMessage;
import javax.jms.Topic;
import javax.jms.TopicConnection;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;


public class ConnectorTest {
    //private static final File tmpDir = Files.createTempDir();
    private static final Connector connector = Connector.newInstance(Configuration.vmProviderUrl);


    @BeforeClass public static void beforeClass() throws Exception {
        //connector.setPersistenceDirectory(tmpDir);
        connector.start();
    }


    @AfterClass public static void afterClass() throws Exception {
        try {
            connector.close();
        } finally {
            //TODO Nicht löschbar: Files.deleteRecursively(tmpDir);
        }
    }


    @Test public void testPublish() throws Exception {
        TextMessage m = connector.newTextMessage();
        m.setText(getClass().getName());
        connector.publish(m);
    }


    @Test public void testGetTopicConnection() {
        TopicConnection result = connector.getTopicConnection();
        assertThat(result, notNullValue());
    }


    @Test public void testGetTopic() {
        Topic result = connector.getTopic();
        assertThat(result, notNullValue());
    }
}