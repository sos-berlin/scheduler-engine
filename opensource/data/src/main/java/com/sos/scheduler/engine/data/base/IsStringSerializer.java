package com.sos.scheduler.engine.data.base;

import java.io.IOException;
import org.codehaus.jackson.JsonGenerator;
import org.codehaus.jackson.map.JsonSerializer;
import org.codehaus.jackson.map.SerializerProvider;

/** @author Joacim Zschimmer */
public final class IsStringSerializer extends JsonSerializer<IsString> {
    public static final IsStringSerializer singleton = new IsStringSerializer();

    private IsStringSerializer() {}

    @Override public Class<IsString> handledType() {
        return IsString.class;
    }

    @Override
    public void serialize(IsString o, JsonGenerator g, SerializerProvider p) throws IOException {
        g.writeString(o.string());
    }
}
