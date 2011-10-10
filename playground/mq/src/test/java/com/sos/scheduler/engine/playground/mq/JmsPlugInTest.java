package com.sos.scheduler.engine.playground.mq;

import com.sos.scheduler.engine.kernel.test.SuperSchedulerTest;
import com.sos.scheduler.engine.kernel.util.Time;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;
import javax.jms.Session;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.TextMessage;
import javax.jms.Topic;
import javax.jms.TopicConnection;
import javax.jms.TopicSession;
import javax.jms.TopicSubscriber;
import org.junit.Test;
import org.apache.log4j.*;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;


public class JmsPlugInTest extends SuperSchedulerTest {
    /** Maven: mvn test -Dtest=JmsPlugInTest -DargLine=-Djms.providerUrl=tcp://localhost:61616 */
    private static final String providerUrl = System.getProperty("jms.providerUrl", Configuration.vmProviderUrl);

    private static final Time schedulerTimeout = Time.of(5);
    private static final Configuration conf = Configuration.newInstance(providerUrl);
    private static final Logger logger = Logger.getLogger(JmsPlugInTest.class);

    private final Topic topic = conf.topic;
    private final TopicConnection topicConnection = conf.topicConnectionFactory.createTopicConnection();
    private final TopicSession topicSession = topicConnection.createTopicSession(false, Session.AUTO_ACKNOWLEDGE);
    private final TopicSubscriber topicSubscriber = newTopicSubscriber();
    private final BlockingQueue<Boolean> resultQueue = new ArrayBlockingQueue<Boolean>(1);

    
    public JmsPlugInTest() throws Exception {
        topicSubscriber.setMessageListener(new MyListener());
        topicConnection.start();
    }


    private TopicSubscriber newTopicSubscriber() throws JMSException {
        String messageSelector = null;
        boolean noLocal = false;
        return topicSession.createSubscriber(topic, messageSelector, noLocal);
    }

    
    @Test public void test() throws Exception {
        runScheduler(schedulerTimeout, "-e");
        assertThat(resultQueue.poll(0, TimeUnit.SECONDS), equalTo(true));
    }


    private class MyListener implements javax.jms.MessageListener {
        @Override public void onMessage(Message message) {
            // Läuft in einem Thread von JMS
            boolean result = false;
            try {
                TextMessage textMessage = (TextMessage) message;
                logger.debug("onMessage: " + textMessage.getText());
                assertThat(textMessage.getText(), startsWith("com.sos.scheduler.engine."));  // Erstmal ist der Klassenname vorangestellt.
                result = true;
            }
            catch (JMSException x) { throw new RuntimeException(x); }
            finally {
                resultQueue.offer(result);
            }
        }
    }
}
