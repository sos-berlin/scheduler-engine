package com.sos.scheduler.engine.tests.jira.js1103;

import sos.spooler.Job_chain;
import sos.spooler.Job_impl;
import sos.spooler.Order;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

public class Job1 extends Job_impl {

    @Override public final boolean spooler_process() {
        Job_chain chain = spooler.job_chain( "chain1" );

        Order order1 = spooler.create_order();
        order1.params().set_value("suspend", "yes"); // to reach max_orders=1
        chain.add_order(order1); // order gets stuck in job_chain (suspended) -> max_orders is reached

        Order order2 = spooler.create_order();
        assertFalse("ignore_max_orders()", order2.ignore_max_orders());

        order2.set_ignore_max_orders(true);
        assertTrue("ignore_max_orders()", order2.ignore_max_orders());

        chain.add_order(order2);

        return false;
    }
}
