package com.sos.scheduler.engine.plugins.databasequery;

import com.sos.scheduler.engine.kernel.command.GenericCommandExecutor;
import com.sos.scheduler.engine.kernel.database.DatabaseSubsystem;
import com.sos.scheduler.engine.kernel.database.entity.TaskHistoryEntity;
import java.util.List;
import javax.persistence.EntityManager;
import javax.persistence.TypedQuery;


public class ShowTaskHistoryCommandExecutor extends GenericCommandExecutor<ShowTaskHistoryCommand,TaskHistoryEntriesResult> {
    private final EntityManager em;
    private final TypedQuery<TaskHistoryEntity> query;
    

    public ShowTaskHistoryCommandExecutor(EntityManager db, String schedulerId, String clusterMemberId) {
        super(ShowTaskHistoryCommand.class);
        this.em = db;
        this.query = createQuery(schedulerId, clusterMemberId);
    }


    private TypedQuery<TaskHistoryEntity> createQuery(String schedulerId, String clusterMemberId) {
        TypedQuery<TaskHistoryEntity> result;
        String sql1 = "select t from TaskHistoryEntity t where t.schedulerId = :schedulerId and t.jobPath <> :schedulerDummyJobPath";
        String orderClause = " order by t.id";
        if (clusterMemberId.isEmpty()) {
            String sql = sql1 + " and t.clusterMemberId is null" + orderClause;
            result = em.createQuery(sql, TaskHistoryEntity.class);
        } else {
            String sql = sql1 + " and t.clusterMemberId = :clusterMemberId" + orderClause;
            result = em.createQuery(sql, TaskHistoryEntity.class);
            result.setParameter("clusterMemberId", clusterMemberId);
        }        
        result.setParameter("schedulerId", DatabaseSubsystem.idForDatabase(schedulerId));
        result.setParameter("schedulerDummyJobPath", TaskHistoryEntity.schedulerDummyJobPath);
        return result;
    }


    @Override protected final TaskHistoryEntriesResult doExecute(ShowTaskHistoryCommand c) {
        //TODO Ergebnis könnte sehr groß sein. Und C++ kopiert XML-Elemente mit clone().
        query.setMaxResults(c.getLimit());
        List<TaskHistoryEntity> resultList = query.getResultList();
        return new TaskHistoryEntriesResult(resultList);
    }
}
