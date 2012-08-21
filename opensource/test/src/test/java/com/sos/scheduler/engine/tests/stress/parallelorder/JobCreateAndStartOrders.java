package com.sos.scheduler.engine.tests.stress.parallelorder;

import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import sos.spooler.Job_chain;
import sos.spooler.Job_impl;
import sos.spooler.Order;

public class JobCreateAndStartOrders extends Job_impl {

    private static final String blank = "";
    private static final String parameterNameRuntime = "RUNTIME_IN_SECONDS";
    private static final String parameterNameNumber = "NUMBER_OF_ORDERS";
    private static final String parameterNameChain = "CHAIN_TO_START";
    private static final int defaultRuntime = 5;

    @Override
    public boolean spooler_process() {
        String paramRuntime = spooler_task.order().params().value(parameterNameRuntime);
        String paramChain = spooler_task.order().params().value(parameterNameChain);
        String paramNumber = spooler_task.order().params().value(parameterNameNumber);
        spooler_log.info(spooler_task.order().params().names());

        if (paramNumber.equals(blank))
            throw new SchedulerException("parameter '" + parameterNameNumber + "' is not set.");

        int runtime = (paramRuntime.equals(blank)) ?  defaultRuntime : Integer.parseInt(paramRuntime);
        int max = Integer.parseInt(paramNumber);

        spooler_log.info("create " + max + " orders with runtime " + runtime + " seconds.");
        for(int i = 1; i <= max; i++) {
            spooler_log.info("starting order " + i + " of " + max);
            Order newOrder = spooler.create_order();
            newOrder.set_id("order_" + i);
            newOrder.params().set_value(parameterNameRuntime,String.valueOf(runtime));
            Job_chain jobchain = spooler.job_chain(paramChain);
            jobchain.add_or_replace_order(newOrder);   // starts the order immidiately
        }
        return true;
    }
}
