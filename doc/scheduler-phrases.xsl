<?xml version="1.0"?>
<!-- $Id$ -->
<!-- Texte für XSLT-Stylesheets der Dokumentation: scheduler.xsl, scheduler_base.xsl, api/api.xsl -->

<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform"
                version   = "1.0">

    <xsl:variable name="lang" select="'de'"/>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~PHRASES-->

    <xsl:template name="this_is_a_container_only_for_phrases">
        
        <phrases style="">     <!-- style nur zum Debuggen -->

            <phrase id="api.title">Programmschnittstelle (API)</phrase>
            <phrase id="api.overview.title">Übersicht</phrase>
            <phrase id="api.method.deprecated">Der Aufruf sollte nicht mehr verwendet werden</phrase>
            <phrase id="api.method.parameters.title">Parameter</phrase>
            <phrase id="api.method.return.title">Rückgabe</phrase>
            <phrase id="api.exceptions.title">Exceptions</phrase>
            <!--
            <phrase id="api.method.not_for_unix_perl">Nicht für Perl unter Unix</phrase>
            <phrase id="api.method.restricted_for_unix_perl">Für Perl unter Unix sind Objekt- und Array-Parameter nicht verwendbar</phrase>
            <phrase id="api.method.not_for_unix_perl.detailed">Dieser Aufruf steht in Perl unter Unix nicht zur Verfügung.</phrase>
            <phrase id="api.method.restricted_for_unix_perl.detailed">Für Perl unter Unix sind Objekt- und Array-Parameter nicht verwendbar.</phrase>
            <phrase id="api.parameter.not_for_unix_perl">Der Parameter kann in Perl unter Unix nicht gesetzt werden.</phrase>
            <phrase id="api.parameter.restricted_for_unix_perl">Für Perl unter Unix sind Objekt- und Array-Parameter nicht verwendbar.</phrase>
            -->

            <phrase id="command_line.variants.title">Aufrufvarianten</phrase>
            <phrase id="command_line.options.title">Optionen</phrase>
            <phrase id="command_line.option.multiple">Die Option kann mehrfach angegeben werden.</phrase>

            
            <phrase id="common.in_programming_language">in</phrase>
            <phrase id="common.see">siehe</phrase>
            <phrase id="common.See">Siehe</phrase>


            <phrase id="example.title">Beispiel</phrase>

            
            <!-- Kopf- und Fußzeilen der HTML-Seiten -->
            <phrase id="head.link_to_first_page">Erste Seite</phrase>
            <phrase id="head.link_to_api">API</phrase>
            <phrase id="head.link_to_xml">XML</phrase>
            <phrase id="head.link_to_index">Register</phrase>
            <phrase id="head.link_to_sos">Software- und Organisations-Service GmbH</phrase>
            <phrase id="head.last_updated_by">Zuletzt geändert von</phrase>

            
            <phrase id="ini.file.title">Die Datei</phrase>
            <phrase id="ini.sections.title">Die Abschnitte</phrase>
            <phrase id="ini.section">Abschnitt</phrase>
            <phrase id="ini.entry">Eintrag</phrase>
            <phrase id="ini.file.location_remark">
                <p>
                    Unter Windows wird sollte die Datei dort angelegt werden, wo Windows seine <code>.ini</code>-Dateien erwartet.
                    Das ist normalerweise im Verzeichnis <code>c:\windows</code>.
                    Sonst wird die Datei im Heimverzeichnis des Benutzers erwartet.
                </p>
            </phrase>


            <phrase id="messages.title">Meldungen</phrase>

            <phrase id="log_categories.table.categories.title">Kategorie</phrase>
            <phrase id="log_categories.table.default.title">Default</phrase>

            <phrase id="log_categories.footnotes">
                ¹) "default" bedeutet, dass diese Kategorie voreingestellt ist.
                <br/>
                "implizit" bedeutet, dass diese Kategorie ausgewählt wird, wenn Sie die Mutterkategorie angeben.
                <br/>
                "nur explizit" bedeutet, dass die Kategorie explizt angegeben werden muss.
                <code>all</code> oder die Mutterkategorie mit oder ohne "<code>.*</code>" schalten dieses Kategorie nicht ein.
            </phrase>

            
            <phrase id="setting.initial_value">Initialwert</phrase>
            <phrase id="setting.subst_env">Umgebungsvariablen (z.B. <code>$HOME</code>) werden ersetzt (siehe</phrase>
            <phrase id="setting.subst_env.here">hier</phrase>
            
            
            <phrase id="setting_references.api_property_has_priority=false.prefix">Die Eigenschaft</phrase>
            <phrase id="setting_references.api_property_has_priority=false.suffix">liest die Einstellung</phrase>
            <phrase id="setting_references.api_property_has_priority=true.prefix">Die Eigenschaft</phrase>
            <phrase id="setting_references.api_property_has_priority=true.suffix">hat Vorrang</phrase>

            <phrase id="setting_references.option_has_priority=false.prefix">Die Option</phrase>
            <phrase id="setting_references.option_has_priority=false.suffix">wird damit überschrieben</phrase>
            <phrase id="setting_references.option_has_priority=true.prefix">Die Option</phrase>
            <phrase id="setting_references.option_has_priority=true.suffix">hat Vorrang</phrase>

            <phrase id="setting_references.ini_has_priority=false.prefix">Die Einstellung</phrase>
            <phrase id="setting_references.ini_has_priority=false.suffix">wird damit überschrieben</phrase>
            <phrase id="setting_references.ini_has_priority=true.prefix">Die Einstellung</phrase>
            <phrase id="setting_references.ini_has_priority=true.suffix">hat Vorrang</phrase>

            <phrase id="setting_references.xml_has_priority=false.prefix">Das XML-Attribut</phrase>
            <phrase id="setting_references.xml_has_priority=false.suffix">wird damit überschrieben</phrase>
            <phrase id="setting_references.xml_has_priority=true.prefix">Das XML-Attribut</phrase>
            <phrase id="setting_references.xml_has_priority=true.suffix">hat Vorrang</phrase>

            
            <phrase id="xml.command_table.title.command">Kommando</phrase>
            <phrase id="xml.command_table.title.answer">Antwort (eingepackt in <code>&lt;spooler>&lt;answer></code>)</phrase>

            
            <phrase id="xml_element.chapter_title.prefix">XML-Element</phrase>
            <phrase id="xml_element.behaviour_with">Verhalten mit</phrase>
            <phrase id="xml_element.parent_elements.title">Eltern-Elemente</phrase>
            <phrase id="xml_element.child_elements.title">Kind-Elemente</phrase>
            <phrase id="xml_element.attributes.title">Attribute</phrase>
            <phrase id="xml_element.answer.title.prefix">Anwort</phrase>

        </phrases>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~behavior_with_xml_element-->

    <xsl:template match="behavior_with_xml_element" mode="phrase">
        <xsl:param name="element"/>
        
        <xsl:if test="@replace_attribute">
            Ersetzt ein Element <xsl:copy-of select="$element"/>
            an der entsprechenden Stelle
            mit gleichem Attribut <code><xsl:value-of select="@replace_attribute"/>=</code>
            aus der Basiskonfiguration.
        </xsl:if>

        <xsl:if test="@replace='yes'">
            Ersetzt ein Element <xsl:copy-of select="$element"/>
            an der entsprechenden Stelle
            aus der Basiskonfiguration.
        </xsl:if>

        <xsl:if test="@complete_attribute">
            Ergänzt ein Element <xsl:copy-of select="$element"/>
            an der entsprechenden Stelle
            mit gleichem Attribut <code><xsl:value-of select="@complete_attribute"/>=</code>
            aus der Basiskonfiguration.
            Hier angegebene Attribute überschreiben die aus der Basiskonfiguration.
        </xsl:if>

        <xsl:if test="@complete='yes'">
            Ergänzt ein Element <xsl:copy-of select="$element"/>
            an der entsprechenden Stelle
            aus der Basiskonfiguration.
            In <xsl:copy-of select="$element"/> 
            angegebene Attribute überschreiben die aus der Basiskonfiguration.
        </xsl:if>

        <xsl:if test="@allowed='no'">
            <xsl:copy-of select="$element"/> darf nicht angegeben werden, 
            wenn es bereits in der Basiskonfiguration steht.
        </xsl:if>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~xml_child_element-->

    <xsl:template match="xml_child_element [ @multiple='yes' ]" mode="phrase">
        <xsl:param name="element"/>

        <xsl:copy-of select="$element"/> kann wiederholt werden.
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

</xsl:stylesheet>
