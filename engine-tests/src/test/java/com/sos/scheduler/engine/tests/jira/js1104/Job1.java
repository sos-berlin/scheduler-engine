package com.sos.scheduler.engine.tests.jira.js1104;

import sos.spooler.Job_chain;
import sos.spooler.Job_impl;

public class Job1 extends Job_impl {

    @Override public final boolean spooler_process() {
        Job_chain chain1 = spooler.job_chain( "chain1" );
        String[] states = chain1.states();

        // Check test conditions
        if ( states.length != 3 ) {
            throw new RuntimeException("states.length = "+ states.length +" (must be 3)");
        }

        checkState(states[0], "STATE_1");
        checkState(states[1], "SUCCESS");
        checkState(states[2], "ERROR");

        spooler.terminate();
        return false;
    }


    private void checkState(String state, String expectedState) {
        if ( !state.equals(expectedState) ) {
            throw new RuntimeException("state = " + state + " (must be "+expectedState+")");
        }
    }
}
