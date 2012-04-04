package com.sos.scheduler.engine.plugins.databasequery;

import com.sos.scheduler.engine.kernel.command.GenericResultXmlizer;
import com.sos.scheduler.engine.data.database.TaskHistoryEntity;
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

    private static Element elementOfTaskHistoryEntity(Document doc, TaskHistoryEntity e) {
        Element result = doc.createElement("row");
        if (!e.getClusterMemberId().isEmpty())
            result.setAttribute("clusterMemberId", e.getClusterMemberId());
        if (!e.getSchedulerId().isEmpty())
            result.setAttribute("schedulerId", e.getSchedulerId().asString());
        if (!e.getJobPath().isEmpty())
            result.setAttribute("job", e.getJobPath());
        if (e.getCause() != null)
            result.setAttribute("cause", e.getCause());
        if (e.getSteps() != null)
            result.setAttribute("steps", e.getSteps().toString());
        if (e.getStartTime() != null)
            result.setAttribute("startedAt", e.getStartTime().toString());
        if (e.getEndTime() != null)
            result.setAttribute("endedAt", e.getEndTime().toString());
        if (e.getErrorCode() != null)
            result.setAttribute("errorCode", e.getErrorCode());
        if (e.getErrorText() != null)
            result.setAttribute("errorText", e.getErrorText());

        return result;
    }
}
