<?xml version='1.0' encoding="utf-8"?>
<!-- $Id -->

<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform" 
                version   = "1.0">

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~xml_element-->
    
    <xsl:template match="xml_element">
    
        <xsl:variable name="title">Scheduler&#160; –&#160; XML-Element &lt;<xsl:value-of select="@name"/>>&#160;  <xsl:value-of select="@title"/></xsl:variable>

        <html>
            <xsl:call-template name="html_head">
                <xsl:with-param name="title" select="$title"/>
            </xsl:call-template>
            
            <body>
                <h1>
                    <xsl:value-of select="$title"/>
                </h1>
                
                <!--<h2>Schema</h2>-->

                <code>&lt;<xsl:value-of select="@name"/></code>
                
                <xsl:if test="xml_attributes/xml_attribute">
                    <div class="indent">
                        <table cellspacing="0" cellpadding="0">
                            <col/>
                            <col/>
                            <col style="padding-left: 4ex"/>
                            <xsl:for-each select="xml_attributes/xml_attribute">
                                <tr>
                                    <td valign="baseline">
                                        <xsl:element name="a">
                                            <xsl:attribute name="href">#attribute_<xsl:value-of select="@name"/></xsl:attribute>
                                            <code><xsl:value-of select="@name"/></code>&#160;
                                        </xsl:element>
                                    </td>
                                    <td valign="baseline">
                                        <code>= "</code><span class="type"><xsl:value-of select="@value"/></span><code>"</code>
                                    </td>
                                    <td valign="baseline">
                                        <span class="title">
                                            <xsl:value-of select="@title"/>
                                        </span>
                                    </td>
                                </tr>
                            </xsl:for-each>
                        </table>
                    </div>
                </xsl:if>

                <!--
                    <table cellspacing="0" cellpadding="0">
                        <tr>
                            <td valign="top">
                                <code>&lt;<xsl:value-of select="@name"/></code>
                                <i style="color: white"> .</span> <!~~ Damit die Höhe stimmt ~~>
                            </td>
                            <td valign="baseline">
                                <table cellspacing="0" cellpadding="0">
                                    <col/>
                                    <col/>
                                    <col style="padding-left: 4ex"/>
                                    <xsl:for-each select="xml_attributes/xml_attribute">
                                        <tr>
                                            <td valign="baseline">
                                                <xsl:element name="a">
                                                    <xsl:attribute name="href">#attribute_<xsl:value-of select="@name"/></xsl:attribute>
                                                    <code><xsl:value-of select="@name"/></code>&#160;
                                                </xsl:element>
                                            </td>
                                            <td valign="baseline">
                                                <code>= "</code><span class="type"><xsl:value-of select="@value"/></span><code>"</code>
                                            </td>
                                            <td valign="baseline">
                                                <xsl:value-of select="@title"/>
                                            </td>
                                        </tr>
                                    </xsl:for-each>
                                </table>
                            </td>
                        </tr>
                    </table>
                 -->
                    
                    <xsl:choose>
                        <xsl:when test="xml_child_elements/xml_child_element">
                    
                    <code>></code>
                            
                            <div class="indent">
                                <table cellspacing="0" cellpadding="0">
                                    <col/>
                                    <col style="padding-left: 4ex"/>
                                    <xsl:for-each select="xml_child_elements/xml_child_element">
                                        <tr>
                                            <td valign="baseline">
                                                <xsl:element name="a">
                                                    <!--xsl:attribute name="href">#element_<xsl:value-of select="@name"/></xsl:attribute-->
                                                    <xsl:attribute name="href"><xsl:value-of select="@name"/>.xml</xsl:attribute>
                                                    <code>&lt;<xsl:value-of select="@name"/></code> ...<code>></code><br/>
                                                </xsl:element>
                                            </td>
                                            <td valign="baseline">
                                                <span class="title">
                                                    <xsl:value-of select="document( concat( 'xml/', @name, '.xml' ) )/xml_element[ @name=current()/@name ]/@title"/>
                                                </span>
                                            </td>
                                        </tr>
                                    </xsl:for-each>
                                </table>
                            </div>
                            
                            <br/>
                            <code>&lt;/<xsl:value-of select="@name"/>></code>
                        </xsl:when>
                        <xsl:otherwise>
                            <code>/></code>
                        </xsl:otherwise>
                    </xsl:choose>                                    
                <!--/p-->
                
                <p>&#160;</p>
                
                <xsl:apply-templates select="description"/>
                
                <xsl:for-each select="behavior_with_xml_element">
                    <p class="example">
                        Verhalten mit 
                        <xsl:call-template name="scheduler_element">
                            <xsl:with-param name="name" select="@element"/>
                        </xsl:call-template>
                    </p>

                    <div class="indent">
                        <!--xsl:if test="@allowed='yes' and /xml_element/xml_attributes/xml_attribute">
                            Attribute werden überschrieben.
                            <p/>
                        </xsl:if-->
                        <xsl:if test="@replace_attribute">
                            Ersetzt ein Element <code>&lt;<xsl:value-of select="parent::*/@name"/>></code>
                            an der entsprechenden Stelle
                            mit gleichem Attribut <code><xsl:value-of select="@replace_attribute"/>=</code>
                            aus der Basiskonfiguration.
                        </xsl:if>

                        <xsl:if test="@replace='yes'">
                            Ersetzt ein Element <code>&lt;<xsl:value-of select="parent::*/@name"/>></code>
                            an der entsprechenden Stelle
                            aus der Basiskonfiguration.
                        </xsl:if>

                        <xsl:if test="@complete_attribute">
                            Ergänzt ein Element <code>&lt;<xsl:value-of select="parent::*/@name"/>></code>
                            an der entsprechenden Stelle
                            mit gleichem Attribut <code><xsl:value-of select="@complete_attribute"/>=</code>
                            aus der Basiskonfiguration.
                            <!--Hier angegebene Attribute überschreiben die aus der Basiskonfiguration.-->
                        </xsl:if>
                        
                        <xsl:if test="@complete='yes'">
                            Ergänzt ein Element <code>&lt;<xsl:value-of select="parent::*/@name"/>></code>
                            an der entsprechenden Stelle
                            aus der Basiskonfiguration.
                            <!--Hier angegebene Attribute überschreiben die aus der Basiskonfiguration.-->
                        </xsl:if>
                        
                        <xsl:if test="@allowed='no'">
                            Das Element darf nicht angegeben werden, wenn es bereits in der Basiskonfiguration steht.
                        </xsl:if>
                        
                        <xsl:apply-templates select="description"/>
                    </div>
                </xsl:for-each>
                
                <xsl:apply-templates select="example"/>
                <xsl:apply-templates select="xml_parent_elements"/>
                <xsl:apply-templates select="xml_attributes"/>
                <xsl:apply-templates select="xml_child_elements"/>
                
                <xsl:call-template name="bottom"/>
            </body>
        </html>
    
    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~xml_parent_elements-->
    
    <xsl:template match="xml_parent_elements">
        <h2>Eltern-Elemente</h2>
        <xsl:apply-templates select="xml_parent_element"/>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~xml_child_elements-->

    <xsl:template match="xml_child_elements">
        <h2>Kind-Elemente</h2>
        <xsl:apply-templates select="xml_child_element"/>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~xml_parent_element | xml_child_element-->
        
    <xsl:template match="xml_parent_element | xml_child_element">

        <xsl:if test="self::xml_child_element">
            <xsl:element name="a">
                <xsl:attribute name="name">element_<xsl:value-of select="@name"/></xsl:attribute>
            </xsl:element>
        </xsl:if>
        
        <table cellspacing="0" cellpadding="0">
            <col valign="baseline"/>
            <tr>
                <xsl:element name="td">
                    <xsl:if test="count( ../* ) &gt; 1  and  not( ../*/description )">
                        <xsl:attribute name="width">150</xsl:attribute>
                    </xsl:if>
                    <p class="element">
                        <b>
                            <code>
                                <xsl:element name="a">
                                    <xsl:attribute name="href"><xsl:value-of select="@name"/>.xml</xsl:attribute>
                                    &lt;<xsl:value-of select="@name"/>>
                                </xsl:element>
                            </code>
                        </b>
                    </p>
                </xsl:element>
                <td valign="baseline">
                    &#160;
                    –
                    <span class="title">
                        <xsl:value-of select="document( concat( 'xml/', @name, '.xml' ) )/xml_element[ @name=current()/@name ]/@title"/>
                    </span>
                </td>
            </tr>
        </table>
            
        
        <div class="indent">
            <xsl:apply-templates select="description"/>
            
            <xsl:if test="self::xml_child_element/@multiple='yes'">
                <p>
                    <code>&lt;<xsl:value-of select="@name"/>></code> kann wiederholt werden.
                </p>
                <br/>
            </xsl:if>
        </div>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~xml_attributes-->

    <xsl:template match="xml_attributes">
        <h2>Attribute</h2>
        <xsl:apply-templates select="xml_attribute"/>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~xml_attribute-->

    <xsl:template match="xml_attribute [ @same_as_element ]">
        <xsl:apply-templates select="document( concat( 'xml/', @same_as_element, '.xml' ) )/xml_element/xml_attributes/xml_attribute[ @name=current()/@name ]"/>
    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~xml_attribute-->

    <xsl:template match="xml_attribute">
    
        <xsl:element name="a">
            <xsl:attribute name="name">attribute_<xsl:value-of select="@name"/></xsl:attribute>
        </xsl:element>
        
        <p class="list">
            <code><b><xsl:value-of select="@name"/></b>="</code>
            <span class="type"><xsl:value-of select="@value"/></span>
            <code>"</code>
            
            <xsl:if test="@initial">
                &#160;(Initialwert: <xsl:value-of select="@initial"/>)
            </xsl:if>
        </p>

        <div class="indent">
            <xsl:if test="@option">
                Kann mit der Kommandozeilen-Option
                <xsl:element name="a">
                    <xsl:attribute name="href">
                        <xsl:value-of select="/*/@base_dir"/>/command_line.xml#option_<xsl:value-of select="@option"/>
                    </xsl:attribute>
                    <code>-<xsl:value-of select="@option"/>=</code>
                </xsl:element>
                überschrieben werden.
                <p/>
            </xsl:if>
            
            <xsl:if test="@subst_env='yes'">
                Umgebungsvariablen (z.B. <code>$HOME</code>) werden ersetzt.
                <p/>
            </xsl:if>
            
            <xsl:apply-templates select="description"/>
            <xsl:apply-templates select="example"/>        
        </div>
    </xsl:template>
    

<!--    
    <xsl:template name="xml_attribute_value">
        <p><xsl:value-of select="../xml_attribute@name"/>="<xsl:value-of select="@value"/>"</p>
        
        <div class="indent">
           <xsl:copy-of select="*|text()"/>
        </div>
    </xsl:template>
-->
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~example-->

    <xsl:template match="example">
        <p class="example">Beispiel</p>
        <xsl:copy-of select="* | text()"/>
    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~description-->

    <xsl:template match="description">
        <!--<xsl:copy-of select="* | text()"/>-->
        <xsl:apply-templates select="node()" mode="description"/>
    </xsl:template>    
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~description-->
    
    <!-- Alles kopieren, außer <scheduler_element> usw. -->
    <xsl:template match="node()" mode="description">
        <xsl:copy>
            <xsl:for-each select="@*">
                <xsl:copy><xsl:value-of select="."/></xsl:copy>
            </xsl:for-each>
            <xsl:apply-templates mode="description"/>
        </xsl:copy>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~p-->
    <!-- Das erste <p> in einem <description> bekommt class="first"
         Damit fällt die Leerzeile am Anfang weg. -->
    
    <xsl:template match="p [ position()=1 and not( @class ) ]" mode="description">
        <element name="p">
            <xsl:attribute name="class">first</xsl:attribute>
            <xsl:for-each select="@*">
                <xsl:copy><xsl:value-of select="."/></xsl:copy>
            </xsl:for-each>
            <xsl:apply-templates mode="description"/>
        </element>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_element-->

    <xsl:template match="scheduler_element" mode="description">
        <xsl:call-template name="scheduler_element">
            <xsl:with-param name="name" select="@name"/>
        </xsl:call-template>
        <!--
        <xsl:element name="a">
            <xsl:attribute name="href"><xsl:value-of select="@name"/>.xml</xsl:attribute>
            <code>&lt;<xsl:value-of select="@name"/>></code>
        </xsl:element>
        -->
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_setting-->

    <xsl:template match="scheduler_setting" mode="description">
    
        <xsl:choose>
            <xsl:when test="ancestor::ini_entry">
                <code><xsl:value-of select="@name"/>=</code>
            </xsl:when>
            <xsl:when test="ancestor::command_option">
                <code>-<xsl:value-of select="@name"/>=</code>
            </xsl:when>
            <xsl:otherwise>
                <code><xsl:value-of select="@name"/>=</code>
            </xsl:otherwise>
        </xsl:choose>

        <xsl:choose>
            <xsl:when test="@value">
                <code><xsl:value-of select="@value"/></code>
            </xsl:when>
            <xsl:otherwise>
                <span class="type"><xsl:value-of select="@type"/></span>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_element-->

    <xsl:template name="scheduler_element">
        <xsl:param name="name"/>
        <xsl:choose>
            <xsl:when test="$name">
                <xsl:element name="a">
                    <xsl:attribute name="href"><xsl:value-of select="$name"/>.xml</xsl:attribute>
                    <code>&lt;<xsl:value-of select="$name"/>></code>
                </xsl:element>
            </xsl:when>
            <!--
            <xsl:otherwise>
                <code>&lt;<xsl:value-of select="/xml_element/@name"/>></code>
            </xsl:otherwise>
            -->
        </xsl:choose>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_option-->

    <xsl:template match="scheduler_option" mode="description">
        <xsl:element name="a">
            <xsl:attribute name="href">../command_line.xml#option_<xsl:value-of select="@name"/></xsl:attribute>
            <code>-<xsl:value-of select="@name"/>=</code>
        </xsl:element>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~command_line-->
    
    <xsl:template match="command_line">

        <xsl:variable name="title">Scheduler&#160; –&#160; <xsl:value-of select="@title"/></xsl:variable>

        <html>
            <xsl:call-template name="html_head">
                <xsl:with-param name="title" select="$title"/>
            </xsl:call-template>
        
            <body>
                <h1>
                    <xsl:value-of select="$title"/>
                </h1>

                <xsl:apply-templates select="command_options" mode="table"/>
                <xsl:apply-templates select="description"/>
                <xsl:apply-templates select="command_options"/>
                <xsl:call-template name="bottom"/>
            </body>
        </html>
        
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~command_options-->
    
    <xsl:template match="command_options" mode="table">
    
        <code><xsl:value-of select="parent::*/@program"/></code>
        
        <div class="indent">
            <table cellspacing="0" cellpadding="0">
                <col/>
                <col style="padding-left: 4ex"/>
                <xsl:for-each select="command_option">
                    <xsl:variable name="setting" select="document( 'settings.xml' )/settings/setting[ @name = current()/@setting ]"/>
                
                    <tr>
                        <td valign="baseline">
                            <xsl:element name="a">
                                <xsl:attribute name="href">#option_<xsl:value-of select="@name"/></xsl:attribute>
                                <code>-<xsl:value-of select="@name"/></code>
                            </xsl:element>
                            
                            <xsl:if test="@type | $setting/@type">
                                <code>=</code><span class="type"><xsl:value-of select="@type | $setting/@type"/></span>
                            </xsl:if>
                        </td>
                        <td valign="baseline">
                            <span class="title">
                                <xsl:value-of select="@title | $setting/@title"/>
                            </span>
                        </td>
                    </tr>
                </xsl:for-each>
            </table>
        </div>
        <p>&#160;</p>
    
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~command_options-->
    
    <xsl:template match="command_options">
    
        <h2>Optionen</h2>
        <xsl:apply-templates select="command_option"/>
    
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~command_option-->
    
    <xsl:template match="command_option">
    
        <xsl:element name="a">
            <xsl:attribute name="name">option_<xsl:value-of select="@name"/></xsl:attribute>
        </xsl:element>
        
        <p class="command_option">
            <b><code>-<xsl:value-of select="@name"/></code></b>
            <xsl:if test="@value">
                <b><code>=</code></b>
                <span class="type"><xsl:value-of select="@value"/></span>                    
            </xsl:if>
        </p>
    
        <div class="indent">

            <xsl:variable name="setting" select="document( 'settings.xml' )/settings/setting[ @name = current()/@setting ]"/>
            
            <xsl:apply-templates select="$setting"/>
            <p/>
            <xsl:if test="$setting">
            </xsl:if>

            <xsl:apply-templates select="description"/>

<!--            
            <xsl:if test="@xml_config_attribute">
                <p>
                    Überschreibt
                    <xsl:element name="a">
                        <xsl:attribute name="href">xml/config.xml#attribute_<xsl:value-of select="@xml_config_attribute"/></xsl:attribute>
                        <code>&lt;config <xsl:value-of select="@xml_config_attribute"/>="..."></code>
                    </xsl:element>.
                </p>
            </xsl:if>
-->            
        </div>        
    
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~html_head-->
    
    <xsl:template name="html_head">
        <xsl:param name="title"/>
        
        <head>
            <title>
                <xsl:value-of select="$title"/>
            </title>
            
            <style type="text/css">
                @import "<xsl:value-of select="/*/@base_dir"/>/scheduler.css";
            </style>
        </head>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~bottom-->

    <xsl:template name="bottom">
        <p style="margin-top: 2ex"/>
        <hr size="1"/>
        <p align="right" style="font-size: 8pt; margin-top: 0">
            Zuletzt geändert von
            <xsl:value-of select="           substring-before( substring-after( /*/@author, 'Author: ' ), ' $' )"            />,
            <xsl:value-of select="translate( substring-before( substring-after( /*/@date,   'Date: '   ), ' $' ), '/', '-' )"/> GMT
        </p>
    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ini-->

    <xsl:template match="ini">
        <xsl:variable name="title">Scheduler&#160; –&#160; <xsl:value-of select="@file"/>&#160; –&#160; <xsl:value-of select="@title"/></xsl:variable>

        <html>
            <xsl:call-template name="html_head">
                <xsl:with-param name="title" select="$title"/>
            </xsl:call-template>
        
            <body>
                <h1>
                    <xsl:value-of select="@file"/>
                </h1>

                <xsl:apply-templates select="ini_sections" mode="table"/>
                <xsl:apply-templates select="description"/>
                <xsl:apply-templates select="ini_sections"/>
                <xsl:call-template name="bottom"/>
            </body>
        </html>
    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ini_sections-->
    
    <xsl:template match="ini_sections" mode="table">

        <table cellspacing="0" cellpadding="0">
            <col valign="baseline"/>
            <col valign="baseline"/>
            <col valign="baseline" style="padding-left: 4ex"/>
            <xsl:for-each select="ini_section">
                <tr>
                    <td>
                        <xsl:element name="a">
                            <xsl:attribute name="href">#section_<xsl:value-of select="@name"/></xsl:attribute>
                            <code>[<xsl:value-of select="@name"/>]</code>
                        </xsl:element>
                    </td>
                    <td>
                        <span class="title">
                            <xsl:value-of select="@title"/>
                        </span>
                    </td>
                </tr>

                <xsl:for-each select="ini_entries/ini_entry">
                    <xsl:variable name="setting" select="document( 'settings.xml' )/settings/setting[ @name = current()/@setting ]"/>
    
                    <tr>
                        <td>
                            <xsl:element name="a">
                                <xsl:attribute name="href">#entry_<xsl:value-of select="parent::*/@setting"/>__entry_<xsl:value-of select="@name"/></xsl:attribute>
                                <code><xsl:value-of select="@name | @setting"/></code>
                            </xsl:element>
                        </td>
                        <td>
                            <code>= </code><span class="type"><xsl:value-of select="$setting/@type"/></span>
                        </td>
                        <td>
                            <span class="title">
                                <xsl:value-of select="$setting/@title"/>
                            </span>
                        </td>
                    </tr>
                </xsl:for-each>
            </xsl:for-each>
        </table>
        <p>&#160;</p>
    
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ini_section-->
    
    <xsl:template match="ini_sections">

        <xsl:for-each select="ini_section">
            <p class="ini_section">
                <xsl:element name="a">
                    <xsl:attribute name="name">section_<xsl:value-of select="@setting"/></xsl:attribute>
                </xsl:element>
                <b><code>[<xsl:value-of select="@name"/>]</code></b>
                <span class="title">
                    <xsl:value-of select="@title"/>
                </span>
            </p>

            <xsl:for-each select="ini_entries/ini_entry">
                <p class="ini_entry">
                    <xsl:variable name="setting" select="document( 'settings.xml' )/settings/setting[ @name = current()/@setting ]"/>

                    <xsl:element name="a">
                        <xsl:attribute name="name">entry_<xsl:value-of select="parent::*/@setting"/>__entry_<xsl:value-of select="@setting"/></xsl:attribute>
                    </xsl:element>
                    <b><code><xsl:value-of select="@setting"/></code></b>
                    
                    <b><code> = </code></b>
                    <span class="type"><xsl:value-of select="$setting/@type"/></span>

                    &#160;
                    –
                    <span class="title">
                        <xsl:value-of select="$setting/@title"/>
                    </span>
                </p>
                
                <div class="indent">
                    <xsl:apply-templates select="$setting/description"/>
                </div>
            </xsl:for-each>
        </xsl:for-each>
    
    </xsl:template>

    
</xsl:stylesheet>
