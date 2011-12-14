package com.sos.scheduler.engine.util.xml;

import java.util.Iterator;
import org.w3c.dom.Element;

public class NamedChildElements implements Iterable<Element> {
    private final String name;
    private final Element parent;

    public NamedChildElements(String elementName, Element parent) {
        this.name = elementName;
        this.parent = parent;
    }

    @Override public final Iterator<Element> iterator() {
        return new NamedSiblingElementIterator(name, parent.getFirstChild());
    }
}
