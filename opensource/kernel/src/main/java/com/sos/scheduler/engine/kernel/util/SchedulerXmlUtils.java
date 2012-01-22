package com.sos.scheduler.engine.kernel.util;

import java.io.ByteArrayInputStream;
import java.nio.charset.Charset;

import static com.google.common.base.Charsets.ISO_8859_1;

public final class SchedulerXmlUtils {
    /** Bytes, die in einem String zwischen C++ und Java ausgetauscht werden. */
    public static final Charset byteEncoding = ISO_8859_1;

    /** Liefert ein byte-codiertes XML-Dokument als String. */
    public static String xmlFromCppByteString(String byteString) {
        return XmlUtils.toXml(XmlUtils.loadXml(new ByteArrayInputStream(byteString.getBytes(byteEncoding))));
    }

    private SchedulerXmlUtils() {}
}
