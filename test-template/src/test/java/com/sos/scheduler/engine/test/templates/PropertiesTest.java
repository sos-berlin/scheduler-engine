package com.sos.scheduler.engine.test.templates;

import static org.junit.Assert.fail;

import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * \file PropertiesTest.java
 * \brief a simple test getting some properties of a running scheduler instance
 *  
 * \class PropertiesTest
 * \brief a simple test getting some properties of a running scheduler instance
 * 
 * \details
 *
 * \code
  \endcode
 *
 * \author ss
 * \version 1.0 - 08.09.2011 11:21:33
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
public class PropertiesTest extends SchedulerTest {
	
    private static Logger logger = LoggerFactory.getLogger(PropertiesTest.class);

    @Test public void test1() throws Exception {
        try {
        	logger.info("some properties of the running scheduler ...");
            controller().runScheduler(shortTimeout);
            logger.debug("port=" + scheduler().getTcpPort());
            logger.debug("host=" + scheduler().getHostname());
            logger.debug("host_complete=" + scheduler().getHostnameLong());
//            fail("Exception expected");
        }
        catch (Exception x) {
            logger.debug(x.getMessage());
        }
    }
}
