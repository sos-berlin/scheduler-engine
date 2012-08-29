package com.sos.scheduler.engine.tests.excluded.ss.js428;

import com.google.common.collect.ImmutableMap;
import com.sos.scheduler.engine.data.order.*;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.order.Order;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.engine.kernel.variable.UnmodifiableVariableSet;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.apache.log4j.Logger;
import org.junit.BeforeClass;
import org.junit.Ignore;
import org.junit.Test;

import java.io.IOException;

import static org.junit.Assert.assertTrue;

public class JS428Test extends SchedulerTest {

    private static final Logger logger = Logger.getLogger(JS428Test.class);
    private final CommandBuilder util = new CommandBuilder();
    private static final String DELIMITER = "/";

    private UnmodifiableVariableSet orderParams;
    private ImmutableMap<String,String> resultSet;

    @BeforeClass
    public static void setUpBeforeClass() throws Exception {
        logger.debug("starting test for " + JS428Test.class.getName());
    }

    @Ignore
    /**
     * Dieser Test funktioniert nicht, weil die StepStartedEvent für shell jobs nicht ausgelöst wird.
     * Hier sind noch Änderungen im C++ code vorzunehmen.
     *
     * Außerdem ist zu klären, wie die Ergebnisse aus dem Job im Test ausgewertet werden können.
     *
     * @throws InterruptedException
     */
    public void mixedTest() throws InterruptedException {
        controller().setTerminateOnError(false);
        String resultFile = getTempFile(JS428Test.class, "scheduler.log").getAbsolutePath();
        controller().activateScheduler("-e","-log-level=warn","-log=" + resultFile);
        util.addOrder("jobchain-mixed")
            .addParam("050/param1", "step 050")
            .addParam("100/param1", "step 100")
            .addParam("200/param1", "step 200")
            .addParam("300/param1", "step 300")
            .addParam("400/param1", "step 400")
            .addParam("param2", "all steps")
        ;
        controller().scheduler().executeXml( util.getCommand() );
        controller().waitForTermination(shortTimeout);
        testSpoolerVariable("job-shell-1_param1", "step 050");		// funktioniert nicht
        testSpoolerVariable("job-shell-1_param2", "all steps");		// funktioniert nicht
        testSpoolerVariable("job-api-1_param1", "step 100");
        testSpoolerVariable("job-api-1_param2", "all steps");
        testSpoolerVariable("job-api-2_param1", "step 200");
        testSpoolerVariable("job-api-2_param2", "all steps");
        testSpoolerVariable("job-api-3_param1", "step 300");
        testSpoolerVariable("job-api-3_param2", "all steps");
        testSpoolerVariable("job-shell-4_param1", "step 400");		// funktioniert nicht
        testSpoolerVariable("job-shell-4_param2", "all steps");		// funktioniert nicht
    }

    @Test
    public void apiTest() throws InterruptedException {
        controller().setTerminateOnError(false);
        String resultFile = getTempFile(JS428Test.class, "scheduler.log").getAbsolutePath();
        controller().activateScheduler("-e","-log-level=warn","-log=" + resultFile);
        util.addOrder("jobchain-api")
            .addParam("100/param1", "step 100")
            .addParam("200/param1", "step 200")
            .addParam("300/param1", "step 300")
            .addParam("param2", "all steps")
        ;
        controller().scheduler().executeXml( util.getCommand() );
        controller().waitForTermination(shortTimeout);
        testSpoolerVariable("job-api-1_param1", "step 100");
        testSpoolerVariable("job-api-1_param2", "all steps");
        testSpoolerVariable("job-api-2_param1", "step 200");
        testSpoolerVariable("job-api-2_param2", "all steps");
        testSpoolerVariable("job-api-3_param1", "step 300");
        testSpoolerVariable("job-api-3_param2", "all steps");
    }

    @HotEventHandler
    public void handleOrderTouched(OrderTouchedEvent e, Order order) throws IOException {
        orderParams = order.getParameters();
    }

    @HotEventHandler
    public void handleStepStart(OrderStepStartedEvent e, Order order) throws IOException, InterruptedException {
        logger.debug("STEP_START=" + order.getState().asString());
        setStepParameter(order, order.getState());
        showOrderParameter(order, order.getState());
    }

    @HotEventHandler
    public void handleStepEnd(OrderStepEndedEvent e, Order order) throws IOException, InterruptedException {
        logger.debug("STEP_END=" + order.getState().asString());
    }

    @HotEventHandler
    public void handleOrderEnd(OrderFinishedEvent e, UnmodifiableOrder order) throws IOException, InterruptedException {
        logger.debug("ORDERFINISHED: " + order.getId().asString());
        resultSet = scheduler().getVariables().toMap();
        controller().terminateScheduler();
    }

    private void testSpoolerVariable(String varname, String expected) {
        String value = resultSet.get(varname);
        assertTrue(varname + " is not set in scheduler variables", value != null);
        assertTrue(value + " is not valid - " + expected + " expected", value.equals(expected));
    }

    private void setStepParameter(Order order, OrderState setForState) {
        String step = setForState.asString();
        String stepId = step + DELIMITER;
        for (String orderParam : orderParams.getNames()) {
            String orderParamValue = orderParams.get(orderParam);
            if (orderParam.startsWith(stepId))
                order.getParameters().put(orderParam.replace(stepId, ""), orderParamValue);
        }
    }

    private void showOrderParameter(Order order, OrderState state) {
        logger.debug("================== order parameter at state " + state);
        for (String orderParam : orderParams.getNames()) {
            String orderParamValue = orderParams.get(orderParam);
            logger.debug(orderParam + "=" + orderParamValue);
        }
        logger.debug("==================");
    }

}
