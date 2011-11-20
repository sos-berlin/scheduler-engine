package com.sos.scheduler.engine.kernel.plugin;

import com.sos.scheduler.engine.kernel.Scheduler;

import org.w3c.dom.Element;

/** Ein Plugin muss eine statische Methode implementieren, die Objekt dieser Klasse liefert.
 * Die Methode wird über Reflection aufgerufen.
 * 
 * public static PluginFactory newFactory();
*/
public interface PluginFactory {
    Plugin newInstance(Scheduler scheduler, Element configurationOrNull);
}
