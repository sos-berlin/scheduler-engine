<?xml version='1.0' encoding="utf-8"?>
<!-- $Id$ -->

<!--
    Änderungswünsche:
    
    @same_as_element sollte durch setting ersetzt werden.
    Zentrale Einstellung nur noch in settings.xml dokumentieren.
-->


<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform" 
                version   = "1.0">
                
    <xsl:output doctype-public="-//W3C//DTD HTML 4.01//EN" />  <!--"http://www.w3.org/TR/html4/strict.dtd"-->


    <!-- Nicht für Firefox 1.0.6:
    <xsl:output method="xml" 
                media-type="application/xhtml+xml" 
                doctype-public="-//W3C//DTD XHTML 1.0 Strict//EN" 
                doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd"/-->
    <!--xsl:output doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"/> <!- - "http://www.w3.org/TR/html4/loose.dtd"-->
    
    <xsl:variable name="start_page" select="'index.xml'"/>
    <xsl:variable name="base_dir"   select="/*/@base_dir"/>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~xml_commands-->
    <!-- Für xml_commands.xml -->
    
    <xsl:template match="xml_commands">
    
        <xsl:variable name="title" select="@title"/>

        <html>
            <xsl:call-template name="html_head">
                <xsl:with-param name="title" select="$title"/>
            </xsl:call-template>

            
            <body>
                <xsl:call-template name="body_start">
                    <xsl:with-param name="title"       select="$title"/>
                    <xsl:with-param name="parent_page" select="@parent_page"/>
                </xsl:call-template>
                
                <xsl:apply-templates select="description"/>
                <xsl:apply-templates select="scheduler_commands"/>
                
                <xsl:call-template name="bottom">
                    <xsl:with-param name="parent_page" select="@parent_page"/>
                </xsl:call-template>
            </body>
        </html>
    
    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_commands-->
    <!-- Für xml_commands.xml -->

    <xsl:template match="scheduler_commands">
        <table cellspacing="0" cellpadding="0">
            <col valign="baseline"/>
            <col valign="baseline" style="padding-top: 1ex; padding-left: 2ex"/>
            <thead>
                <tr>
                    <td>Kommando</td>
                    <td>Antwort (eingepackt in <code>&lt;spooler>&lt;answer></code>)</td>
                </tr>
                <tr>
                    <td colspan="2">
                        <hr/>
                    </td>
                </tr>
            </thead>
            <tbody>
                <xsl:for-each select="scheduler_command">
                    <xsl:sort select="@name"/>
                    <tr>
                        <td>
                            <xsl:call-template name="scheduler_element">
                                <xsl:with-param name="name" select="@name"/>
                            </xsl:call-template>
                        </td>
                        <td>
                            <xsl:call-template name="scheduler_element">
                                <xsl:with-param name="directory" select="'xml/answer/'"/>
                                <xsl:with-param name="name" select="@answer"/>
                            </xsl:call-template>
                        </td>
                    </tr>
                </xsl:for-each>
            </tbody>
        </table>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~xml_element-->
    
    <xsl:template match="xml_element">
    
        <xsl:variable name="title">XML-Element&#160; &lt;<xsl:value-of select="@name"/>><!-- &#160; – &#160; <xsl:value-of select="@title"/>--><xsl:if test="@category">&#160; &#160; (<xsl:value-of select="@category"/>)</xsl:if></xsl:variable>

        <html>
            <xsl:call-template name="html_head">
                <xsl:with-param name="title" select="$title"/>
            </xsl:call-template>

            
            <body>
                <xsl:call-template name="body_start">
                    <xsl:with-param name="title"       select="$title"/>
                    <xsl:with-param name="parent_page" select="@parent_page"/>
                </xsl:call-template>
                
                <!--<h2>Schema</h2>-->

                <code>&lt;<xsl:value-of select="@name"/></code>
                
                <xsl:if test="xml_attributes/xml_attribute">
                    <div class="indent">
                        <table cellspacing="0" cellpadding="0">
                            <col/>
                            <col/>
                            <col style="padding-left: 4ex"/>
            
                            <xsl:for-each select="xml_attributes/xml_attribute">
                                <xsl:sort select="@name | @setting"/>

                                <xsl:variable name="setting" select="document( 'settings.xml' )/settings/setting[ @setting = current()/@setting ]"/>
                    
                                <tr>
                                    <td valign="baseline">
                                        <xsl:element name="a">
                                            <xsl:attribute name="class">silent</xsl:attribute>
                                            <xsl:attribute name="href">#attribute_<xsl:value-of select="@name | @setting"/></xsl:attribute>
                                            <code><xsl:value-of select="@name | @setting"/></code>&#160;
                                        </xsl:element>
                                    </td>
                                    <td valign="baseline">
                                        <code>= "</code><span class="type"><xsl:value-of select="@type | $setting/@type"/></span><code>"</code>
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
                </xsl:if>
                    
                <xsl:choose>
                    <xsl:when test="xml_child_elements/xml_child_element">
                
                        <code>></code>
                        
                        <div class="indent">
                            <table cellspacing="0" cellpadding="0">
                                <col/>
                                <col style="padding-left: 4ex"/>
                                <xsl:for-each select="xml_child_elements/xml_child_element">
                                
                                    <xsl:variable name="element" select="document( concat( 'xml/', /*/@sub_directory, @name, '.xml' ) )/xml_element[ @name=current()/@name ]"/>
                                
                                    <tr>
                                        <td valign="baseline">
                                            <xsl:element name="a">
                                                <xsl:attribute name="class">silent</xsl:attribute>
                                                <!--xsl:attribute name="href">#element_<xsl:value-of select="@name"/></xsl:attribute-->
                                                <xsl:attribute name="href"><xsl:value-of select="@name"/>.xml</xsl:attribute>
                                                <code>&lt;<xsl:value-of select="@name"/></code> ...<code>></code><br/>
                                            </xsl:element>
                                        </td>
                                        <td valign="baseline">
                                            <span class="title">
                                                <xsl:value-of select="@title | $element/@title"/>
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
                
                <p>&#160;</p>
                
                <xsl:apply-templates select="description"/>
                <xsl:apply-templates select="example"/>
                
                <xsl:for-each select="behavior_with_xml_element">
                    <h2>
                        Verhalten mit&#160;
                        <xsl:call-template name="scheduler_element">
                            <xsl:with-param name="name" select="@element"/>
                        </xsl:call-template>
                    </h2>

                    <div class="indent">
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
                            Hier angegebene Attribute überschreiben die aus der Basiskonfiguration.
                        </xsl:if>
                        
                        <xsl:if test="@complete='yes'">
                            Ergänzt ein Element <code>&lt;<xsl:value-of select="parent::*/@name"/>></code>
                            an der entsprechenden Stelle
                            aus der Basiskonfiguration.
                            Hier angegebene Attribute überschreiben die aus der Basiskonfiguration.
                        </xsl:if>
                        
                        <xsl:if test="@allowed='no'">
                            Das Element darf nicht angegeben werden, wenn es bereits in der Basiskonfiguration steht.
                        </xsl:if>
                        
                        <xsl:apply-templates select="description"/>
                    </div>
                </xsl:for-each>
                
                <xsl:apply-templates select="xml_parent_elements"/>
                <xsl:apply-templates select="xml_attributes"/>
                <xsl:apply-templates select="xml_child_elements"/>
                <xsl:apply-templates select="xml_answer"/>
                <!--xsl:call-template name="xml_answer"/-->
                
                <xsl:call-template name="bottom">
                    <xsl:with-param name="parent_page" select="@parent_page"/>
                </xsl:call-template>
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

        <xsl:variable name="element" select="document( concat( 'xml/', /*/@sub_directory, @name, '.xml' ) )/xml_element[ @name=current()/@name ]"/>

        <xsl:if test="self::xml_child_element">
            <xsl:element name="a">
                <xsl:attribute name="name">element_<xsl:value-of select="@name"/></xsl:attribute>
            </xsl:element>
        </xsl:if>
        
        <table cellspacing="0" cellpadding="0">
            <col valign="baseline"/>
            <col valign="baseline"/>
            <tr>
                <xsl:element name="td">
                    <xsl:if test="count( ../* ) &gt; 1  and  not( ../*/description )">
                        <xsl:attribute name="style">width: 150px</xsl:attribute>
                    </xsl:if>
                    <p class="xml_element">
                        <b>
                            <code>
                                <xsl:element name="a">
                                    <xsl:attribute name="class">silent</xsl:attribute>
                                    <xsl:attribute name="href"><xsl:value-of select="@name"/>.xml</xsl:attribute>
                                    &lt;<xsl:value-of select="@name"/>>
                                </xsl:element>
                            </code>
                        </b>
                    </p>
                </xsl:element>
                <td>
                    <p>
                    &#160;
                    –
                    <span class="title">
                        <xsl:value-of select="@title | $element/@title"/>
                    </span>
                    </p>
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
    
        <xsl:variable name="setting" select="document( 'settings.xml' )/settings/setting[ @setting = current()/@setting ]"/>
        
        <xsl:element name="a">
            <xsl:attribute name="name">attribute_<xsl:value-of select="@name | @setting"/></xsl:attribute>
        </xsl:element>
        
        <p class="xml_attribute">
            <code><b><xsl:value-of select="@name | @setting"/></b>="</code>
            <span class="type"><xsl:value-of select="@type | $setting/@type"/></span>
            <code>"</code>

            &#160;
            <xsl:apply-templates select="." mode="setting_header_rest"/>            
        </p>

        <xsl:apply-templates mode="setting_description" select="."/>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~xml_answer-->

    <xsl:template match="xml_answer">
    
        <h2>
            Antwort&#160; &lt;<xsl:value-of select="@element"/>>
        </h2>

        <p>
            <code>
                <xsl:call-template name="scheduler_element">
                    <xsl:with-param name="directory" select="'xml/answer/'"/>
                    <xsl:with-param name="name"      select="'spooler'"/>
                </xsl:call-template>
            </code>
            <br/>
            <code>&#160; &#160;</code>

            <code>
                <xsl:call-template name="scheduler_element">
                    <xsl:with-param name="directory" select="'xml/answer/'"/>
                    <xsl:with-param name="name"      select="'answer'"/>
                </xsl:call-template>
            </code>        
            <br/>
            <code>&#160; &#160; &#160; &#160;</code>

            <code>
                <xsl:call-template name="scheduler_element">
                    <xsl:with-param name="directory" select="'xml/answer/'"/>
                    <xsl:with-param name="name"      select="@element"/>
                    <xsl:with-param name="parameter" select="'…'"/>
                </xsl:call-template>
            </code>        
            <br/>
            <code>&#160; &#160;</code>
            
            <code>&lt;/answer></code>
            <br/>
            <code>&lt;/spooler></code>
        </p>
        
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~log_categories-->

    <xsl:template match="log_categories">
    
        <xsl:variable name="title" select="@title"/>

        <html>
            <xsl:call-template name="html_head">
                <xsl:with-param name="title" select="$title"/>
            </xsl:call-template>

            
            <body>
                <xsl:call-template name="body_start">
                    <xsl:with-param name="title"       select="$title"/>
                    <xsl:with-param name="parent_page" select="@parent_page"/>
                </xsl:call-template>
                
                <xsl:apply-templates select="description"/>
                
                <p>
                    <table cellspacing="0" cellpadding="0">
                        <col valign="baseline"/>
                        <col valign="baseline" style="padding-left: 2ex"/>
                        <col valign="baseline" style="padding-left: 2ex"/>
                        
                        <thead>
                            <tr>
                                <td>
                                    Kategorie
                                </td>
                                <td>
                                    Default¹
                                </td>
                            </tr>
                            <tr>
                                <td colspan="3">
                                    <hr/>
                                </td>
                            </tr>
                        </thead>
                        
                        <tbody>
                            <xsl:apply-templates select="log_category">
                                <xsl:sort select="@name"/>
                            </xsl:apply-templates>
                        </tbody>
                    </table>                
                </p>
                
                <p class="small">
                    ¹) "default" bedeutet, dass diese Kategorie voreingestellt ist.
                    <br/>
                    "implizit" bedeutet, dass diese Kategorie ausgewählt wird, wenn Sie die Mutterkategorie angeben.
                    <br/>
                    "nur explizit" bedeutet, dass die Kategorie explizt angegeben werden muss.
                    <code>all</code> oder die Mutterkategorie mit oder ohne "<code>.*</code>" schalten dieses Kategorie nicht ein.
                </p>
                
                <xsl:call-template name="bottom">
                    <xsl:with-param name="parent_page" select="@parent_page"/>
                </xsl:call-template>
            </body>
        </html>
        
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~log_category-->

    <xsl:template match="log_category">
        
        <xsl:param name="indent"/>

        <tr>
            <td>    
                <code>
                    <xsl:if test="parent::*/@name">
                        <xsl:value-of select="parent::*/@name"/>
                        <xsl:text>.</xsl:text>
                    </xsl:if>
                    
                    <xsl:element name="span">
                        <xsl:if test="not( parent::log_category )">
                            <xsl:attribute name="style">font-weight: bold</xsl:attribute>
                        </xsl:if>
                        <xsl:value-of select="@name"/>
                    </xsl:element>
                </code>
            </td>
            
            <td>
                <xsl:if test="@global_default='off'">
                </xsl:if>
                <xsl:if test="@global_default='on'">
                    <xsl:text>default</xsl:text>
                </xsl:if>
                <xsl:if test="@local_default='off'">
                    <xsl:text>nur explizit</xsl:text>
                </xsl:if>
                <xsl:if test="@local_default='on'">
                    <xsl:text>implizit an</xsl:text>
                </xsl:if>
            </td>

            <td>
                <xsl:value-of select="@title"/>
                <xsl:apply-templates select="description"/>
            </td>
        </tr>

        <xsl:apply-templates select="log_category">
            <xsl:with-param name="indent" select="concat( $indent, '&#160;&#160;&#160;&#160;' )"/>
            <xsl:sort select="@name"/>
        </xsl:apply-templates>

        <xsl:if test="not( parent::log_category )">
            <tr>
                <td>
                    &#160;
                </td>
            </tr>
        </xsl:if>
        
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~setting_header_rest-->

    <xsl:template match="*" mode="setting_header_rest">

        <xsl:variable name="setting" select="document( 'settings.xml' )/settings/setting[ @setting = current()/@setting ]"/>
        
        <xsl:if test="@initial | $setting/@initial">
            (Initialwert: <code><xsl:value-of select="@initial | $setting/@initial"/></code>)
            &#160; &#160;
        </xsl:if>

        <span class="title">
            <xsl:value-of select="@title | $setting/@title"/>
        </span>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~mode=setting_description-->
    
    <xsl:template match="*" mode="setting_description">
        <xsl:variable name="setting" select="document( 'settings.xml' )/settings/setting[ @setting = current()/@setting ]"/>
        
        <div class="indent">

            <xsl:if test="$setting/description">
                <xsl:apply-templates select="$setting/description"/>
                <p/>
            </xsl:if>

            <xsl:if test="generate-id( $setting/description ) != generate-id( description )">
                <xsl:apply-templates select="description"/>
            </xsl:if>

            <xsl:if test="@subst_env='yes' or $setting/@subst_env='yes'">
                <p>
                    Umgebungsvariablen (z.B. <code>$HOME</code>) werden ersetzt
                    (s.
                    <xsl:element name="a">
                        <xsl:attribute name="class">silent</xsl:attribute>
                        <xsl:attribute name="href"><xsl:value-of select="$base_dir"/>ersetzung_von_umgebungsvariablen.xml</xsl:attribute>
                        <xsl:text>hier</xsl:text>
                    </xsl:element>).
                </p>
            </xsl:if>
            
            <xsl:apply-templates select="." mode="setting_references"/>
            <xsl:apply-templates select="$setting/example"/>
            <xsl:apply-templates select="example"/>
        </div>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~mode=setting_references-->
    
    <xsl:template match="*" mode="setting_references">
        <!-- Verweise auf andere Stellen, wo der Wert eingestellt werden kann: -->
        
        <p>
            <xsl:if test="not( self::command_option )">
                <xsl:variable name="command_option" select="document( 'command_line.xml' )/command_line/command_options/command_option[ @setting = current()/@setting ]"/>
                <xsl:if test="$command_option">
                    <!--Die Einstellung-->
                    Die Option

                    <xsl:call-template name="scheduler_option">
                        <xsl:with-param name="name" select="$command_option/@name"/>
                    </xsl:call-template>

                    hat Vorrang.
                    <br/>
                </xsl:if>
            </xsl:if>

            <xsl:if test="not( self::ini_entry )">
                <xsl:variable name="ini_entry" select="( document( 'factory_ini_job.xml' ) | document( 'factory_ini_spooler.xml' ) )/ini_section/ini_entries/ini_entry[ @setting = current()/@setting ]"/>
                <xsl:variable name="current_setting" select="."/>

                <xsl:for-each select="$ini_entry">
                    <!--Die Einstellung-->

                    <xsl:call-template name="scheduler_ini_entry">
                        <xsl:with-param name="file"    select="'factory.ini'"/>
                        <xsl:with-param name="section" select="ancestor::ini_section/@name"/>
                        <xsl:with-param name="entry"   select="@name | @setting"/>
                    </xsl:call-template>

                    <xsl:choose>
                        <xsl:when test="$current_setting/self::xml_attribute">
                            hat Vorrang.
                        </xsl:when>
                        <xsl:when test="$current_setting/self::command_option">
                            wird damit überschrieben.
                        </xsl:when>
                    </xsl:choose>
                    <br/>
                </xsl:for-each>
            </xsl:if>
            
            <xsl:if test="not( self::xml_attribute )">
                <xsl:variable name="config_attribute" select="document( 'xml/config.xml' )/xml_element/xml_attributes/xml_attribute[ @setting = current()/@setting ]"/>
                <xsl:variable name="current_setting" select="."/>

                <xsl:if test="$config_attribute">
                    <!--Die Einstellung-->

                    <xsl:call-template name="scheduler_element">
                        <xsl:with-param name="name"      select="'config'"/>
                        <xsl:with-param name="attribute" select="$config_attribute/@name | $config_attribute/@setting"/>
                    </xsl:call-template>

                    <xsl:choose>
                        <xsl:when test="$current_setting/self::ini_entry or $current_setting/self::command_option">
                            wird damit überschrieben.
                        </xsl:when>
                    </xsl:choose>
                    <br/>
                </xsl:if>
            </xsl:if>
        </p>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~example-->

    <xsl:template match="example">
        <xsl:param name="programming_language"/>
        <h3>
            Beispiel
            
            <xsl:if test="$programming_language">
                <span style="font-weight: normal">
                    in <xsl:value-of select="$programming_language"/>
                </span>
            </xsl:if>
        </h3>
        
        <xsl:copy-of select="* | text()"/>
    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/register-->
    
    <xsl:template match="/register">

        <xsl:variable name="title"       select="'Register'"/>

        <html>
            <xsl:call-template name="html_head">
                <xsl:with-param name="title" select="$title"/>
            </xsl:call-template>
        
            <body>
                <xsl:call-template name="body_start">
                    <xsl:with-param name="title"       select="$title"/>
                </xsl:call-template>
                
                <xsl:call-template name="complete_register"/>
<!--                
                <p>
                    <b>Inhalt</b>
                </p>

                <p>
                    <a href="#stichwörter">Stichwörter</a><br/>
                    <a href="#optionen">Optionen</a><br/>
                    <a href="#ini">Einträge in .ini-Dateien</a><br/>
                    <a href="#xml_elemente">XML-Elemente</a><br/>
                </p>

                <a name="stichwörter"/>
                <h2>Stichwörter</h2>
                <xsl:call-template name="register">
                    <xsl:with-param name="children_name" select="'register_entry'"/>
                </xsl:call-template>
                
                <a name="optionen"/>
                <h2>Optionen der Kommandozeile</h2>
                <xsl:call-template name="register">
                    <xsl:with-param name="children_name" select="'register_option'"/>
                </xsl:call-template>
                
                <a name="ini"/>
                <h2>Einträge in .ini-Dateien</h2>
                <xsl:call-template name="register">
                    <xsl:with-param name="children_name" select="'register_ini_entry'"/>
                </xsl:call-template>
                
                <a name="xml_elemente"/>
                <h2>XML-Elemente</h2>
                <xsl:call-template name="register">
                    <xsl:with-param name="children_name" select="'register_element'"/>
                </xsl:call-template>
-->                
                
                <xsl:call-template name="bottom"/>
            </body>
        </html>
        
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~complete_register-->

    <xsl:template name="complete_register">

        <p>
            <a href="#letter__a">A&#160;</a>
            <a href="#letter__b">B&#160;</a>
            <a href="#letter__c">C&#160;</a>
            <a href="#letter__d">D&#160;</a>
            <a href="#letter__e">E&#160;</a>
            <a href="#letter__f">F&#160;</a>
            <a href="#letter__g">G&#160;</a>
            <a href="#letter__h">H&#160;</a>
            <a href="#letter__i">I&#160;</a>
            <a href="#letter__j">J&#160;</a>
            <a href="#letter__k">K&#160;</a>
            <a href="#letter__l">L&#160;</a>
            <a href="#letter__m">M&#160;</a>
            <a href="#letter__n">N&#160;</a>
            <a href="#letter__o">O&#160;</a>
            <a href="#letter__p">P&#160;</a>
            <a href="#letter__q">Q&#160;</a>
            <a href="#letter__r">R&#160;</a>
            <a href="#letter__s">S&#160;</a>
            <a href="#letter__t">T&#160;</a>
            <a href="#letter__u">U&#160;</a>
            <a href="#letter__v">V&#160;</a>
            <a href="#letter__w">W&#160;</a>
            <a href="#letter__x">X&#160;</a>
            <a href="#letter__y">Y&#160;</a>
            <a href="#letter__z">Z&#160;</a>
        </p>

        <p>&#160;</p>
        
        <table cellspacing="0" cellpadding="0">
            <col valign="baseline"/>
            <col valign="baseline" style="padding-top: 0pt; padding-left: 2ex"/>

            <xsl:for-each select="register_keyword">
                <!--xsl:sort select="@keyword"/   Nicht hier sortieren, sonst funktioniert preceding-sibling:self nicht. -->

                <xsl:variable name="first_letter" select="translate( substring( @keyword, 1, 1 ), 'abcdefghijklmnopqrstuvwxyz', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ' )"/>
                
                <xsl:if test="$first_letter != translate( substring( (preceding-sibling::*)[ position() = last() ]/@keyword, 1, 1 ), 'abcdefghijklmnopqrstuvwxyz', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ' )">
                    <tr><td colspan="9">
                        <xsl:element name="a">
                            <xsl:attribute name="name">letter__<xsl:value-of select="$first_letter"/></xsl:attribute>
                        </xsl:element>
                        <xsl:if test="position() > 1">
                            &#160;<br/><!--<hr size="1" style="color: #f0f0f0"/>-->
                        </xsl:if>
                        <b><xsl:value-of select="$first_letter"/></b>
                    </td></tr>
                </xsl:if>
                
                <tr>
                    <td style="white-space: nowrap">
                        <xsl:choose>
                            <xsl:when test="register_keyword_display">
                                <xsl:apply-templates select="register_keyword_display/*" mode="description"/>
                            </xsl:when>
                            <xsl:otherwise>
                                <xsl:value-of select="@keyword"/>
                            </xsl:otherwise>
                        </xsl:choose>
                        <!--xsl:apply-templates select="*[ name(.) = $children_name ][ position() = 1]/.." mode="keyword"/-->
                    </td>
                    <td>
                        <xsl:for-each select="register_entry | register_ini_entry">
                            <xsl:sort select="@type" order="descending"/>
                            <xsl:sort select="@register_title"/>

                            <xsl:apply-templates select=".">
                                <xsl:sort select="@register_keyword"/>
                                <xsl:sort select="@register_title"/>
                            </xsl:apply-templates>
                                        
                            <xsl:if test="position() &lt; last()">,&#160; </xsl:if>
                        </xsl:for-each>
                    </td>
                </tr>
            </xsl:for-each>
        </table>                

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~register-->

    <xsl:template name="register">
        <xsl:param name="children_name"/>
        
        <table cellspacing="0" cellpadding="0">
            <col valign="baseline"/>
            <col valign="baseline" style="padding-top: 2pt; padding-left: 2ex"/>

            <xsl:for-each select="*[ name(./*) = $children_name ]">
                <xsl:sort select="@keyword"/>
                
                <tr>
                    <td style="white-space: nowrap">
                        <xsl:value-of select="@keyword"/>
                        <!--xsl:apply-templates select="*[ name(.) = $children_name ][ position() = 1]/.." mode="keyword"/-->
                    </td>
                    <td>
                        <xsl:for-each select="*[ name(.) = $children_name ]">
                            <xsl:sort select="@register_title"/>
                            
                            <xsl:apply-templates select=".">
                                <xsl:sort select="@register_keyword"/>
                                <xsl:sort select="@register_title"/>
                            </xsl:apply-templates>
                                        
                            <xsl:if test="position() &lt; last()">,&#160; </xsl:if>
                        </xsl:for-each>
                    </td>
                </tr>
            </xsl:for-each>
        </table>                

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~register_entry-->
    <!--
    <xsl:template match="*" mode="keyword">

    **    <xsl:value-of select="@keyword"/>

    </xsl:template>
    -->
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~register_entry-->
    
    <xsl:template match="register_entry">

        <span style="white-space: nowrap">
            <xsl:element name="a">
                <xsl:choose>
                    <xsl:when test="contains( @register_file, '#' )">
                        <xsl:attribute name="href"><xsl:value-of select="@register_file"/></xsl:attribute>
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:attribute name="href"><xsl:value-of select="concat( @register_file, '#keyword__', @register_keyword )"/></xsl:attribute>
                    </xsl:otherwise>
                </xsl:choose>
                
                <xsl:apply-templates select="." mode="register_title"/>
            </xsl:element>
        </span>
        
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~register_ini_entry-->
    
    <xsl:template match="register_ini_entry">

        <span style="white-space: nowrap">
            <xsl:value-of select="@file"/>
            <xsl:text> [</xsl:text>
            <xsl:value-of select="@section"/>
            <xsl:text>]:&#160;</xsl:text>

            <xsl:element name="a">
                <xsl:attribute name="href"><xsl:value-of select="concat( @register_file, '#use_entry__', @file, '__', @section, '__', @entry )"/></xsl:attribute>
                <xsl:apply-templates select="." mode="register_title"/>
            </xsl:element>
        </span>
        
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~register_entry-->
    <!--    
    <xsl:template match="* [ child::register_option ]" mode="keyword">

        -<xsl:value-of select="@keyword"/>=

    </xsl:template>
    -->
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~register_option-->
    <!--
    <xsl:template match="register_option">
        
        Option:
        <xsl:element name="a">
            <xsl:attribute name="href"><xsl:value-of select="concat( @register_file, '#use_option__', @name )"/></xsl:attribute>
            <xsl:apply-templates select="." mode="register_title"/>
        </xsl:element>
        
    </xsl:template>
    -->
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~register_element-->
    <!--    
    <xsl:template match="register_element">

        <xsl:element name="a">
            <xsl:attribute name="href"><xsl:value-of select="concat( @register_file, '#use_element__', @name )"/></xsl:attribute>
            <xsl:apply-templates select="." mode="register_title"/>
        </xsl:element>
        
    </xsl:template>
    -->
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~register_title-->
    
    <xsl:template match="*" mode="register_title">

        <xsl:choose>
            <xsl:when test="@type='definition'">
                <span class="register_definition"><xsl:value-of select="@register_title"/></span>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="@register_title"/>
            </xsl:otherwise>
        </xsl:choose>
        
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/description-->
    
    <xsl:template match="/description">

        <html>
            <xsl:call-template name="html_head">
                <xsl:with-param name="title" select="@head_title | @title"/>
            </xsl:call-template>
        
            <body>
                <xsl:call-template name="body_start">
                    <xsl:with-param name="title"       select="@title"/>
                    <xsl:with-param name="parent_page" select="@parent_page"/>
                </xsl:call-template>

                <xsl:apply-templates select="node()" mode="description"/>
                
                <xsl:call-template name="bottom">
                    <xsl:with-param name="parent_page" select="@parent_page"/>
                </xsl:call-template>
            </body>
        </html>
        
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
        <xsl:element name="p">
            <xsl:attribute name="class">first</xsl:attribute>
            <xsl:for-each select="@*">
                <xsl:copy><xsl:value-of select="."/></xsl:copy>
            </xsl:for-each>
            <xsl:apply-templates mode="description"/>
        </xsl:element>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~description.style-->
    <!-- Wird schon von html_head interpretiert -->
    
    <xsl:template match="description.style" mode="description">
	<!-- style gehört ins <head>, s. html_head -->
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_comment-->
    <!-- Für Bemerkungen und Anregungen, die nicht für die Anwender gedacht sind -->
    
    <xsl:template match="scheduler_comment" mode="description">
	<!--
        <span class="comment">
            <xsl:for-each select="@*">
                <xsl:copy><xsl:value-of select="."/></xsl:copy>
            </xsl:for-each>
            <xsl:apply-templates mode="description"/>
        </span>
        -->
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_element-->

    <xsl:template match="scheduler_element" mode="description">
        <xsl:call-template name="scheduler_element">
            <xsl:with-param name="directory" select="@directory"/>
            <xsl:with-param name="name"      select="@name"     />
            <xsl:with-param name="attribute" select="@attribute"/>
            <xsl:with-param name="value"     select="@value"    />
            <xsl:with-param name="parameter" select="@parameter"/>
        </xsl:call-template>
        <!--
        <xsl:element name="a">
            <xsl:attribute name="href"><xsl:value-of select="@name"/>.xml</xsl:attribute>
            <code>&lt;<xsl:value-of select="@name"/>></code>
        </xsl:element>
        -->
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_element-->

    <xsl:template name="scheduler_element">
        <xsl:param name="name"/>
        <xsl:param name="directory"/>
        <xsl:param name="attribute"/>
        <xsl:param name="value"     select="'…'"/>
        <xsl:param name="parameter"/>
        
        <xsl:element name="a">
            <xsl:attribute name="name">use_element__<xsl:value-of select="$name"/></xsl:attribute>
        </xsl:element>

        <xsl:element name="a">
            <xsl:attribute name="class">silent</xsl:attribute>
            
            <xsl:variable name="href2">
                <xsl:if test="$attribute">#attribute_<xsl:value-of select="$attribute"/></xsl:if>
            </xsl:variable>

            <xsl:if test="$directory">
                <xsl:attribute name="href"><xsl:value-of select="concat( $base_dir, $directory, $name, '.xml', $href2 )"/></xsl:attribute>
            </xsl:if>            
            <xsl:if test="not( $directory )">
                <xsl:attribute name="href"><xsl:value-of select="concat( $base_dir, 'xml/', $name, '.xml', $href2 )"/></xsl:attribute>
            </xsl:if>            
            
            <code>
                <xsl:text>&lt;</xsl:text>
                <xsl:value-of select="$name"/>

                <xsl:if test="$attribute">
                    <xsl:text> </xsl:text>
                    <xsl:value-of select="$attribute"/>
                    <xsl:text>="</xsl:text>
                    <xsl:value-of select="$value"/>
                    <xsl:text>"</xsl:text>
                </xsl:if>

                <xsl:if test="$parameter">
                    <xsl:text> </xsl:text>
                    <xsl:value-of select="$parameter"/>
                </xsl:if>

                <xsl:text>></xsl:text>
            </code>
        </xsl:element>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_setting-->

    <xsl:template match="scheduler_setting" mode="description">

        <xsl:element name="a">
            <xsl:attribute name="href">settings.xml#option_<xsl:value-of select="@setting | @name"/></xsl:attribute>
    
            <xsl:choose>
                <xsl:when test="ancestor::ini_entry">
                    <code><xsl:value-of select="@name"/>=</code>
                </xsl:when>
                <xsl:when test="ancestor::command_option">
                    <code>-<xsl:value-of select="@name"/>=</code>
                </xsl:when>
                <xsl:otherwise>
                    <code><xsl:value-of select="@name | @setting"/>=</code>
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
        </xsl:element>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_ini_entry-->

    <xsl:template match="scheduler_ini_entry" mode="description">
    
        <xsl:call-template name="scheduler_ini_entry">
            <xsl:with-param name="file"    select="@file"/>
            <xsl:with-param name="section" select="@section"/>
            <xsl:with-param name="entry"   select="@entry"/>
        </xsl:call-template>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_ini_entry-->

    <xsl:template name="scheduler_ini_entry">
        <xsl:param name="file"/>
        <xsl:param name="section"/>
        <xsl:param name="entry"/>
        
        <xsl:element name="a">
            <xsl:attribute name="name">use_entry__<xsl:value-of select="$file"/>__<xsl:value-of select="$section"/>__<xsl:value-of select="$entry"/></xsl:attribute>
        </xsl:element>
        
        <xsl:element name="a">
            <xsl:attribute name="class">silent</xsl:attribute>
            <xsl:attribute name="href"><xsl:value-of select="concat( $base_dir, translate( $file, '.', '_' ), '_', $section, '.xml', '#entry_', $entry )"/></xsl:attribute>
            
            <code><xsl:value-of select="$file"/></code>
            
            (Abschnitt
            <code>[<xsl:value-of select="$section"/>]</code>,
            
            Eintrag
            <code><xsl:value-of select="$entry"/>=…</code>
            
            <xsl:text>)</xsl:text>
        </xsl:element>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_option-->

    <xsl:template match="scheduler_option" mode="description">
        <xsl:call-template name="scheduler_option">
            <xsl:with-param name="name" select="@name"/>
        </xsl:call-template>
        <!--
        <xsl:element name="a">
            <xsl:attribute name="class">silent</xsl:attribute>
            <xsl:attribute name="href">../command_line.xml#option_<xsl:value-of select="@name"/></xsl:attribute>
            <code>-<xsl:value-of select="@name"/>=</code>
        </xsl:element>
        -->
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_option-->

    <xsl:template name="scheduler_option">
        <xsl:param name="name"/>
        
        <xsl:element name="a">
            <xsl:attribute name="name">use_option__<xsl:value-of select="$name"/></xsl:attribute>
        </xsl:element>
        
        <xsl:element name="a">
            <xsl:attribute name="class">silent</xsl:attribute>
            <xsl:attribute name="href"><xsl:value-of select="concat( $base_dir, 'command_line.xml#option_', $name )"/></xsl:attribute>
            <code>-<xsl:value-of select="$name"/></code>

<!--
            <xsl:variable name="command_option" select="document( 'command_line.xml' )/command_line/command_options/command_option[ @name = current()/@name or @setting = current()/@name ]"/>

            <xsl:if test="$command_option/@type">
                <code>="</code><i><xsl:value-of select="$command_option/@type"/></i><code>"</code>
            </xsl:if>
-->            
        </xsl:element>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_method-->

    <xsl:template match="scheduler_method" mode="description">
        <xsl:call-template name="scheduler_method">
            <xsl:with-param name="class"            select="@class"/>
            <xsl:with-param name="method"           select="@method"/>
            <xsl:with-param name="property"         select="@property"/>
            <xsl:with-param name="java_signature"   select="@java_signature"/>
            <xsl:with-param name="access"           select="@access"/>
        </xsl:call-template>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_method-->

    <xsl:template name="scheduler_method">
        <xsl:param name="class"/>
        <xsl:param name="method"/>
        <xsl:param name="property"/>
        <xsl:param name="java_signature"/>
        <xsl:param name="access"/>          <!-- "read" (default), "write" -->
<!--        
        <xsl:variable name="java_method">
            <xsl:choose>
                <xsl:when test="$access = 'write'"><xsl:value-of select="concat( 'set_', $property )"/></xsl:when>
                <xsl:otherwise                    ><xsl:value-of select="$method | $property"/></xsl:otherwise>
            </xsl:choose>
        </xsl:variable>
-->        
        <xsl:element name="a">
            <xsl:attribute name="class">silent</xsl:attribute>
            
            <xsl:variable name="href_local"><xsl:if test="@method | @property">#method__<xsl:value-of select="@method | @property"/></xsl:if></xsl:variable>

            <xsl:attribute name="href"><xsl:value-of select="concat( $base_dir, 'api/', @class, '-', $programming_language, '.xml', $href_local )"/></xsl:attribute> 
            <!--xsl:attribute name="href"><xsl:value-of select="$base_dir"/>javadoc/sos/spooler/<xsl:value-of select="@class"/>.html#<xsl:value-of select="$java_method"/>(<xsl:value-of select="$java_signature"/>)</xsl:attribute-->
            
            <code>
                <xsl:if test="@class != 'Job_impl'">
                    <xsl:value-of select="@class"/>
                </xsl:if>
                
                <xsl:if test="@class != 'Job_impl' and ( @method or @property )">
                    <xsl:text>.</xsl:text>
                </xsl:if>
<!--                
                <xsl:if test="@method | @property">
                    <xsl:text>.</xsl:text>
                    <xsl:value-of select="$java_method"/>
                    <xsl:text>()</xsl:text>
                </xsl:if>
-->
                <xsl:if test="@method">
                    <xsl:value-of select="@method"/>
                    <xsl:text>()</xsl:text>
                </xsl:if>
                
                <xsl:if test="@property">
                    <xsl:value-of select="@property"/>
                </xsl:if>
                
            </code>
        </xsl:element>
        
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_error-->

    <xsl:template match="scheduler_error" mode="description">
    
        <code><xsl:value-of select="@code"/></code>
        
    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_keyword-->

    <xsl:template match="scheduler_keyword" mode="description">
    
        <xsl:element name="a">
            <xsl:attribute name="name">keyword__<xsl:value-of select="@keyword"/></xsl:attribute>
        </xsl:element>
        
    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~command_line-->
    
    <xsl:template match="command_line">

        <html>
            <xsl:call-template name="html_head">
                <xsl:with-param name="title" select="@title"/>
            </xsl:call-template>
        
            <body>
                <xsl:call-template name="body_start">
                    <xsl:with-param name="title" select="@title"/>
                </xsl:call-template>

                <xsl:apply-templates select="command_options" mode="table"/>
                <xsl:apply-templates select="description"/>
                <xsl:apply-templates select="command_options"/>
                <xsl:call-template name="bottom"/>
            </body>
        </html>
        
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~command_options-->
    
    <xsl:template match="command_options" mode="table">
    
        <p>
            <code><xsl:value-of select="parent::*/@program"/></code>
        </p>
        
        <div class="indent">
            <table cellspacing="0" cellpadding="0">
                <col/>
                <col style="padding-left: 4ex"/>
                <xsl:for-each select="command_option">
                    <xsl:variable name="setting" select="document( 'settings.xml' )/settings/setting[ @setting = current()/@setting ]"/>
                
                    <tr>
                        <td valign="baseline">
                            <xsl:element name="a">
                                <xsl:attribute name="class">silent</xsl:attribute>
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
        <xsl:apply-templates select="command_option">
            <xsl:sort select="@name | @setting"/>
        </xsl:apply-templates>
    
    </xsl:template>     

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~command_option-->
    
    <xsl:template match="command_option">
    
        <xsl:variable name="setting" select="document( 'settings.xml' )/settings/setting[ @setting = current()/@setting ]"/>
        
        <p class="command_option">
            <xsl:element name="a">
                <xsl:attribute name="name">option_<xsl:value-of select="@name"/></xsl:attribute>
            </xsl:element>
            
            <b><code>-<xsl:value-of select="@name"/></code></b>
            <xsl:if test="@type | $setting/@type">
                <b><code>=</code></b>
                <span class="type"><xsl:value-of select="@type | $setting/@type"/></span>                    
            </xsl:if>

            &#160;
            <xsl:apply-templates select="." mode="setting_header_rest"/>            
        </p>

        <xsl:apply-templates mode="setting_description" select="."/>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ini-->

    <xsl:template match="ini">
        <!--xsl:variable name="title" select="concat( 'Datei ', @file, ', ', @title )"/-->
        <xsl:variable name="title" select="@title"/>

        <html>
            <xsl:call-template name="html_head">
                <xsl:with-param name="title" select="$title"/>
            </xsl:call-template>
        
            <body>
                <xsl:call-template name="body_start">
                    <xsl:with-param name="title" select="$title"/>
                </xsl:call-template>
                
                <xsl:apply-templates mode="description" select="description/*"/>

                <p>
                    Unter Windows wird sollte die Datei dort angelegt werden, wo Windows seine <code>.ini</code>-Dateien erwartet.
                    Das ist normalerweise im Verzeichnis <code>c:\windows</code>. 
                    Sonst wird die Datei im Heimverzeichnis des Benutzers erwartet.
                </p>
                
                <h2>Abschnitte</h2>
                
                <xsl:for-each select="ini_sections/ini_section">
                    <xsl:variable name="section_filename" select="concat( translate( ancestor::ini/@file, '.', '_' ), '_', @name, '.xml' )"/>
                    <xsl:variable name="section"          select="document( $section_filename )/ini_section"/>
                    
                    <xsl:apply-templates select="$section" mode="table">
                        <xsl:with-param name="href" select="$section_filename"/>
                    </xsl:apply-templates>
                </xsl:for-each>
                
                <xsl:call-template name="bottom"/>
            </body>
        </html>
    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ini_section-->
    
    <xsl:template match="ini_section" mode="table">
        <xsl:param name="href"/>

        <table cellspacing="0" cellpadding="0">
            <col valign="baseline"/>
            <col valign="baseline"/>
            <col valign="baseline" style="padding-left: 4ex"/>
            
            <tr>
                <td>
                    <p class="ini_section">
                        <!--xsl:element name="a">
                            <xsl:attribute name="href">#section_<xsl:value-of select="@name"/></xsl:attribute>
                        </xsl:element-->
                        
                        <xsl:element name="a">
                            <xsl:if test="$href">
                                <xsl:attribute name="href"><xsl:value-of select="$href"/></xsl:attribute>
                            </xsl:if>
                            
                            <b>
                                <code>
                                    [<xsl:value-of select="@name"/>
                                    <xsl:if test="@variable_suffix">
                                        <i>&#160;<xsl:value-of select="@variable_suffix"/></i>
                                    </xsl:if>
                                    <xsl:text>]</xsl:text>
                                </code>
                            </b>
                        </xsl:element>
                    </p>
                </td>
                <td>
                    <span class="title">
                        <xsl:value-of select="@title"/>
                    </span>
                </td>
            </tr>
            
            <tr>
                <td colspan="3">
                    <xsl:apply-templates mode="description" select="description/*"/>
                    <p>&#160;</p>
                </td>
            </tr>

            <xsl:for-each select="ini_entries/ini_entry">
                <xsl:sort select="@name | @setting"/>
                
                <xsl:variable name="setting" select="document( 'settings.xml' )/settings/setting[ @setting = current()/@setting ]"/>

                <tr>
                    <td>
                        <xsl:element name="a">
                            <xsl:attribute name="class">silent</xsl:attribute>
                            <xsl:attribute name="href"><xsl:value-of select="$href"/>#setting_<xsl:value-of select="@setting"/></xsl:attribute>
                            <code><xsl:value-of select="@name | @setting"/></code>
                        </xsl:element>
                    </td>
                    <td>
                        <code>= </code><span class="type"><xsl:value-of select="@type | $setting/@type"/></span>
                    </td>
                    <td>
                        <span class="title">
                            <xsl:value-of select="@title | $setting/@title"/>
                        </span>
                    </td>
                </tr>
            </xsl:for-each>
        </table>
        <p>&#160;</p>
    
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ini_section-->
    
    <xsl:template match="ini_section">

        <xsl:variable name="title"       select="concat( 'Datei ', @file, ',&#160; Abschnitt&#160; [', @name, ']' )"/>
        <xsl:variable name="parent_page" select="concat( translate( @file, '.', '_' ), '.xml' )"/>
            
        <html>
            <xsl:call-template name="html_head">
                <xsl:with-param name="title" select="$title"/>
            </xsl:call-template>
        
            <body>
                <xsl:call-template name="body_start">
                    <xsl:with-param name="title"       select="$title"/>
                    <xsl:with-param name="parent_page" select="$parent_page"/>
                </xsl:call-template>

                <xsl:apply-templates select="." mode="table"/>
                
                <h2>Einträge</h2>
                <xsl:apply-templates select="." mode="details"/>

                <xsl:call-template name="bottom">
                    <xsl:with-param name="title"       select="$title"/>
                    <xsl:with-param name="parent_page" select="$parent_page"/>
                </xsl:call-template>
            </body>
        </html>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ini_section-->
    
    <xsl:template match="ini_section" mode="details">

        <p class="ini_section">
            <!--
            <xsl:element name="a">
                <xsl:attribute name="name">section_<xsl:value-of select="@setting"/></xsl:attribute>
            </xsl:element>
            -->
            
            <span class="title">
                <xsl:value-of select="@title"/>
            </span>
        </p>

        <xsl:for-each select="ini_entries/ini_entry">
            <!--xsl:sort select="@name | @setting"/-->

            <p class="ini_entry">
                <xsl:variable name="setting" select="document( 'settings.xml' )/settings/setting[ @setting = current()/@setting ]"/>

                <xsl:element name="a">
                    <xsl:attribute name="name">setting_<xsl:value-of select="@setting"/></xsl:attribute>
                </xsl:element>
                
                <xsl:element name="a">
                    <xsl:attribute name="name">entry_<xsl:value-of select="@name | @setting"/></xsl:attribute>
                </xsl:element>
                
                <table cellspacing="0" cellpadding="0">
                    <tr>
                        <td style="width: 250px">
                            <b><code><xsl:value-of select="@name | @setting"/></code></b>
                            
                            <b><code>=</code></b>
                            <span class="type"><xsl:value-of select="@type | $setting/@type"/></span>
                        </td>
                        <td style="padding-left: 2ex">
                            <xsl:apply-templates select="." mode="setting_header_rest"/>            
                        </td>
                    </tr>
                </table>
            </p>
            
            <xsl:apply-templates mode="setting_description" select="."/>
        </xsl:for-each>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~settings-->
    
    <xsl:template match="settings">

        <html>
            <xsl:call-template name="html_head">
                <xsl:with-param name="title" select="@title"/>
            </xsl:call-template>
        
            <body>
                <xsl:call-template name="body_start">
                    <xsl:with-param name="title"       select="@title"/>
                    <xsl:with-param name="parent_page" select="@parent_page"/>
                </xsl:call-template>

                <xsl:apply-templates select="description"/>
                <p>&#160;</p>

                <xsl:apply-templates select="setting">
                    <xsl:sort select="@setting"/>
                </xsl:apply-templates>

                <xsl:call-template name="bottom">
                    <xsl:with-param name="title"       select="@title"/>
                    <xsl:with-param name="parent_page" select="@parent_page"/>
                </xsl:call-template>
            </body>
        </html>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~setting-->
    
    <xsl:template match="setting">
    
        <xsl:element name="a">
            <xsl:attribute name="name">option_<xsl:value-of select="@setting"/></xsl:attribute>
        </xsl:element>
        
        <p class="command_option">
            <b><code><xsl:value-of select="@setting"/></code></b>
            <xsl:if test="@type">
                <b><code>=</code></b>
                <span class="type"><xsl:value-of select="@type"/></span>                    
            </xsl:if>

            &#160;
            <xsl:apply-templates select="." mode="setting_header_rest"/>            
        </p>

        <xsl:apply-templates mode="setting_description" select="."/>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~html_head-->
    
    <xsl:template name="html_head">
        <xsl:param name="title"/>
        
        <head>
            <title>
                Scheduler: <xsl:value-of select="$title"/>
            </title>
            
            <style type="text/css">
                @import "<xsl:value-of select="$base_dir"/>scheduler.css";
                <xsl:copy-of select="/*/description.style/node()"/>
            </style>
        </head>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~body_start-->
    
    <xsl:template name="body_start">
        <xsl:param name="title"/>
        <xsl:param name="parent_page"/>

        <table cellspacing="0" cellpadding="0" width="100%">
            <tr>
                <td>
                    <xsl:call-template name="browse_bar">
                        <xsl:with-param name="parent_page" select="$parent_page"/>
                    </xsl:call-template>
                </td>
                <td align="right">
                    <p style="font-size: 8pt; margin-top: 0">
                        <xsl:call-template name="browse_bar_register">
                            <xsl:with-param name="parent_page" select="$parent_page"/>
                        </xsl:call-template>

                        Software- und Organisations-Service GmbH
                    </p>
                </td>
            </tr>
        </table>

        <hr style="margin-bottom: 20pt"/>
        
        <xsl:if test="$title">
            <h1 style="margin-top: 0pt">
                <xsl:value-of select="$title"/>
            </h1>
        </xsl:if>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~bottom-->

    <xsl:template name="bottom">
        <xsl:param name="parent_page"/>

        <hr style="margin-top: 30pt"/>
        <table cellspacing="0" cellpadding="0" width="100%">
            <tr>
                <td>
                    <xsl:call-template name="browse_bar">
                        <xsl:with-param name="parent_page" select="$parent_page"/>
                    </xsl:call-template>
                </td>
                <td align="right">
                    <p style="font-size: 8pt; margin-top: 0">
                        <xsl:call-template name="browse_bar_register">
                            <xsl:with-param name="parent_page" select="$parent_page"/>
                        </xsl:call-template>

                        Zuletzt geändert von
                        <xsl:variable name="name" select="substring-before( substring-after( /*/@author, 'Author: ' ), ' $' )"/>
                        <xsl:choose>
                            <xsl:when test="$name = 'jz'">Joacim Zschimmer</xsl:when>
                            <xsl:otherwise><xsl:value-of select="$name"/></xsl:otherwise>
                        </xsl:choose>,
                        <!--xsl:variable name="name" select="document('standards.xml')/standards/authors/author[ @author = current()/@author ]/@full_name"/>
                        <xsl:value-of select="$name"/>,-->
                        <xsl:value-of select="translate( substring-before( substring-after( /*/@date,   'Date: '   ), ' (' ), '/', '-' )"/>
                    </p>
                </td>
            </tr>
        </table>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~browse_bar-->

    <xsl:template name="browse_bar">
        <xsl:param name="parent_page"/>

        <p style="font-size: 8pt; margin-top: 0px; padding-top: 0px">
            Scheduler &#160; &#160; &#160; 
            <xsl:if test="not( /*/@suppress_browse_bar='yes' )">
                <xsl:element name="a">
                    <xsl:attribute name="class">silent</xsl:attribute>
                    <xsl:attribute name="href"><xsl:value-of select="concat( $base_dir, $start_page )"/></xsl:attribute>
                    <xsl:attribute name="rel">start</xsl:attribute>
                    <xsl:text>Erste Seite</xsl:text>
                </xsl:element>
                
                <xsl:if test="$parent_page">
                    &#160; – &#160;
                    <xsl:element name="a">
                        <xsl:attribute name="class">silent</xsl:attribute>
                        <!--xsl:attribute name="href"><xsl:value-of select="concat( $base_dir, $parent_page )"/></xsl:attribute-->
                        <xsl:attribute name="href"><xsl:value-of select="$parent_page"/></xsl:attribute>
                        <xsl:value-of select="document( $parent_page )/*/@title"/>
                    </xsl:element>
                </xsl:if>
                
            </xsl:if>
        </p>
    </xsl:template>    
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~browse_bar_register-->

    <xsl:template name="browse_bar_register">

        <xsl:element name="a">
            <xsl:attribute name="href"><xsl:value-of select="$base_dir"/>register.xml</xsl:attribute>
            <xsl:attribute name="rel">index</xsl:attribute>
            <xsl:text>Register</xsl:text>
        </xsl:element>
        &#160; &#160; &#160; 

    </xsl:template>    
    
</xsl:stylesheet>

<!-- Das ist ein Gedankenstrich: – -->
<!-- Das ist drei Punkte: … -->
