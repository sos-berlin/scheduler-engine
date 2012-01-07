package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.http.SchedulerHttpResponse;

@CppClass(clas="sos::scheduler::http::Java_response", directory="scheduler", include="spooler.h")
public interface HttpResponseC extends CppProxy {
    void close();
    HttpChunkReaderC chunk_reader();
    int status();
    String header_string();
}
