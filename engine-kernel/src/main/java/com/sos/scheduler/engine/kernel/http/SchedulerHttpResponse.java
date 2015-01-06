package com.sos.scheduler.engine.kernel.http;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;

@ForCpp public abstract class SchedulerHttpResponse {
    @ForCpp public abstract void onNextChunkIsReady();
}
