package com.sos.scheduler.engine.kernel.scheduler;

import com.google.inject.ImplementedBy;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.cppproxy.HttpResponseC;
import com.sos.scheduler.engine.kernel.http.SchedulerHttpRequest;
import com.sos.scheduler.engine.kernel.http.SchedulerHttpResponse;

@ImplementedBy(Scheduler.class)
public interface SchedulerHttpService {
    /** @return {@link HttpResponseC#Release()} ()} MUSS aufgerufen werden! */
    HttpResponseC executeHttpRequest(SchedulerHttpRequest request, SchedulerHttpResponse response);
}
