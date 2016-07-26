package com.sos.scheduler.engine.tests.scheduler.variables;

import com.sos.scheduler.engine.kernel.variable.SchedulerVariableSet;
import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Test;
import static org.hamcrest.Matchers.equalTo;
import static org.junit.Assert.assertThat;

public final class VariablesIT extends SchedulerTest {
    @Test public void test() {
        controller().activateScheduler();
        SchedulerVariableSet variables = instance(SchedulerVariableSet.class);
        assertThat(variables.apply("Ä"), equalTo("ä"));
        assertThat(variables.apply("Bb"), equalTo("bb"));
        assertThat(variables.apply("Ccc"), equalTo("ccc"));
        controller().terminateScheduler();
    }
}
