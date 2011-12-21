package com.sos.scheduler.engine.plugins.event;

import java.io.Closeable;
import java.io.File;
import javax.jms.DeliveryMode;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.ObjectMessage;
import javax.jms.Session;
import javax.jms.TextMessage;
import javax.jms.Topic;
import javax.jms.TopicConnection;
import javax.jms.TopicPublisher;
import javax.jms.TopicSession;

public class Connector implements Closeable {
	
	//TODO
	// if the transport protocol "vm" is using allows clients to connect to each other inside the java VM.
	// The first client to use the VM connection will boot an embedded broker. Currently the database
	// of the broker will be installed in a subfolder called activemq-data. This behaviour should be
	// configured. 
	
	//TODO
	// Wird das plugin in unitTests verwendet, sollte dafür gesorgt werden, dass die Datenbank am Ende des Tests verschwindet.
	
	//TODO
	// Im Rahmen des unitTests sollte es möglich sein, den message broker als eigenen service zu starten.


	
//    private final BrokerService brokerService = new BrokerService();
    // BrokerService auskommentiert, weil der BrokerService am Ende nicht ordentlich abgeräumt wird, dann liegt noch eine MBean herum:
    // "javax.management.InstanceAlreadyExistsException: org.apache.activemq:BrokerName=com.sos.scheduler,Type=Broker" in addMBean()
    // ActiveMQ hat einen automatisch startenden BrokerService unter dem Namen "localhost".
    // Der legt im Arbeitsverzeichnis seine Datenbank an, im Unterverzeichnis activemq-data.

    private final TopicConnection connection;
    private final Topic topic;
    private final TopicSession session;
    private final TopicPublisher publisher;
    
//	private static final Logger logger = LoggerFactory.getLogger(Connector.class);

    Connector(Configuration c) {
        try {
            //brokerService.setBrokerName(Configuration.brokerName);
            //brokerService.addConnector("tcp://localhost:63102");
            connection = c.topicConnectionFactory.createTopicConnection();
            topic = c.topic;
            session = newTopicSession();
            publisher = session.createPublisher(topic);
            publisher.setDeliveryMode(DeliveryMode.PERSISTENT);    // message gerantiert zustellen
            connection.start();
        } catch (Exception x) {
            if (x instanceof RuntimeException)  throw (RuntimeException)x;
            throw new RuntimeException(x);
        }
    }


    private TopicSession newTopicSession() throws JMSException {
        boolean transacted = false;
        return connection.createTopicSession(transacted, Session.CLIENT_ACKNOWLEDGE);
//        return connection.createTopicSession(transacted, Session.AUTO_ACKNOWLEDGE);
    }


    void setPersistenceDirectory(File dir) {
//        try {
//            brokerService.getPersistenceAdapter().setDirectory(dir);
//        } catch (IOException x) { throw new RuntimeException(x); }
    }

    
    public void start() {
//        try {
//            brokerService.start();
//        } catch (Exception x) { throw new RuntimeException(x); }
    }


    public void stop() {
        try {
            try {
                connection.close();
            } catch (JMSException x) { throw new RuntimeException(x); }
            finally {
//                brokerService.stop();
            }
        } catch (Exception x) {
            if (x instanceof RuntimeException)  throw (RuntimeException)x;
            throw new RuntimeException(x);
        }
    }

    
    @Override
    public void close() {
        stop();
//        brokerService.waitUntilStopped();
    }


    void waitUntilStopped() throws Exception {
//        brokerService.waitUntilStopped();
        //brokerService.getPersistenceAdapter().stop();
    }


    public TopicConnection getTopicConnection() {
        return connection;
    }


    public Topic getTopic() {
        return topic;
    }


    public TextMessage newTextMessage() {
        try {
            return session.createTextMessage();
        } catch (JMSException x) { throw new RuntimeException(x); }
    }

    ObjectMessage newObjectMessage() {
        try {
            return session.createObjectMessage();
        } catch (JMSException x) { throw new RuntimeException(x); }
    }


    public void publish(Message message) {
        try {
            publisher.publish(message);
        } catch (JMSException x) { throw new RuntimeException(x); }
    }


    public static Connector newInstance(String providerUrl, String persistenceDirectory) {

//    	try {
//    		if (providerUrl.startsWith("vm:"))
//    			startActiveMqBroker(providerUrl, persistenceDirectory);
//    		
//		} catch (Exception e) {
//			logger.error("error starting ActiveMq broker: " + e.getMessage() );
//			e.printStackTrace();
//		}
		
        return new Connector(Configuration.newInstance(providerUrl));
    }

//    private static void startActiveMqBroker(String providerUrl, String persistenceDirectory) throws Exception {
//    	
//		logger.error("persistence directory for ActiveMQ broker is " + persistenceDirectory );
//        BrokerService broker = new BrokerService();
//        broker.setUseJmx(true);
//        broker.setPersistent(true);
//        
//        KahaPersistenceAdapter persistenceAdapter = new KahaPersistenceAdapter();
//        persistenceAdapter.setDirectory(new File(persistenceDirectory + "/kaha"));
//        broker.setDataDirectoryFile(new File(persistenceDirectory + "/data"));
//        broker.setTmpDataDirectory(new File(persistenceDirectory + "/temp"));
//        persistenceAdapter.setMaxDataFileLength(500L*1024*1024);
//       
//        try {
//            broker.setPersistenceAdapter(persistenceAdapter);
//        } catch (IOException e) {
//            throw new RuntimeException(e);
//        } 
//        broker.addConnector(providerUrl);
//        broker.start();
//    }

}
