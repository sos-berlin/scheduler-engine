package com.sos.scheduler.engine.kernel.util;

import java.io.ByteArrayInputStream;
import java.nio.charset.Charset;

import static com.google.common.base.Charsets.ISO_8859_1;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;

public final class SchedulerXmlUtils {
    /** Bytes, die in einem String zwischen C++ und Java ausgetauscht werden. */
    public static final Charset byteEncoding = ISO_8859_1;

    /** Liefert ein byte-codiertes XML-Dokument als String. */
    public static String xmlFromCppByteString(String byteString) {
        return toXml(loadXml(new ByteArrayInputStream(byteArrayFromCppByteString(byteString))));
    }

    /** Liefert ein byte-codiertes XML-Dokument als byte[]. */
    public static byte[] byteArrayFromCppByteString(String byteString) {
        return byteString.getBytes(byteEncoding);
    }

    private SchedulerXmlUtils() {}
}
