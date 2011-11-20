package com.sos.scheduler.engine.kernel.folder;

import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.scheduler.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.scheduler.Platform;

@ForCpp
public abstract class FileBased extends AbstractHasPlatform implements Sister {
    protected FileBased(Platform p) {
        super(p);
    }
}
