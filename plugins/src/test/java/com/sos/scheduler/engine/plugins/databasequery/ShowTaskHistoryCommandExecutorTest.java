package com.sos.scheduler.engine.plugins.databasequery;

import javax.persistence.EntityManager;
import org.junit.*;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;


public class ShowTaskHistoryCommandExecutorTest {
    private final EntityManager em = null;  // TOOO Wir brauchen eine Attrappe unserer Datenbank
    private static final String schedulerId = "SCHEDULER_ID";
    private static final String clusterMemberId = "CLUSTER_MEMBER_ID";
    private final ShowTaskHistoryCommandExecutor executor = new ShowTaskHistoryCommandExecutor(em, schedulerId, clusterMemberId);


    @Ignore @Test public void testDoExecute() {
        ShowTaskHistoryCommand command = new ShowTaskHistoryCommand(ShowTaskHistoryCommand.defaultLimit);
        TaskHistoryEntriesResult result = executor.doExecute(command);
        assertThat(result.getTaskHistoryEntries().size(), greaterThan(0));
    }
}