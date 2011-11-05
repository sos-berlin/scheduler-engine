package com.sos.scheduler.engine.plugins.event;

import java.util.Properties;
import javax.jms.Topic;
import javax.jms.TopicConnectionFactory;
import javax.naming.Context;
import javax.naming.InitialContext;
import javax.naming.NamingException;

import org.apache.log4j.Logger;

public class Configuration {
    private static final Logger logger = Logger.getLogger(Configuration.class);
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

    private Configuration(TopicConnectionFactory cf, Topic t) {
        topicConnectionFactory = cf;
        topic = t;
    }

    public static Configuration newInstance(String providerUrl) {
    	try {
            InitialContext c = new InitialContext(jmsProperties(providerUrl));
            TopicConnectionFactory cf = (TopicConnectionFactory)c.lookup(topicConnectionFactoryName);
            Topic topic = (Topic)c.lookup(topicName);
            logger.debug("try to connect with provider " + providerUrl);
            return new Configuration(cf, topic);
        } catch (NamingException x) { throw new RuntimeException(x); }
    }

    private static Properties jmsProperties(String providerUrl) {
        Properties result = new Properties();
        result.put(Context.SECURITY_PRINCIPAL, "system");
        result.put(Context.SECURITY_CREDENTIALS, "manager");
        result.put(Context.INITIAL_CONTEXT_FACTORY, initialContextFactoryName);
        result.put(Context.PROVIDER_URL, providerUrl);
        result.put("connectionFactoryNames", topicConnectionFactoryName);
        result.put("topic." + topicName, topicName);
        return result;
    }
}
