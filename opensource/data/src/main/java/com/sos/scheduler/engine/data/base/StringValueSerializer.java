package com.sos.scheduler.engine.data.base;

import java.io.IOException;
import org.codehaus.jackson.JsonGenerator;
import org.codehaus.jackson.map.JsonSerializer;
import org.codehaus.jackson.map.SerializerProvider;

/** @author Joacim Zschimmer */
public final class StringValueSerializer extends JsonSerializer<StringValue> {
    public static final StringValueSerializer singleton = new StringValueSerializer();

    private StringValueSerializer() {}

    @Override public Class<StringValue> handledType() {
        return StringValue.class;
    }

    @Override
    public void serialize(StringValue o, JsonGenerator g, SerializerProvider p) throws IOException {
        g.writeString(o.asString());
    }
}
