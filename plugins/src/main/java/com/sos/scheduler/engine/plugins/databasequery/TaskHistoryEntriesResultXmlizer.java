package com.sos.scheduler.engine.plugins.databasequery;

import com.sos.scheduler.engine.kernel.command.GenericResultXmlizer;
import com.sos.scheduler.engine.kernel.database.entity.TaskHistoryEntity;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.newDocument;


public class TaskHistoryEntriesResultXmlizer extends GenericResultXmlizer<TaskHistoryEntriesResult> {
    public static final TaskHistoryEntriesResultXmlizer singleton = new TaskHistoryEntriesResultXmlizer();

    
    public TaskHistoryEntriesResultXmlizer() {
        super(TaskHistoryEntriesResult.class);
    }


    @Override protected final Element doToElement(TaskHistoryEntriesResult r) {
        Document doc = newDocument();
        Element result = doc.createElement("myResult");
        for (TaskHistoryEntity e: r.getTaskHistoryEntries())
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
}
