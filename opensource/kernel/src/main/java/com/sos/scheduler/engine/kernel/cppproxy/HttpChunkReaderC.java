package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;

@CppClass(clas="sos::scheduler::http::Chunk_reader", directory="scheduler", include="spooler.h")
public interface HttpChunkReaderC extends CppProxy {
    String content_type();
    //int recommended_block_size();
    boolean next_chunk_is_ready();
    int get_next_chunk_size();
    String read_from_chunk(int size);
}
