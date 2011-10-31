package com.sos.scheduler.engine.plugins.js644;

import javax.inject.Inject;

import org.w3c.dom.Element;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.EventHandler;
import com.sos.scheduler.engine.kernel.event.SchedulerOperation;
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem;
import com.sos.scheduler.engine.kernel.folder.event.FileBasedActivatedEvent;
import com.sos.scheduler.engine.kernel.job.Job;
import com.sos.scheduler.engine.kernel.order.OrderSubsystem;
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin;
import com.sos.scheduler.engine.kernel.plugin.Plugin;
import com.sos.scheduler.engine.kernel.plugin.PluginFactory;

public class JS644Plugin extends AbstractPlugin {
    private final FolderSubsystem folderSubsystem;
    private final OrderSubsystem orderSubsystem;

    @Inject JS644Plugin(FolderSubsystem folderSubsystem, OrderSubsystem orderSubsystem) {
        this.folderSubsystem = folderSubsystem;
        this.orderSubsystem = orderSubsystem;
    }

    @EventHandler public SchedulerOperation handleEvent(FileBasedActivatedEvent e) throws Exception {
        if (e.getObject() instanceof Job) {
            final Job job = (Job)e.getObject();
            if (job.isFileBasedReread()) {
                return new SchedulerOperation() {
                    @Override public void execute() throws Exception {
                        for (JobChain jobChain: orderSubsystem.jobchainsOfJob(job)) jobChain.setForceFileReread();
                        folderSubsystem.updateFolders(0);
                    }
                };
            }
        }
        return null;
    }
}
