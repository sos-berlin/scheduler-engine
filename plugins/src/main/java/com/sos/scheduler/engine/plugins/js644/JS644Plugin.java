package com.sos.scheduler.engine.plugins.js644;

import javax.inject.Inject;

import com.sos.scheduler.engine.kernel.event.EventHandler;
import com.sos.scheduler.engine.kernel.event.OperationQueue;
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem;
import com.sos.scheduler.engine.kernel.folder.event.FileBasedActivatedEvent;
import com.sos.scheduler.engine.kernel.job.Job;
import com.sos.scheduler.engine.kernel.order.OrderSubsystem;
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin;

public class JS644Plugin extends AbstractPlugin {
    private final OrderSubsystem orderSubsystem;
    private final FolderSubsystem folderSubsystem;
    private final OperationQueue operationQueue;

    @Inject JS644Plugin(OrderSubsystem orderSubsystem, FolderSubsystem folderSubsystem, OperationQueue operationQueue) {
        this.orderSubsystem = orderSubsystem;
        this.folderSubsystem = folderSubsystem;
        this.operationQueue = operationQueue;
    }

    @EventHandler public void handleEvent(FileBasedActivatedEvent e) throws Exception {
        if (e.getObject() instanceof Job) {
            final Job job = (Job)e.getObject();
            if (job.isFileBasedReread()) {
                operationQueue.add(new Runnable() {
                    @Override public void run() {
                        for (JobChain jobChain : orderSubsystem.jobchainsOfJob(job)) jobChain.setForceFileReread();
                        folderSubsystem.updateFolders();
                    }
                });
            }
        }
    }
}
