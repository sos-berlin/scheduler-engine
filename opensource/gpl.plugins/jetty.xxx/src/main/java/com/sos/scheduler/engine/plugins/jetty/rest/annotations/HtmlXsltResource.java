package com.sos.scheduler.engine.plugins.jetty.rest.annotations;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.METHOD)
public @interface HtmlXsltResource {
    /** Pfad der XSLT-Resource zur Konvertierung von XML nach HTML. */
    String path();
}
