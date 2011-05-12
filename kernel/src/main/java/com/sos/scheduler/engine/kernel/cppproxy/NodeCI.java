package com.sos.scheduler.engine.kernel.cppproxy;


public interface NodeCI {
    String string_order_state();
    String string_next_state();
    String string_error_state();
    NodeC next_node();
    NodeC error_node();
}
