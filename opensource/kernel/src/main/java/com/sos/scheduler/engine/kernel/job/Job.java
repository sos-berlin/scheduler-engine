package com.sos.scheduler.engine.kernel.job;

import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.Platform;
import com.sos.scheduler.engine.kernel.cppproxy.JobC;
import com.sos.scheduler.engine.kernel.folder.AbsolutePath;
import com.sos.scheduler.engine.kernel.folder.FileBased;
import com.sos.scheduler.engine.kernel.folder.FileBasedState;

@ForCpp
public final class Job extends FileBased implements Sister {
    private final JobC cppProxy;

    private Job(Platform platform, JobC jobC) {
        super(platform);
        this.cppProxy = jobC;
    }

    @Override public void onCppProxyInvalidated() {}

    public FileBasedState getFileBasedState() {
        return FileBasedState.ofCppName(cppProxy.file_based_state_name());
    }

    public String getName() {
        return cppProxy.name();
    }

    public AbsolutePath getPath() {
        return new AbsolutePath(cppProxy.path());
    }

    /** @return true, wenn das {@link FileBased} nach einer Änderung erneut geladen worden ist. */
    public boolean isFileBasedReread() {
        return cppProxy.is_file_based_reread();
    }

    @Override public String toString() {
        return getClass().getSimpleName() + " " + getPath();
    }

    public static class Type implements SisterType<Job, JobC> {
        @Override public final Job sister(JobC proxy, Sister context) {
            return new Job(Platform.of(context), proxy);
        }
    }
}
