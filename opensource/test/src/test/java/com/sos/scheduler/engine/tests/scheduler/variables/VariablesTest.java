package com.sos.scheduler.engine.tests.scheduler.variables;

import static org.hamcrest.Matchers.containsInAnyOrder;
import static org.hamcrest.Matchers.equalTo;
import static org.junit.Assert.assertThat;

import java.util.Collection;

import org.junit.Test;

import com.sos.scheduler.engine.kernel.variable.VariableSet;
import com.sos.scheduler.engine.test.SchedulerTest;

public final class VariablesTest extends SchedulerTest {
    @Test public void test() {
        controller().startScheduler();
        VariableSet variables = scheduler().getVariables();
        Collection<String> names = variables.getNames();
        assertThat(names, containsInAnyOrder("Ä", "Bb", "Ccc"));
        assertThat(variables.get("Ä"), equalTo("ä"));
        assertThat(variables.get("Bb"), equalTo("bb"));
        assertThat(variables.get("Ccc"), equalTo("ccc"));
        controller().terminateScheduler();
    }
}
