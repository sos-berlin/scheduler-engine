package com.sos.scheduler.engine.data.job;

import org.codehaus.jackson.JsonParser;
import org.codehaus.jackson.map.DeserializationContext;
import org.codehaus.jackson.map.JsonDeserializer;

import java.io.IOException;

class TaskIdDeserializer extends JsonDeserializer<TaskId> {
    @Override
    public final TaskId deserialize(JsonParser jsonParser, DeserializationContext deserializationContext) throws IOException {
        return new TaskId( Integer.parseInt(jsonParser.getText()) );
    }
}
