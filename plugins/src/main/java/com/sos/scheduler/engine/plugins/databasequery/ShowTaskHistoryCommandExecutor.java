package com.sos.scheduler.engine.plugins.databasequery;

import com.sos.scheduler.engine.kernel.command.GenericCommandExecutor;
import com.sos.scheduler.engine.kernel.database.DatabaseSubsystem;
import com.sos.scheduler.engine.kernel.database.entity.TaskHistoryEntity;
import java.util.List;
import javax.persistence.EntityManager;
import javax.persistence.TypedQuery;


public class ShowTaskHistoryCommandExecutor extends GenericCommandExecutor<ShowTaskHistoryCommand,TaskHistoryEntriesResult> {
    private final DatabaseSubsystem db;
    

    public ShowTaskHistoryCommandExecutor(DatabaseSubsystem db) {
        super(ShowTaskHistoryCommand.class);
        this.db = db;
    }


    @Override protected final TaskHistoryEntriesResult doExecute(ShowTaskHistoryCommand c) {
        //TODO Ergebnis könnte sehr groß sein. Und C++ kopiert XML-Elemente mit clone().
        String sql = "select t from " + TaskHistoryEntity.class.getSimpleName() + " t";
        EntityManager em = db.getEntityManager();
        TypedQuery<TaskHistoryEntity> q = em.createQuery(sql, TaskHistoryEntity.class);
        List<TaskHistoryEntity> resultSet = q.getResultList();
        return new TaskHistoryEntriesResult(resultSet);
    }
}
