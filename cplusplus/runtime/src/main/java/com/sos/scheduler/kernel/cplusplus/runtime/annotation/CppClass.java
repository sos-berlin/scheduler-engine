package com.sos.scheduler.kernel.cplusplus.runtime.annotation;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;


@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.TYPE)
public @interface CppClass
{
    /** Name der C++-Klasse */
    String clas() default "";

    /** Header-Datei, das vom generierten C++-Code eingelesen werden soll */
    String include() default "";

    /** Unterverzeichnis, in das der C++-Code geschrieben werden soll */
    String directory() default "";
}
