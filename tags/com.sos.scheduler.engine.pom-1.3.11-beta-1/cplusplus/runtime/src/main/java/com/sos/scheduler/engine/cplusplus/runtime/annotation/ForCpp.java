package com.sos.scheduler.engine.cplusplus.runtime.annotation;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/** Lässt den Java/C++-Generator ein Java-Proxy der Klasse generieren, 
 * wenn der Generator mit einem umfassenden Paketnamen aufgerufen wird.
 */
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.TYPE)
public @interface ForCpp {}
