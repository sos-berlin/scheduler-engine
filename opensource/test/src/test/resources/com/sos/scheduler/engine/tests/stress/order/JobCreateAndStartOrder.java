package com.sos.scheduler.engine.tests.stress.order;

import sos.spooler.Job_impl;
import sos.spooler.Order;

public class JobCreateAndStartOrder extends Job_impl {
    @Override
    public boolean spooler_process() {
        Order order = spooler_task.order();
        Order newOrder = spooler.create_order();
        newOrder.set_id(order.id());
        order.job_chain().add_or_replace_order(newOrder);   // starts the order immidiately
        return true;
    }
}
