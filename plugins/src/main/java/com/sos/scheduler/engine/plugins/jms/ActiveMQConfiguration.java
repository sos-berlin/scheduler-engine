package com.sos.scheduler.engine.plugins.jms;

import java.util.Properties;
import javax.jms.Topic;
import javax.jms.TopicConnectionFactory;
import javax.naming.Context;
import javax.naming.InitialContext;
import javax.naming.NamingException;

import org.apache.log4j.Logger;

public class ActiveMQConfiguration {
    private static final Logger logger = Logger.getLogger(ActiveMQConfiguration.class);
    public static final String brokerName = "localhost"; //"com.sos.scheduler";
    public static final String initialContextFactoryName = "org.apache.activemq.jndi.ActiveMQInitialContextFactory";
    public static final String topicConnectionFactoryName = "TopicCF";
    public static final String topicName = "com.sos.scheduler.engine.Event";   // + hostName + portNumber in Url-Notation tcp://host:4444
    private static final String nonPersistentVmProviderUrl = "vm://"+brokerName+"?broker.persistent=false";  // Damit wird keine Datenbankdatei im Arbeitsverzeichnis angelegt
    //private static final String persistentVmProviderUrl = "vm://"+brokerName; //tcp://localhost:61616";
    public static final String vmProviderUrl = nonPersistentVmProviderUrl;
    public static final String persistenceDirectory = "active-mq-data";

    public final TopicConnectionFactory topicConnectionFactory;
    public final Topic topic;

    private ActiveMQConfiguration(TopicConnectionFactory cf, Topic t) {
        topicConnectionFactory = cf;
        topic = t;
    }

    public static ActiveMQConfiguration newInstance(String providerUrl) {
    	try {
            InitialContext c = new InitialContext(jmsProperties(providerUrl));
            TopicConnectionFactory cf = (TopicConnectionFactory)c.lookup(topicConnectionFactoryName);
            Topic topic = (Topic)c.lookup(topicName);
            logger.debug("try to connect with provider " + providerUrl);
            return new ActiveMQConfiguration(cf, topic);
        } catch (NamingException x) { throw new RuntimeException(x); }
    }

    private static Properties jmsProperties(String providerUrl) {
        Properties result = new Properties();
        result.setProperty(Context.SECURITY_PRINCIPAL, "system");
        result.setProperty(Context.SECURITY_CREDENTIALS, "manager");
        result.setProperty(Context.INITIAL_CONTEXT_FACTORY, initialContextFactoryName);
        result.setProperty(Context.PROVIDER_URL, providerUrl);
        result.setProperty("connectionFactoryNames", topicConnectionFactoryName);
        result.setProperty("topic." + topicName, topicName);
        return result;
    }
}
