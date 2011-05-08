<?xml version="1.0"?>
<!-- $Id: scheduler-phrases.xsl 5178 2007-08-27 09:27:39Z jz $ -->
<!-- Texte für XSLT-Stylesheets der Dokumentation: scheduler.xsl, scheduler_base.xsl, api/api.xsl -->

<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform"
                version   = "1.0">

    <xsl:variable name="lang" select="'de'"/>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~PHRASES-->

    <xsl:template name="this_is_a_container_only_for_phrases">
        
        <phrases style="">     <!-- style nur zum Debuggen -->

            <phrase id="api.title">Application Program Interface (API)</phrase>
        <!--<phrase id="api.title">Programmschnittstelle (API)</phrase>-->
            <phrase id="api.overview.title">Overview</phrase>
        <!--<phrase id="api.overview.title">Übersicht</phrase>-->
            <phrase id="api.method.deprecated">This method should not be used</phrase>
        <!--<phrase id="api.method.deprecated">Der Aufruf sollte nicht mehr verwendet werden</phrase>-->
            <phrase id="api.method.parameters.title">Parameters</phrase>
        <!--<phrase id="api.method.parameters.title">Parameter</phrase>-->
            <phrase id="api.method.return.title">Return Value</phrase>
        <!--<phrase id="api.method.return.title">Rückgabe</phrase>-->
            <phrase id="api.exceptions.title">Exceptions</phrase>
        <!--<phrase id="api.exceptions.title">Exceptions</phrase>-->
            <!--
            <phrase id="api.method.not_for_unix_perl">Nicht für Perl unter Unix</phrase>
            <phrase id="api.method.restricted_for_unix_perl">Für Perl unter Unix sind Objekt- und Array-Parameter nicht verwendbar</phrase>
            <phrase id="api.method.not_for_unix_perl.detailed">Dieser Aufruf steht in Perl unter Unix nicht zur Verfügung.</phrase>
            <phrase id="api.method.restricted_for_unix_perl.detailed">Für Perl unter Unix sind Objekt- und Array-Parameter nicht verwendbar.</phrase>
            <phrase id="api.parameter.not_for_unix_perl">Der Parameter kann in Perl unter Unix nicht gesetzt werden.</phrase>
            <phrase id="api.parameter.restricted_for_unix_perl">Für Perl unter Unix sind Objekt- und Array-Parameter nicht verwendbar.</phrase>
            -->

            <phrase id="command_line.variants.title">Variants</phrase>
        <!--<phrase id="command_line.variants.title">Aufrufvarianten</phrase>-->
            <phrase id="command_line.options.title">Options</phrase>
        <!--<phrase id="command_line.options.title">Optionen</phrase>-->
            <phrase id="command_line.option.multiple">This option can be used repeatedly.</phrase>
        <!--<phrase id="command_line.option.multiple">Die Option kann mehrfach angegeben werden.</phrase>-->
            <phrase id="command_line.option.reference">Command line option</phrase>
        <!--<phrase id="command_line.option.reference">Kommandozeilenoption</phrase>-->

            
            <phrase id="common.in_programming_language">in</phrase>
        <!--<phrase id="common.in_programming_language">in</phrase>-->
            <phrase id="common.see">see</phrase>
        <!--<phrase id="common.see">siehe</phrase>-->
            <phrase id="common.See">See</phrase>
        <!--<phrase id="common.See">Siehe</phrase>-->


            <phrase id="example.title">Example</phrase>
        <!--<phrase id="example.title">Beispiel</phrase>-->

            
            <!-- Kopf- und Fußzeilen der HTML-Seiten -->
            <phrase id="head.link_to_first_page">Home</phrase>
        <!--<phrase id="head.link_to_first_page">Erste Seite</phrase>-->
            <phrase id="head.link_to_options">Start</phrase>
        <!--<phrase id="head.link_to_options">Start</phrase>-->
            <phrase id="head.link_to_api">API</phrase>
        <!--<phrase id="head.link_to_api">API</phrase>-->
            <phrase id="head.link_to_xml">XML</phrase>
        <!-- <phrase id="head.link_to_xml">XML</phrase>-->
            <phrase id="head.link_to_index">Index</phrase>
        <!--<phrase id="head.link_to_index">Register</phrase>-->
            <phrase id="head.link_to_sos">Software- und Organisations-Service GmbH</phrase>
        <!--<phrase id="head.link_to_sos">Software- und Organisations-Service GmbH</phrase>-->
            <phrase id="head.last_updated_by">Last updated by</phrase>
        <!--<phrase id="head.last_updated_by">Zuletzt geändert von</phrase>-->

            <phrase id="index.title">Registry</phrase>
        <!--<phrase id="index.title">Register</phrase>-->

            <phrase id="ini.file.title">Files</phrase>
        <!--<phrase id="ini.file.title">Die Datei</phrase>-->
            <phrase id="ini.file">File</phrase>
        <!--<phrase id="ini.file">Datei</phrase>-->
            <phrase id="ini.sections.title">Sections</phrase>
        <!--<phrase id="ini.sections.title">Die Abschnitte</phrase>-->
            <phrase id="ini.section">section</phrase>
        <!--<phrase id="ini.section">Abschnitt</phrase>-->
            <phrase id="ini.entries.title">Entries</phrase>
        <!--<phrase id="ini.entries.title">Die Einträge</phrase>-->
            <phrase id="ini.entry">entry</phrase>
        <!--<phrase id="ini.entry">Eintrag</phrase>-->
            <phrase id="ini.file.location_remark">
                <p>
                    On Windows systems the file should be saved in the directory where 
                    Windows expects to find <code>.ini</code> files - e.g.
                    in the <code>C:\WINDOWS</code> or <code>C:\WINNT</code> folders.
                    <br/>
                    On Unix systems this file is expected in the user's home directory.
                <!--Unter Windows sollte die Datei dort angelegt werden, wo Windows seine <code>.ini</code>-Dateien erwartet, z.B.
                    <code>C:\WINDOWS</code> oder <code>C:\WINNT</code>.
                    <br/>
                    Unter Unix wird die Datei im Heimatverzeichnis des Benutzers erwartet.-->
                </p>
            </phrase>


            <phrase id="messages.list.title.prefix">Messages for the</phrase>
        <!--<phrase id="messages.list.title.prefix">Meldungen des Pakets</phrase>-->
            <phrase id="messages.list.title.suffix">Package</phrase>
        <!--<phrase id="messages.list.title.suffix"></phrase>-->
            <phrase id="messages.title">Messages</phrase>
        <!--<phrase id="messages.title">Meldungen</phrase>-->

            <phrase id="log_categories.table.categories.title">Categories</phrase>
        <!--<phrase id="log_categories.table.categories.title">Kategorie</phrase>-->
            <phrase id="log_categories.table.default.title">Default</phrase>
        <!--<phrase id="log_categories.table.default.title">Default</phrase>-->
            <phrase id="log_category.global_default.off"></phrase>
        <!--<phrase id="log_category.global_default.off"></phrase>-->
            <phrase id="log_category.global_default.on">default</phrase>
        <!--<phrase id="log_category.global_default.on">default</phrase>-->
            <phrase id="log_category.local_default.off">only explicit</phrase>
        <!--<phrase id="log_category.local_default.off">nur explizit</phrase>-->
            <phrase id="log_category.local_default.on">implied for</phrase>
        <!--<phrase id="log_category.local_default.on">implizit an</phrase>-->

            <phrase id="log_categories.footnotes">
                ¹) "default" means that a category is automatically allocated, unless otherwise specified.
                <br/>
                "implied" means that a category is automatically selected when its parent category is selected.
                <br/>
                "only explicit" means that the category will only be selected when it is clearly specified.
                A category marked "only explicit" will not be selected when either <code>all</code> or 
                the parent category (with or without "<code>.*</code>") is specified.
            <!--¹) "default" bedeutet, dass diese Kategorie voreingestellt ist.
                <br/>
                "implizit" bedeutet, dass diese Kategorie ausgewählt wird, wenn Sie die Mutterkategorie angeben.
                <br/>
                "nur explizit" bedeutet, dass die Kategorie explizt angegeben werden muss.
                <code>all</code> oder die Mutterkategorie mit oder ohne "<code>.*</code>" schalten dieses Kategorie nicht ein.-->
            </phrase>

            
            <phrase id="setting.initial_value">Initial value</phrase>
        <!--<phrase id="setting.initial_value">Initialwert</phrase>-->
            <phrase id="setting.subst_env">Environment variables (e.g. <code>$HOME</code>) will be replaced (see</phrase>
        <!--<phrase id="setting.subst_env">Umgebungsvariablen (z.B. <code>$HOME</code>) werden ersetzt (siehe</phrase>-->
            <phrase id="setting.subst_env.here">here</phrase>
        <!--<phrase id="setting.subst_env.here">hier</phrase>-->
            
            
            <phrase id="setting_references.api_property_has_priority=false.prefix">The</phrase>
        <!--<phrase id="setting_references.api_property_has_priority=false.prefix">Die Eigenschaft</phrase>-->
            <phrase id="setting_references.api_property_has_priority=false.suffix">property uses this setting</phrase>
        <!--<phrase id="setting_references.api_property_has_priority=false.suffix">liest die Einstellung</phrase>-->
            <phrase id="setting_references.api_property_has_priority=true.prefix">The</phrase>
        <!--<phrase id="setting_references.api_property_has_priority=true.prefix">Die Eigenschaft</phrase>-->
            <phrase id="setting_references.api_property_has_priority=true.suffix">property has precedence over this setting</phrase>
        <!--<phrase id="setting_references.api_property_has_priority=true.suffix">hat Vorrang</phrase>-->

            <phrase id="setting_references.option_has_priority=false.prefix">The</phrase>
        <!--<phrase id="setting_references.option_has_priority=false.prefix">Die Option</phrase>-->
            <phrase id="setting_references.option_has_priority=false.suffix">option is overwritten by this parameter</phrase>
        <!--<phrase id="setting_references.option_has_priority=false.suffix">wird damit überschrieben</phrase>-->
            <phrase id="setting_references.option_has_priority=true.prefix">The</phrase>
        <!--<phrase id="setting_references.option_has_priority=true.prefix">Die Option</phrase>-->
            <phrase id="setting_references.option_has_priority=true.suffix">option has precedence over this parameter</phrase>
        <!--<phrase id="setting_references.option_has_priority=true.suffix">hat Vorrang</phrase>-->

            <phrase id="setting_references.ini_has_priority=false.prefix">The</phrase>
        <!--<phrase id="setting_references.ini_has_priority=false.prefix">Die Einstellung</phrase>-->
            <phrase id="setting_references.ini_has_priority=false.suffix">setting is overwritten by this parameter</phrase>
        <!--<phrase id="setting_references.ini_has_priority=false.suffix">wird damit überschrieben</phrase>-->
            <phrase id="setting_references.ini_has_priority=true.prefix">The</phrase>
        <!--<phrase id="setting_references.ini_has_priority=true.prefix">Die Einstellung</phrase>-->
            <phrase id="setting_references.ini_has_priority=true.suffix">setting has precedence over this parameter</phrase>
        <!--<phrase id="setting_references.ini_has_priority=true.suffix">hat Vorrang</phrase>-->

            <phrase id="setting_references.xml_has_priority=false.prefix">The</phrase>
        <!--<phrase id="setting_references.xml_has_priority=false.prefix">Das XML-Attribut</phrase>-->
            <phrase id="setting_references.xml_has_priority=false.suffix">XML attribute is overwritten by this parameter</phrase>
        <!--<phrase id="setting_references.xml_has_priority=false.suffix">wird damit überschrieben</phrase>-->
            <phrase id="setting_references.xml_has_priority=true.prefix">The</phrase>
        <!--<phrase id="setting_references.xml_has_priority=true.prefix">Das XML-Attribut</phrase>-->
            <phrase id="setting_references.xml_has_priority=true.suffix">XML attribute has precedence over this parameter</phrase>
        <!--<phrase id="setting_references.xml_has_priority=true.suffix">hat Vorrang</phrase>-->

            
            <phrase id="xml.command_table.title.command">Command</phrase>
        <!--<phrase id="xml.command_table.title.command">Kommando</phrase>-->
            <phrase id="xml.command_table.title.answer">Response (included in <code>&lt;spooler>&lt;answer></code>)</phrase>
        <!--<phrase id="xml.command_table.title.answer">Antwort (eingepackt in <code>&lt;spooler>&lt;answer></code>)</phrase>-->

            
            <phrase id="xml_element.chapter_title.prefix">XML Element</phrase>
        <!--<phrase id="xml_element.chapter_title.prefix">XML-Element</phrase>-->
            <phrase id="xml_element.behaviour_with">Behavior with</phrase>
        <!--<phrase id="xml_element.behaviour_with">Verhalten mit</phrase>-->
            <phrase id="xml_element.parent_elements.title">Parent Elements</phrase>
        <!--<phrase id="xml_element.parent_elements.title">Eltern-Elemente</phrase>-->
            <phrase id="xml_element.child_elements.title">Child Elements</phrase>
        <!--<phrase id="xml_element.child_elements.title">Kind-Elemente</phrase>-->
            <phrase id="xml_element.attributes.title">Attributes</phrase>
        <!--<phrase id="xml_element.attributes.title">Attribute</phrase>-->
            <phrase id="xml_element.answer.title.prefix">Answer</phrase>
        <!--<phrase id="xml_element.answer.title.prefix">Anwort</phrase>-->
            <phrase id="xml_attributes">XML Attributes</phrase>
        <!--<phrase id="xml_attributes">XML-Attribute</phrase>-->

            <phrase id="table_of_content">Contents</phrase>
        <!--<phrase id="table_of_content">Inhalt</phrase>-->
        </phrases>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~behavior_with_xml_element-->

    <xsl:template match="behavior_with_xml_element" mode="phrase">
        <xsl:param name="element"/>
        
        <xsl:if test="@replace_attribute">
            Replaces a <xsl:copy-of select="$element"/> element
            at the appropriate position 
            with the same <code><xsl:value-of select="@replace_attribute"/>=</code> attribute 
            from the base configuration.
        <!--Ersetzt ein Element <xsl:copy-of select="$element"/>
            an der entsprechenden Stelle
            mit gleichem Attribut <code><xsl:value-of select="@replace_attribute"/>=</code>
            aus der Basiskonfiguration.-->
        </xsl:if>

        <xsl:if test="@replace='yes'">
            Replaces the <xsl:copy-of select="$element"/> element
            in the corresponding node 
            of the base XML configuration.
        <!--Ersetzt ein Element <xsl:copy-of select="$element"/>
            an der entsprechenden Stelle
            aus der Basiskonfiguration.-->
        </xsl:if>

        <xsl:if test="@complete_attribute">
            Supplements the <xsl:copy-of select="$element"/> element
            in the corresponding node 
            with the same <code><xsl:value-of select="@complete_attribute"/>=</code> attribute
            of the base XML configuration.
            Attributes specified here have precedence over those entered in the base XML configuration.
        <!--Ergänzt ein Element <xsl:copy-of select="$element"/>
            an der entsprechenden Stelle
            mit gleichem Attribut <code><xsl:value-of select="@complete_attribute"/>=</code>
            aus der Basiskonfiguration.
            Hier angegebene Attribute überschreiben die aus der Basiskonfiguration.-->
        </xsl:if>

        <xsl:if test="@complete='yes'">
            Supplements the <xsl:copy-of select="$element"/> element
            in the corresponding node 
            of the basic XML configuration.
            Attributes specified in <xsl:copy-of select="$element"/> 
            overwrite those specified in the base XML configuration.
        <!--Ergänzt ein Element <xsl:copy-of select="$element"/>
            an der entsprechenden Stelle
            aus der Basiskonfiguration.
            In <xsl:copy-of select="$element"/> 
            angegebene Attribute überschreiben die aus der Basiskonfiguration.-->
        </xsl:if>

        <xsl:if test="@allowed='no'">
            The <xsl:copy-of select="$element"/> element may not be specified here 
            when it has already been specified in the base XML configuration.
        <!--<xsl:copy-of select="$element"/> darf nicht angegeben werden, 
            wenn es bereits in der Basiskonfiguration steht.-->
        </xsl:if>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~xml_child_element-->

    <xsl:template match="xml_child_element [ @multiple='yes' ]" mode="phrase">
        <xsl:param name="element"/>

        <xsl:copy-of select="$element"/> may be repeated.
    <!--<xsl:copy-of select="$element"/> kann wiederholt werden.-->
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

</xsl:stylesheet>
