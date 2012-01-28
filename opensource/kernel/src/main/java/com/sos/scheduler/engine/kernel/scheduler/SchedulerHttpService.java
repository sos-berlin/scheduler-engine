package com.sos.scheduler.engine.kernel.scheduler;

import com.sos.scheduler.engine.kernel.cppproxy.HttpResponseC;
import com.sos.scheduler.engine.kernel.http.SchedulerHttpRequest;
import com.sos.scheduler.engine.kernel.http.SchedulerHttpResponse;

public interface SchedulerHttpService {
    /** @return {@link HttpResponseC#Release()} ()} MUSS aufgerufen werden! */
    HttpResponseC executeHttpRequest(SchedulerHttpRequest request, SchedulerHttpResponse response);
}
