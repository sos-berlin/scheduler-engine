package com.sos.scheduler.engine.cplusplus.runtime.annotation;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;


/** C++-Methode ist selbst thread-sicher, sie muss nicht durch ein Mutex abgesichert werden. */
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.METHOD)
public @interface CppThreadSafe {}
