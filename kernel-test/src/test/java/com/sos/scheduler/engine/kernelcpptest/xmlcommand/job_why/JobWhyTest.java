package com.sos.scheduler.engine.kernelcpptest.xmlcommand.job_why;

import static com.google.common.collect.Sets.difference;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.booleanXPath;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.elementXPath;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.loadXml;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.toXml;
import static com.sos.scheduler.engine.kernelcpptest.xmlcommand.job_why.Configuration.jobNames;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.equalTo;

import java.util.Map;

import org.apache.log4j.Logger;
import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;
import org.w3c.dom.Element;

import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import com.sos.scheduler.engine.test.TestSchedulerController;

public final class JobWhyTest {
    private static final Logger logger = Logger.getLogger(JobWhyTest.class);

    private static final TestSchedulerController controller = TestSchedulerController.of(JobWhyTest.class.getPackage());
    private static Map<String,Element> results = null;

    @BeforeClass public static void beforeClass() {
        controller.startScheduler();
        results = executeJobWhy();
    }

    @AfterClass public static void afterClass() {
        controller.close();
    }

    private static Map<String,Element> executeJobWhy() {
        ImmutableMap.Builder<String,Element> b = new ImmutableMap.Builder<String,Element>();
        for (String j: jobNames)  b.put(j, executeJobWhy(j));
        return b.build();
    }
    
    private static Element executeJobWhy(String jobPath) {
        String xml = controller.scheduler().executeXml("<job.why job='" + jobPath + "'/>");
        Element result = elementXPath(loadXml(xml), "/spooler/answer/job.why");
        logger.debug(jobPath + ": " + toXml(result));
        return result;
    }

    @Test public void testJobStopped() {
        checkXPathForJobsExactly("obstacle[@state='stopped']", "c");
    }

    @Test public void testJobMinTasks() {
        checkXPathForJobsExactly("start_reason.why[@min_tasks='1']/obstacle[@in_period='false']", "minTasks");
    }

    @Test public void testJobMaxTasks() {
        checkXPathForJobsExactly("obstacle[@max_tasks='0']", "maxTasks");
    }

    @Test public void testJobchain() {
        for (Map.Entry<String,Element> i: results.entrySet())
            elementXPath(i.getValue(), jobchainXPath(i.getKey()));
    }

    @Test public void testJobchainStopped() {
        checkXPathForAllJobs("job_chain_nodes.why/job_chain_node.why/job_chain.why/obstacle[@state='stopped']");
    }

    private static void checkXPathForAllJobs(String xPath) {
        checkXPathForJobsExactly(xPath, jobNames.toArray(new String[jobNames.size()]));
    }
    
    private static void checkXPathForJobsExactly(String xPath, String... selectedJobNames) {
        for (String jobName: selectedJobNames)
            elementXPath(results.get(jobName), xPath);
        for (String jobName: difference(ImmutableSet.copyOf(selectedJobNames), ImmutableSet.copyOf(jobNames)))
            assertThat(booleanXPath(results.get(jobName), "not(obstacle[@state='stopped'])"), equalTo(false));
    }

    private static String jobchainXPath(String jobName) {
        return jobchainNodeXPath(jobName) + "/job_chain.why[@path='/j']";
    }

    private static String jobchainNodeXPath(String jobName) {
        return "job_chain_nodes.why/job_chain_node.why[@order_state='state-" + jobName + "']";
    }
}
