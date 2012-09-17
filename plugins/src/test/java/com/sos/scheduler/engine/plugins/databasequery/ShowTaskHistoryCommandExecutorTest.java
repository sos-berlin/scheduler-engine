package com.sos.scheduler.engine.plugins.databasequery;

import javax.persistence.EntityManager;

import com.sos.scheduler.engine.data.scheduler.ClusterMemberId;
import com.sos.scheduler.engine.data.scheduler.SchedulerId;
import org.junit.*;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.*;

public final class ShowTaskHistoryCommandExecutorTest {
    private final EntityManager em = null;  // TOOO Wir brauchen eine Attrappe unserer Datenbank
    private static final SchedulerId schedulerId = new SchedulerId("SCHEDULER_ID");
    private static final ClusterMemberId clusterMemberId = new ClusterMemberId("CLUSTER_MEMBER_ID");
    private final ShowTaskHistoryCommandExecutor executor = new ShowTaskHistoryCommandExecutor(em, schedulerId, clusterMemberId);

    @Ignore @Test public void testDoExecute() {
        ShowTaskHistoryCommand command = new ShowTaskHistoryCommand(ShowTaskHistoryCommand.defaultLimit);
        TaskHistoryEntriesResult result = executor.doExecute(command);
        assertThat(result.getTaskHistoryEntries().size(), greaterThan(0));
    }
}