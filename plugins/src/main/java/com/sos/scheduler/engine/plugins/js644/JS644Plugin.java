package com.sos.scheduler.engine.plugins.js644;

import java.util.concurrent.Callable;

import org.w3c.dom.Element;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.DeferredOperationExecutor;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.event.EventSubsystem;
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
    private final EventSubsystem eventSubsystem;
    private final FolderSubsystem folderSubsystem;
    private final OrderSubsystem orderSubsystem;
    private final MyEventSubscriber eventSubscriber;
    private final DeferredOperationExecutor deferredOperationExecutor;

    JS644Plugin(EventSubsystem eventSubsystem, FolderSubsystem folderSubsystem, OrderSubsystem orderSubsystem,
            DeferredOperationExecutor deferredOperationExecutor) {
        this.eventSubsystem = eventSubsystem;
        this.folderSubsystem = folderSubsystem;
        this.orderSubsystem = orderSubsystem;
        this.deferredOperationExecutor = deferredOperationExecutor;
        this.eventSubscriber = new MyEventSubscriber();
    }

    @Override public void activate() {
        eventSubsystem.subscribe(eventSubscriber);
    }

    @Override public void close() {
        eventSubsystem.unsubscribe(eventSubscriber);
    }

	public static PluginFactory factory() {
    	return new PluginFactory() {
            @Override public Plugin newInstance(Scheduler scheduler, Element pluginElement) {
            	return new JS644Plugin(scheduler.getEventSubsystem(), scheduler.getFolderSubsystem(), scheduler.getOrderSubsystem(),
                        scheduler.getDeferredOperationExecutor());
            }
        };
    }

    class MyEventSubscriber implements EventSubscriber {
        @Override public void onEvent(Event event) throws Exception {
            if (event instanceof FileBasedActivatedEvent) {
                FileBasedActivatedEvent e = (FileBasedActivatedEvent)event;
                if (e.getObject() instanceof Job) {
                    Job job = (Job)e.getObject();
                    if (job.isFileBasedReread()) {
                        deferredOperationExecutor.addOperation(new RereadJobchainsCallable(job));
                    }
                }
            }
        }
    }

    private class RereadJobchainsCallable implements SchedulerOperation {
        private final Job job;

        public RereadJobchainsCallable(Job job) {
            this.job = job;
        }

        @Override public void execute() throws Exception {
            for (JobChain jobChain: orderSubsystem.jobchainsOfJob(job)) jobChain.setForceFileReread();
            folderSubsystem.updateFolders(0);
        }
    }
}
