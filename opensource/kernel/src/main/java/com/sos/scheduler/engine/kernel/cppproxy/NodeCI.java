package com.sos.scheduler.engine.kernel.cppproxy;


public interface NodeCI {
    String job_chain_path();
    String string_order_state();
    String string_next_state();
    String string_error_state();
    NodeC next_node();
    NodeC error_node();
    String string_action();
    void set_action_string(String o);
}
