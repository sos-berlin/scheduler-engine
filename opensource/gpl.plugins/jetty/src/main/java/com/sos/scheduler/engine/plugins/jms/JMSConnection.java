package com.sos.scheduler.engine.plugins.jms;

import java.util.Iterator;
import java.util.List;

import javax.jms.*;
import org.apache.log4j.Logger;

import com.sos.scheduler.engine.test.SchedulerTest;
//import com.sos.scheduler.engine.test.util.CommandBuilder;


public class JMSConnection extends SchedulerTest {

    private static final Logger logger = Logger.getLogger(JMSConnection.class);
//    private static final CommandBuilder util = CommandBuilder.getInstance();

	/** Maven: mvn test -Dtest=JmsPlugInTest -DargLine=-Djms.providerUrl=tcp://localhost:61616 */
	
	/* start this module with -Djms.providerUrl=tcp://localhost:61616 to test with an external JMS server */
//    private static final String providerUrl = System.getProperty("jms.providerUrl", ActiveMQConfiguration.vmProviderUrl);
    private final ActiveMQConfiguration conf;

    private static final boolean noLocal = false;
    
    private final String providerUrl;
    private final Topic topic;
    private final TopicConnection topicConnection;
    private final TopicSession topicSession;
    private final TopicSubscriber topicSubscriber;
    
    public JMSConnection(String providerUrl, List<String> eventFilter) throws Exception {
    	super();
    	this.providerUrl = providerUrl;
    	this.conf = ActiveMQConfiguration.newInstance(providerUrl);
    	this.topic = conf.topic;
    	this.topicConnection = conf.topicConnectionFactory.createTopicConnection();
    	this.topicSession = topicConnection.createTopicSession(false, Session.CLIENT_ACKNOWLEDGE);

//    	String messageSelector = "eventName = '" + eventFilter + "'";
    	String eventString = "";
    	Iterator<String> it = eventFilter.iterator();
    	while (it.hasNext()) {
    		String eventName = it.next();
    		eventString += "'" + eventName + "',";
    	}
    	String messageSelector = "eventName IN (" + eventString.substring(0,eventString.length()-1) + ")";
    	logger.info("createSubscriber with filter: " + messageSelector);
    	this.topicSubscriber = topicSession.createSubscriber(topic, messageSelector, noLocal);
    }
    
    public JMSConnection(String providerUrl) throws Exception {
    	super();
    	this.providerUrl = providerUrl;
    	this.conf = ActiveMQConfiguration.newInstance(providerUrl);
    	this.topic = conf.topic;
    	this.topicConnection = conf.topicConnectionFactory.createTopicConnection();
    	this.topicSession = topicConnection.createTopicSession(false, Session.CLIENT_ACKNOWLEDGE);
    	logger.info("createSubscriber to get all events");
    	this.topicSubscriber = topicSession.createSubscriber(topic, null, noLocal);
    }
    
    protected void close() throws JMSException {
		topicSubscriber.close();
    }
    
    protected void setMessageListener(MessageListener listenerClass) throws Exception {
        this.topicSubscriber.setMessageListener(listenerClass);
        this.topicConnection.start();
    }
   
    protected void showMessageHeader(Message m) throws JMSException {
    	logger.debug("getJMSCorrelationID=" + m.getJMSCorrelationID() );
    	logger.debug("getJMSDeliveryMode (persistent/non persistent)=" + m.getJMSDeliveryMode() );
    	logger.debug("getJMSExpiration=" + m.getJMSExpiration() );
    	logger.debug("getJMSMessageID=" + m.getJMSMessageID() );
    	logger.debug("getJMSPriority=" + m.getJMSPriority() );
    	logger.debug("getJMSTimestamp=" + m.getJMSTimestamp() );
    	logger.debug("getJMSDestination=" + m.getJMSType() );
    	logger.debug("getJMSRedelivered=" + m.getJMSRedelivered() );
    	logger.debug("getJMSDestination (Topicname)=" + getTopicname(m) );
    	if (m.getJMSReplyTo()!=null) logger.debug("getJMSReplyTo=" + m.getJMSReplyTo().toString() );
    }
    
    protected String getTopicname(Message m) throws JMSException {
    	Topic t = (Topic)m.getJMSDestination();
    	return (t.getTopicName()!=null) ? t.getTopicName() : "???";
    }

    protected String getProviderUrl() {
    	return providerUrl;
    }
}
