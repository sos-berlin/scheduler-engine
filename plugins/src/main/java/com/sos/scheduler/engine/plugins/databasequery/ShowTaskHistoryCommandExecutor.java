package com.sos.scheduler.engine.plugins.databasequery;

import com.google.common.base.Function;
import com.sos.scheduler.engine.data.job.TaskHistoryEntry;
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId;
import com.sos.scheduler.engine.data.scheduler.SchedulerId;
import com.sos.scheduler.engine.kernel.command.GenericCommandExecutor;
import com.sos.scheduler.engine.persistence.entities.TaskHistoryEntity;
import com.sos.scheduler.engine.persistence.entities.TaskHistoryEntity$;
import com.sos.scheduler.engine.persistence.entities.TaskHistoryEntityConverter$;

import javax.inject.Inject;
import javax.persistence.EntityManager;
import javax.persistence.EntityManagerFactory;
import javax.persistence.TypedQuery;
import java.util.List;

import static com.google.common.collect.Collections2.transform;
import static com.sos.scheduler.engine.persistence.SchedulerDatabases.idForDatabase;

class ShowTaskHistoryCommandExecutor extends GenericCommandExecutor<ShowTaskHistoryCommand, TaskHistoryEntriesResult> {
    private final SchedulerId schedulerId;
    private final ClusterMemberId clusterMemberId;
    private final EntityManagerFactory entityManagerFactory;

    @Inject ShowTaskHistoryCommandExecutor(SchedulerId schedulerId, ClusterMemberId clusterMemberId, EntityManagerFactory f) {
        super(ShowTaskHistoryCommand.class);
        this.schedulerId = schedulerId;
        this.clusterMemberId = clusterMemberId;
        this.entityManagerFactory = f;
    }

    @Override protected final TaskHistoryEntriesResult doExecute(ShowTaskHistoryCommand c) {
        EntityManager entityManager = entityManagerFactory.createEntityManager();
        try {
            TypedQuery<TaskHistoryEntity> query = createQuery(entityManager);
            //TODO Ergebnis könnte sehr groß sein. Und C++ kopiert XML-Elemente mit clone().
            query.setMaxResults(c.getLimit());
            List<TaskHistoryEntity> resultList = query.getResultList();
            return new TaskHistoryEntriesResult(transform(resultList, new Function<TaskHistoryEntity, TaskHistoryEntry>() {
                @Override public TaskHistoryEntry apply(TaskHistoryEntity o) {
                    return TaskHistoryEntityConverter$.MODULE$.toObject(o);
                }
            }));
        }
        finally {
            entityManager.close();
        }
    }

    private TypedQuery<TaskHistoryEntity> createQuery(EntityManager entityManager) {
        TypedQuery<TaskHistoryEntity> result;
        String sql1 = "select t from TaskHistoryEntity t where t.schedulerId = :schedulerId and t.jobPath <> :schedulerDummyJobPath";
        String orderClause = " order by t.id";
        if (clusterMemberId.isEmpty()) {
            String sql = sql1 + " and t.clusterMemberId is null" + orderClause;
            result = entityManager.createQuery(sql, TaskHistoryEntity.class);
        } else {
            String sql = sql1 + " and t.clusterMemberId = :clusterMemberId" + orderClause;
            result = entityManager.createQuery(sql, TaskHistoryEntity.class);
            result.setParameter("clusterMemberId", clusterMemberId);
        }
        result.setParameter("schedulerId", idForDatabase(schedulerId));
        result.setParameter("schedulerDummyJobPath", TaskHistoryEntity$.MODULE$.schedulerDummyJobPath());
        return result;
    }
}
