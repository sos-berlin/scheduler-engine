package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.ReleasableCppProxy;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;

@CppClass(clas="sos::scheduler::http::Java_response", directory="scheduler", include="spooler.h")
public interface HttpResponseC extends ReleasableCppProxy {
    void close();
    HttpChunkReaderC chunk_reader();
    int status();
    String header_string();
}
