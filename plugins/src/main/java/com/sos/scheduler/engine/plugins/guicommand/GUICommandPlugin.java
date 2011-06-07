package com.sos.scheduler.engine.plugins.guicommand;

import java.util.List;
import javax.persistence.TypedQuery;
import javax.persistence.EntityManager;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.SchedulerException;
import com.sos.scheduler.engine.kernel.command.InvalidCommandException;
import com.sos.scheduler.engine.kernel.database.DatabaseSubsystem;
import com.sos.scheduler.engine.kernel.database.entity.TaskHistoryEntity;
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin;
import com.sos.scheduler.engine.kernel.plugin.CommandPlugin3;
import com.sos.scheduler.engine.kernel.plugin.PlugIn;
import com.sos.scheduler.engine.kernel.plugin.PlugInCommandCommand;
import com.sos.scheduler.engine.kernel.plugin.PlugInCommandResult;
import com.sos.scheduler.engine.kernel.plugin.PlugInFactory;
import java.sql.SQLException;
import java.util.Collection;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;


//TODO SQL gegen alle möglichen Datenbanktypen prüfen (vielleicht JPA oder JPQL verwenden?
//TODO Ergebnis könnte sehr groß sein. C++ kopiert XML-Elemente mit clone(). Am besten auf TCP streamen.
public class GUICommandPlugin extends AbstractPlugin implements CommandPlugin3 {
    private final DatabaseSubsystem databaseSubsystem;


    GUICommandPlugin(DatabaseSubsystem db) {
        this.databaseSubsystem = db;
    }


    @Override public final void close() {
    }

    
    @Override public final PlugInCommandResult executeCommand(PlugInCommandCommand c) {
        Element result;
        Collection<Element> childElements = elementsXPath(c.getElement(), "*");
        if (childElements.size() != 1)  throw new InvalidCommandException(c);
        Element element = childElements.iterator().next();
        if (element.getNodeName().equals("showTaskHistory"))    //TODO Als Command implementieren
            result = executeShowTaskHistory(element);
        else
        if (element.getNodeName().equals("test"))
            result = executeTest(element);
        else
            throw new InvalidCommandException(c);
        return new PlugInCommandResult(result);
    }


    private Element executeShowTaskHistory(Element element) {
        try {
            Document doc = newDocument();
            String sql = "select t from " + TaskHistoryEntity.class.getSimpleName() + " t";
            return xmlFromSqlQuery(doc, sql);
        } catch (SQLException x) { throw new SchedulerException(x); }
    }


    private Element xmlFromSqlQuery(Document doc, String sql) throws SQLException {
        EntityManager em = databaseSubsystem.getEntityManager();
        TypedQuery<TaskHistoryEntity> q = em.createQuery(sql, TaskHistoryEntity.class);
        List<TaskHistoryEntity> resultSet = q.getResultList();
        return xmlFromTaskHistoryEntities(doc, resultSet);
    }


    private Element xmlFromTaskHistoryEntities(Document doc, List<TaskHistoryEntity> entities) {
        Element result = doc.createElement("myResult");
        for (TaskHistoryEntity e: entities)
            result.appendChild(elementOfTaskHistoryEntity(doc, e));
        return result;
    }


    private Element elementOfTaskHistoryEntity(Document doc, TaskHistoryEntity e) {
        Element result = doc.createElement("row");
        result.setAttribute("clusterMemberId", e.getClusterMemberId());
        result.setAttribute("spoolerId", e.getSpoolerId());
        result.setAttribute("job", e.getJobName());
        return result;
    }


    private Element executeTest(Element element) {
        return newDocument().createElement("testResult");
    }


	public static PlugInFactory factory() {
    	return new PlugInFactory() {
            @Override public PlugIn newInstance(Scheduler scheduler, Element plugInElement) {
            	return new GUICommandPlugin(scheduler.getDatabaseSubsystem());
            }
        };
    }
}
