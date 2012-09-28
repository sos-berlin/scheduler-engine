package com.sos.scheduler.engine.kernel.job;

import com.google.inject.Injector;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.data.folder.FileBasedType;
import com.sos.scheduler.engine.data.folder.JobPath;
import com.sos.scheduler.engine.data.job.JobPersistentState;
import com.sos.scheduler.engine.kernel.cppproxy.JobC;
import com.sos.scheduler.engine.kernel.folder.FileBased;
import com.sos.scheduler.engine.kernel.folder.FileBasedState;
import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.scheduler.HasInjector;
import org.joda.time.DateTime;
import scala.Option;

import javax.annotation.Nullable;

import static com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.eternalMillis;
import static com.sos.scheduler.engine.kernel.util.SchedulerXmlUtils.byteArrayFromCppByteString;
import static org.joda.time.DateTimeZone.UTC;

@ForCpp
public final class Job extends FileBased implements Sister, UnmodifiableJob {
    private final JobC cppProxy;
    private final Injector injector;

    private Job(JobC jobC, Injector injector) {
        this.cppProxy = jobC;
        this.injector = injector;
    }

    @Override public void onCppProxyInvalidated() {}

    @Override public FileBasedState getFileBasedState() {
        return FileBasedState.ofCppName(cppProxy.file_based_state_name());
    }

    @Override public String getName() {
        return cppProxy.name();
    }

    @Override public FileBasedType getFileBasedType() {
        return FileBasedType.job;
    }

    @Override public JobPath getPath() {
        return JobPath.of(cppProxy.path());
    }

    public byte[] getConfigurationXmlBytes() {
        return byteArrayFromCppByteString(cppProxy.source_xml());
    }

    public String getDescription() {
        return cppProxy.description();
    }

    /** @return true, wenn das {@link FileBased} nach einer Änderung erneut geladen worden ist. */
    @Override public boolean isFileBasedReread() {
        return cppProxy.is_file_based_reread();
    }

    @Override public PrefixLog getLog() {
        return cppProxy.log().getSister();
    }

    public JobState state() {
        return JobState.valueOf(cppProxy.state_name());
    }

    public void endTasks() {
        setStateCommand(JobStateCommand.endTasks);
    }

    public void setStateCommand(JobStateCommand c) {
        cppProxy.set_state_cmd(c.cppValue());
    }

    @ForCpp @Nullable public JobPersistentState tryFetchPersistentState() {
        Option<JobPersistentState> s = persistentStateStore().tryFetch(getPath());
        return s.isDefined()? s.get() : null;
    }

    @ForCpp public void persistState() {
        persistentStateStore().store(persistentState());
    }

    @ForCpp public void deletePersistentState() {
        persistentStateStore().delete(getPath());
    }

    private JobPersistentStateStore persistentStateStore() {
        return injector.getInstance(JobPersistentStateStore.class);
    }

    private JobPersistentState persistentState() {
        long nextStartTimeMillis = cppProxy.next_start_time_millis();
        return new JobPersistentState(
                getPath(),
                Option.apply(nextStartTimeMillis == eternalMillis? null : new DateTime(nextStartTimeMillis, UTC)),
                isPermanentlyStopped());
    }

    public boolean isPermanentlyStopped() {
        return cppProxy.is_permanently_stopped();
    }

    @Override public String toString() {
        return getClass().getSimpleName() + " " + getPath().asString();
    }

    public static class Type implements SisterType<Job, JobC> {
        @Override public final Job sister(JobC proxy, Sister context) {
            return new Job(proxy, ((HasInjector)context).getInjector());
        }
    }
}
