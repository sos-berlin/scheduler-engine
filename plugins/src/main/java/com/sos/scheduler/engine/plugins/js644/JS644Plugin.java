package com.sos.scheduler.engine.plugins.js644;

import static scala.collection.JavaConversions.asJavaIterable;

import javax.inject.Inject;

import com.sos.scheduler.engine.data.folder.FileBasedActivatedEvent;
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue;
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem;
import com.sos.scheduler.engine.kernel.job.Job;
import com.sos.scheduler.engine.kernel.order.OrderSubsystem;
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.kernel.plugin.AbstractPlugin;

public class JS644Plugin extends AbstractPlugin implements EventHandlerAnnotated {
    private final SchedulerThreadCallQueue callQueue;
    private final OrderSubsystem orderSubsystem;
    private final FolderSubsystem folderSubsystem;

    @Inject private JS644Plugin(SchedulerThreadCallQueue callQueue, OrderSubsystem orderSubsystem, FolderSubsystem folderSubsystem) {
        this.callQueue = callQueue;
        this.orderSubsystem = orderSubsystem;
        this.folderSubsystem = folderSubsystem;
    }

    @HotEventHandler public final void handleEvent(FileBasedActivatedEvent e, final Job job) {
        if (job.isFileBasedReread()) {
            callQueue.add(new Runnable() {
                @Override public void run() {
                    for (JobChain jobChain : asJavaIterable(orderSubsystem.jobChainsOfJob(job))) jobChain.forceFileReread();
                    folderSubsystem.updateFolders();
                }
            });
        }
    }
}
