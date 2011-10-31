package com.sos.scheduler.engine.plugins.js644;

import org.w3c.dom.Element;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.Event;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.event.EventSubsystem;
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

    JS644Plugin(EventSubsystem eventSubsystem, FolderSubsystem folderSubsystem, OrderSubsystem orderSubsystem) {
        this.eventSubsystem = eventSubsystem;
        this.folderSubsystem = folderSubsystem;
        this.orderSubsystem = orderSubsystem;
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
            @Override public Plugin newInstance(Scheduler scheduler, Element plugInElement) {
            	return new JS644Plugin(scheduler.getEventSubsystem(), scheduler.getFolderSubsystem(), scheduler.getOrderSubsystem());
            }
        };
    }

    class MyEventSubscriber implements EventSubscriber {
        @Override public void onEvent(Event event) throws Exception {
            if (event instanceof FileBasedActivatedEvent) {
                FileBasedActivatedEvent e = (FileBasedActivatedEvent)event;
                if (e.getObject() instanceof Job) {
                    Job job = (Job)e.getObject();
                    if (job.isFileBasedReloaded()) {
                        onReloadedJobActivated(job);
                    }
                }
            }
        }

        private void onReloadedJobActivated(Job job) {
            for (JobChain jobChain: orderSubsystem.ofJob(job)) jobChain.setForceFileReload();
            folderSubsystem.updateFolders(0);
        }
    }
}
