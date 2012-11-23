package com.sos.scheduler.engine.plugins.databasequery;

import com.sos.scheduler.engine.data.scheduler.ClusterMemberId;
import com.sos.scheduler.engine.data.scheduler.SchedulerId;
import org.junit.Ignore;
import org.junit.Test;

import javax.persistence.EntityManagerFactory;

import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.greaterThan;

public final class ShowTaskHistoryCommandExecutorTest {
    private final EntityManagerFactory entityManagerFactory = null;  // TOOO Wir brauchen eine Attrappe unserer Datenbank
    private static final SchedulerId schedulerId = new SchedulerId("SCHEDULER_ID");
    private static final ClusterMemberId clusterMemberId = new ClusterMemberId("CLUSTER_MEMBER_ID");
    private final ShowTaskHistoryCommandExecutor executor = new ShowTaskHistoryCommandExecutor(schedulerId, clusterMemberId, entityManagerFactory);

    @Ignore @Test public void testDoExecute() {
        ShowTaskHistoryCommand command = new ShowTaskHistoryCommand(ShowTaskHistoryCommand.defaultLimit);
        TaskHistoryEntriesResult result = executor.doExecute(command);
        assertThat(result.getTaskHistoryEntries().size(), greaterThan(0));
    }
}