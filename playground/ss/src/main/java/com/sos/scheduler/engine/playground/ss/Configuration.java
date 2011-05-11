package com.sos.scheduler.engine.playground.ss;

import java.util.Properties;
import javax.jms.*;
import javax.naming.Context;
import javax.naming.InitialContext;
import javax.naming.NamingException;


public class Configuration { //public nur für JMSPlugInTest, Klasse ist zu komplex für public
    public static final String brokerName = "localhost";//"com.sos.scheduler";
    public static final String initialContextFactoryName = "org.apache.activemq.jndi.ActiveMQInitialContextFactory";
    public static final String topicConnectionFactoryName = "TopicCF";
    public static final String topicName = "com.sos.scheduler.engine.Event";   // + hostName + portNumber in Url-Notation tcp://host:4444
    public static final String vmProviderUrl = "vm:" + brokerName; //tcp://localhost:61616";

    public final TopicConnectionFactory topicConnectionFactory;
    public final Topic topic;


    private Configuration(TopicConnectionFactory cf, Topic t) {
        topicConnectionFactory = cf;
        topic = t;
    }


//    public static Configuration newInstance() {
//        return newInstance(vmProviderUrl);
//    }

    
    public static Configuration newInstance(String providerUrl) {
        try {
            InitialContext c = new InitialContext(jmsProperties(providerUrl));  // Datei jndi.jmsProperties
            TopicConnectionFactory cf = (TopicConnectionFactory)c.lookup(topicConnectionFactoryName);
            Topic topic = (Topic)c.lookup(topicName);
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
