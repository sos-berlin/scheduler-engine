package com.sos.scheduler.engine.kernel.http;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;

@ForCpp
public interface SchedulerHttpRequest {
    @ForCpp boolean hasParameter(String name);
    @ForCpp String parameter(String name);
    @ForCpp String header(String name);
    @ForCpp String protocol();
    @ForCpp String urlPath();
    @ForCpp String charsetName();
    @ForCpp String httpMethod();
    @ForCpp String body();
}
