/*
 * <div class="sos_branding">
 *   <p>(c) 2012 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

/*
 * <div class="sos_branding">
 *   <p>(c) 2012 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

/*
 * <div class="sos_branding">
 *   <p>(c) 2012 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

package com.sos.scheduler.engine.tests.excluded.ss.schedulerparameter;

import com.sos.JSHelper.Exceptions.JobSchedulerException;
import org.apache.log4j.Logger;
import sos.scheduler.job.JobSchedulerJobAdapter;

import java.util.HashMap;

/**
 * Created by IntelliJ IDEA.
 * User: ss
 * Date: 07.02.12
 * Time: 16:27
 */
public class ReadParameterJob extends JobSchedulerJobAdapter {

    private static final String expected_1 = "scheduler.xml";
    private static final String expected_2 = "job.xml";

    @SuppressWarnings("unused")
    private static Logger logger			= Logger.getLogger(ReadParameterJob.class);

    @SuppressWarnings("unused")
    private	final String		conSVNVersion			= "$Id: ReadParameterJob.java 16850 2012-03-21 12:14:23Z ss $";

    @Override
    public boolean spooler_process() throws Exception {

        try {
            super.spooler_process();
            
            HashMap<String,String> params = getAllParametersAsProperties();
            String value_1 = params.get("parameter_1");
            String value_2 = params.get("parameter_2");
            logger.info("parameter_1: " + value_1);
            logger.info("parameter_2: " + value_2);
            if (!value_1.equals(expected_1))
                throw new JobSchedulerException("parameter 'parameter_1' does not conform the expected value '" + expected_1 + "'. Current value is '" + value_1 + "'") ;
            if (!value_2.equals(expected_2))
                throw new JobSchedulerException("parameter 'parameter_2' does not conform the expected value '" + expected_2 + "'. Current value is '" + value_2 + "'") ;
        }
        catch (Exception e) {
            throw new JobSchedulerException("error in job",e);
        }
        return false;

    } // spooler_process

}
