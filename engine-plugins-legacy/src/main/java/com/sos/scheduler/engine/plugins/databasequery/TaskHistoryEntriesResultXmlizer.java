package com.sos.scheduler.engine.plugins.databasequery;

import com.sos.scheduler.engine.data.job.TaskHistoryEntry;
import com.sos.scheduler.engine.kernel.command.GenericResultXmlizer;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

import static com.sos.scheduler.engine.common.xml.XmlUtils.newDocument;

class TaskHistoryEntriesResultXmlizer extends GenericResultXmlizer<TaskHistoryEntriesResult> {
    static final TaskHistoryEntriesResultXmlizer singleton = new TaskHistoryEntriesResultXmlizer();

    TaskHistoryEntriesResultXmlizer() {
        super(TaskHistoryEntriesResult.class);
    }

    @Override protected final Element doToElement(TaskHistoryEntriesResult r) {
        Document doc = newDocument();
        Element result = doc.createElement("myResult");
        for (TaskHistoryEntry e: r.getTaskHistoryEntries())
            result.appendChild(elementOfTaskHistoryEntity(doc, e));
        return result;
    }

    private static Element elementOfTaskHistoryEntity(Document doc, TaskHistoryEntry o) {
        Element result = doc.createElement("row");
        setAttributeOptional(result, "clusterMemberId", o.clusterMemberId().string());
        setAttributeOptional(result, "schedulerId", o.schedulerId().string());
        setAttributeOptional(result, "job", o.jobPath().string());
        setAttributeOptional(result, "cause", o.cause());
        if (o.stepsOption().isDefined())
            result.setAttribute("steps", o.stepsOption().get().toString());
        result.setAttribute("startedAt", o.startTime().toString());
        if (o.endTimeOption().isDefined())
            result.setAttribute("endedAt", o.endTimeOption().get().toString());
        setAttributeOptional(result, "errorCode", o.errorCode());
        setAttributeOptional(result, "errorText", o.errorText());
        return result;
    }

    private static void setAttributeOptional(Element e, String name, String value) {
        if (!value.isEmpty())
            e.setAttribute(name, value);
    }
}
