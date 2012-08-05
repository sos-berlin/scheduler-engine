package com.sos.scheduler.engine.plugins.js644;

import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.data.folder.FileBasedActivatedEvent;
import com.sos.scheduler.engine.kernel.event.OperationQueue;
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem;
import com.sos.scheduler.engine.kernel.job.Job;
import com.sos.scheduler.engine.kernel.order.OrderSubsystem;
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin;

import javax.inject.Inject;

public class JS644Plugin extends AbstractPlugin {
    private final OperationQueue operationQueue;
    private final OrderSubsystem orderSubsystem;
    private final FolderSubsystem folderSubsystem;

    @Inject JS644Plugin(OperationQueue operationQueue, OrderSubsystem orderSubsystem, FolderSubsystem folderSubsystem) {
        this.operationQueue = operationQueue;
        this.orderSubsystem = orderSubsystem;
        this.folderSubsystem = folderSubsystem;
    }

    @HotEventHandler public final void handleEvent(FileBasedActivatedEvent e, final Job job) {
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
