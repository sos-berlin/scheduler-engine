<?xml version='1.0' encoding="utf-8"?>
<!-- $Id$        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com -->

<!--
    Änderungswünsche:

    @same_as_element sollte durch setting ersetzt werden.
    Zentrale Einstellung nur noch in settings.xml dokumentieren.
    
    
    <a name="use_element__...">, <a name="use_entry__..."> (fürs Register) können mehrfach vorkommen, das ist nicht valide.
    <a name=".."> soll in XHTML 1.1 <a id="..."> heißen
-->


<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform"
                version   = "1.0">

    <xsl:import href="scheduler-phrases.xsl"/>
    <xsl:variable name="phrases" select="document( 'scheduler-phrases.xsl' )/xsl:stylesheet/xsl:template [ @name='this_is_a_container_only_for_phrases' ]/phrases" />
    
    <!--xsl:param name="selected_programming_language"/   verhindert in ie6 die Transformation -->
    
    <xsl:output 
        method               = "html" 
        doctype-public       = "-//W3C//DTD XHTML 1.0 Strict//EN" 
        doctype-system       = "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd"/>
    <!-- omit-xml-declaration="yes" für Firefox 1.5.3 -->
    <!--xsl:output doctype-public="-//W3C//DTD HTML 4.01//EN" />  <!- -"http://www.w3.org/TR/html4/strict.dtd"-->
    <!-- Nicht für Firefox 1.0.6: media-type="application/xhtml+xml" -->

    <xsl:variable name="start_page" select="'index.xml'"/>
    <xsl:variable name="base_dir"   select="/*/@base_dir"/>
    <xsl:variable name="debug_phrases" select="$phrases [ @style and @style!='' ]"/>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~phrase-->

    <xsl:template name="phrase">
        <xsl:param name="id"/>

        <xsl:element name="span">
            <xsl:attribute name="style"><xsl:value-of select="$phrases/@style"/></xsl:attribute>

            <xsl:variable name="phrase" select="$phrases/phrase [ @id = $id ]"/>
            
            <xsl:choose>
                <xsl:when test="$phrase/node()">
                    <xsl:copy-of select="$phrase/node()"/>
                </xsl:when>
                <xsl:otherwise>
                    <span style="background-color: red; color: white" title="Missing translation in scheduler-phrases.xsl">
                        &lt;phrase id="<xsl:value-of select="$id"/>"/>
                    </span>
                    <xsl:message>*** Übersetzung &lt;phrase id="<xsl:value-of select="$id"/>"/> fehlt ***</xsl:message>
                </xsl:otherwise>
            </xsl:choose>

        </xsl:element>
    </xsl:template>

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
            <col valign="baseline"/>
            <thead>
                <tr>
                    <td>
                        <xsl:call-template name="phrase">
                            <xsl:with-param name="id" select="'xml.command_table.title.command'"/>
                        </xsl:call-template>
                    </td>
                    <td style="padding-top: 1ex; padding-left: 2ex">
                        <xsl:call-template name="phrase">
                            <xsl:with-param name="id" select="'xml.command_table.title.answer'"/>
                        </xsl:call-template>
                    </td>
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
                        <td style="padding-left: 2ex">
                            <xsl:call-template name="scheduler_element">
                                <xsl:with-param name="directory" select="'xml/answer/'"/>
                                <xsl:with-param name="name" select="@answer"/>
                            </xsl:call-template>
                            
                            <xsl:if test="@answer2">
                                <xsl:call-template name="scheduler_element">
                                    <xsl:with-param name="directory" select="'xml/answer/'"/>
                                    <xsl:with-param name="name" select="@answer2"/>
                                </xsl:call-template>
                            </xsl:if>
                        </td>
                    </tr>
                </xsl:for-each>
            </tbody>
        </table>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~xml_element-->

    <xsl:template match="xml_element">

        <xsl:variable name="title"><xsl:value-of select="$phrases/phrase [ @id='xml_element.chapter_title.prefix' ]"/>&#160; &lt;<xsl:value-of select="@name"/>><xsl:if test="@category">&#160; &#160; (<xsl:value-of select="@category"/>)</xsl:if></xsl:variable>

        <html>
            <xsl:call-template name="html_head">
                <xsl:with-param name="title" select="$title"/>
            </xsl:call-template>


            <body>
                <xsl:call-template name="body_start">
                    <xsl:with-param name="title"       select="$title"/>
                    <xsl:with-param name="parent_page" select="@parent_page"/>
                </xsl:call-template>

                <div class="example">
                    <code>&lt;<xsl:value-of select="@name"/></code>

                    <xsl:if test="xml_attributes/xml_attribute">
                        <div class="indent">
                            <table cellspacing="0" cellpadding="0">
                                <col/>
                                <col/>
                                <col/>

                                <xsl:for-each select="xml_attributes/xml_attribute">
                                    <xsl:sort select="@name | @setting"/>

                                    <xsl:variable name="setting" select="document( 'settings.xml' )/settings/setting[ @setting = current()/@setting ]"/>

                                    <tr>
                                        <td valign="baseline">
                                            <xsl:element name="a">
                                                <xsl:attribute name="class">silent</xsl:attribute>
                                                <xsl:attribute name="href">
                                                    <xsl:text>#attribute_</xsl:text>
                                                    <xsl:value-of select="@name | @setting"/>
                                                </xsl:attribute>
                                                <code><xsl:value-of select="@name | @setting"/></code>&#160;
                                            </xsl:element>
                                        </td>
                                        <td valign="baseline">
                                            <code>= "</code><span class="type"><xsl:value-of select="@type | $setting/@type"/></span><code>"</code>
                                        </td>
                                        <td valign="baseline" style="padding-left: 4ex">
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
                                    <col/>
                                    <xsl:for-each select="xml_child_elements/xml_child_element">
                                        <xsl:variable name="path"><xsl:value-of select="@directory"/><xsl:if test="not( @directory )"><xsl:value-of select="concat( 'xml/', /*/@sub_directory )"/></xsl:if><xsl:value-of select="concat( @name, '.xml' )"/></xsl:variable>
                                        <xsl:variable name="element" select="document( string($path) )/xml_element[ @name=current()/@name ]"/>
                                        
                                        <tr>
                                            <td valign="baseline">
                                                <xsl:element name="a">
                                                    <xsl:attribute name="class">silent</xsl:attribute>
                                                    <xsl:choose>
                                                        <xsl:when test="* | @multiple">
                                                            <xsl:attribute name="href">
                                                                <xsl:text>#element_</xsl:text>
                                                                <xsl:value-of select="@name"/>
                                                            </xsl:attribute>
                                                        </xsl:when>
                                                        <xsl:otherwise>
                                                            <xsl:attribute name="href"><xsl:value-of select="concat( /*/@base_dir, $path )"/></xsl:attribute>
                                                        </xsl:otherwise>
                                                    </xsl:choose>
                                                    <code>&lt;<xsl:value-of select="@name"/></code> ...<code>></code><br/>
                                                </xsl:element>
                                            </td>
                                            <td valign="baseline" style="padding-left: 4ex">
                                                <span class="title">
                                                    <xsl:choose>
                                                        <xsl:when test="@title">
                                                            <xsl:value-of select="@title"/>
                                                        </xsl:when>
                                                        <xsl:otherwise>
                                                            <xsl:value-of select="$element/@title"/>
                                                        </xsl:otherwise>
                                                    </xsl:choose>
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
                </div>
                
                <p>&#160;</p>

                <xsl:apply-templates select="description"/>
                <xsl:apply-templates select="example"/>

                <xsl:for-each select="behavior_with_xml_element">
                    <h2>
                        <xsl:call-template name="phrase">
                            <xsl:with-param name="id" select="'xml_element.behaviour_with'"/>
                        </xsl:call-template>
                        
                        <xsl:text>&#160;</xsl:text>
                        
                        <xsl:call-template name="scheduler_element">
                            <xsl:with-param name="name" select="@element"/>
                        </xsl:call-template>
                    </h2>

                    <p>
                        <xsl:apply-templates select="." mode="phrase">
                            <xsl:with-param name="element">
                                <code>&lt;<xsl:value-of select="parent::*/@name"/>&gt;</code>
                            </xsl:with-param>
                        </xsl:apply-templates>
                        <xsl:apply-templates select="description"/>
                    </p>
                </xsl:for-each>

                <xsl:apply-templates select="xml_parent_elements"/>
                <xsl:apply-templates select="xml_attributes"/>
                <xsl:apply-templates select="xml_child_elements"/>
                <xsl:apply-templates select="xml_answer"/>
                <xsl:apply-templates select="messages"/>

                <xsl:call-template name="bottom">
                    <xsl:with-param name="parent_page" select="@parent_page"/>
                </xsl:call-template>
            </body>
        </html>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~xml_parent_elements-->

    <xsl:template match="xml_parent_elements">
        <h2>
            <xsl:call-template name="phrase">
                <xsl:with-param name="id" select="'xml_element.parent_elements.title'"/>
            </xsl:call-template>
        </h2>
        
        <xsl:apply-templates select="xml_parent_element"/>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~xml_child_elements-->

    <xsl:template match="xml_child_elements">
        <h2>
            <xsl:call-template name="phrase">
                <xsl:with-param name="id" select="'xml_element.child_elements.title'"/>
            </xsl:call-template>
        </h2>

        <xsl:apply-templates select="xml_child_element"/>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~xml_parent_element | xml_child_element-->

    <xsl:template match="xml_parent_element | xml_child_element">

        <xsl:variable name="path"><xsl:value-of select="@directory"/><xsl:if test="not( @directory )"><xsl:value-of select="concat( 'xml/', /*/@sub_directory )"/></xsl:if><xsl:value-of select="concat( @name, '.xml' )"/></xsl:variable>
        <xsl:variable name="element" select="document( string($path) )/xml_element[ @name=current()/@name ]"/>

        <xsl:if test="self::xml_child_element">
            <xsl:element name="a">
                <xsl:attribute name="name">
                    <xsl:text>element_</xsl:text>
                    <xsl:value-of select="@name"/>
                </xsl:attribute>
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
                    <p>
                        <b>
                            <code>
                                <xsl:element name="a">
                                    <xsl:attribute name="class">silent</xsl:attribute>
                                    <xsl:attribute name="href"><xsl:value-of select="concat( /*/@base_dir, $path )"/></xsl:attribute>
                                    <xsl:text>&lt;</xsl:text>
                                    <xsl:value-of select="@name"/>
                                    <xsl:text>></xsl:text>
                                </xsl:element>
                            </code>
                        </b>
                        &#160;
                    </p>
                </xsl:element>
                <td>
                    <p>
                    &#160;
                    –
                    <span class="title">
                        <xsl:choose>
                            <xsl:when test="@title">
                                <xsl:value-of select="@title"/>
                            </xsl:when>
                            <xsl:otherwise>
                                <xsl:value-of select="$element/@title"/>
                            </xsl:otherwise>
                        </xsl:choose>
                    </span>
                    </p>
                </td>
            </tr>
        </table>


        <div class="indent">
            <xsl:apply-templates select="description"/>

            <xsl:if test="self::xml_child_element/@multiple='yes'">
                <p>
                    <xsl:apply-templates select="." mode="phrase">
                        <xsl:with-param name="element">
                            <code>&lt;<xsl:value-of select="@name"/>></code>
                        </xsl:with-param>
                    </xsl:apply-templates>
                </p>
            </xsl:if>
            
            <xsl:apply-templates select="example"/>
        </div>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~xml_attributes-->

    <xsl:template match="xml_attributes">
        <h2>
            <xsl:call-template name="phrase">
                <xsl:with-param name="id" select="'xml_element.attributes.title'"/>
            </xsl:call-template>
        </h2>
        
        <xsl:apply-templates select="xml_attribute"/>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~xml_attribute-->

    <xsl:template match="xml_attribute [ @same_as_element ]">
        <xsl:apply-templates select="document( concat( 'xml/', /*/@sub_directory, @same_as_element, '.xml' ) )/xml_element/xml_attributes/xml_attribute[ @name=current()/@name ]"/>
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
            <xsl:call-template name="phrase">
                <xsl:with-param name="id" select="'xml_element.answer.title.prefix'"/>
            </xsl:call-template>

            <xsl:text>&#160;</xsl:text>
            &lt;<xsl:value-of select="@element"/>>

            <xsl:if test="@element2">
                &lt;<xsl:value-of select="@element2"/>>
            </xsl:if>
        </h2>

        <div class="example">
            <code>
                <xsl:call-template name="scheduler_element">
                    <xsl:with-param name="directory" select="'xml/answer/'"/>
                    <xsl:with-param name="name"      select="'spooler'"/>
                </xsl:call-template>
            </code>
            <br/>

            <code>
                &#160; &#160;

                <xsl:call-template name="scheduler_element">
                    <xsl:with-param name="directory" select="'xml/answer/'"/>
                    <xsl:with-param name="name"      select="'answer'"/>
                </xsl:call-template>
            </code>
            <br/>

            <code>
                &#160; &#160; &#160; &#160;
                <xsl:call-template name="scheduler_element">
                    <xsl:with-param name="directory" select="'xml/answer/'"/>
                    <xsl:with-param name="name"      select="@element"/>
                    <xsl:with-param name="parameter" select="'…'"/>
                </xsl:call-template>
            </code>
            <br/>

            <xsl:if test="@element2">
                <code>
                    &#160; &#160; &#160; &#160; &#160; &#160;
                    <xsl:call-template name="scheduler_element">
                        <xsl:with-param name="directory" select="'xml/answer/'"/>
                        <xsl:with-param name="name"      select="@element2"/>
                        <xsl:with-param name="parameter" select="'…'"/>
                    </xsl:call-template>
                </code>
                <br/>

                <code>
                    &#160; &#160; &#160; &#160;
                    &lt;/<xsl:value-of select="@element"/>>
                </code>
                <br/>
            </xsl:if>


            <code>
                &#160; &#160;
                &lt;/answer>
            </code>
            <br/>
            <code>&lt;/spooler></code>
        </div>

        <xsl:apply-templates select="description"/>

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
                        <col valign="baseline"/>
                        <col valign="baseline"/>

                        <thead>
                            <tr>
                                <td>
                                    <xsl:call-template name="phrase">
                                        <xsl:with-param name="id" select="'log_categories.table.categories.title'"/>
                                    </xsl:call-template>
                                </td>
                                <td style="padding-left: 2ex">
                                    <xsl:call-template name="phrase">
                                        <xsl:with-param name="id" select="'log_categories.table.default.title'"/>
                                    </xsl:call-template>

                                    <xsl:text>¹</xsl:text>
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
                    <xsl:call-template name="phrase">
                        <xsl:with-param name="id" select="'log_categories.footnotes'"/>
                    </xsl:call-template>
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

            <td style="padding-left: 2ex">
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

            <td style="padding-left: 2ex">
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
            <xsl:text>(</xsl:text>

            <xsl:call-template name="phrase">
                <xsl:with-param name="id" select="'setting.initial_value'"/>
            </xsl:call-template>

            <xsl:text>: </xsl:text>
            <code>
                <xsl:value-of select="@initial | $setting/@initial"/>
            </code>

            <xsl:text>)</xsl:text>
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
                    <xsl:call-template name="phrase">
                        <xsl:with-param name="id" select="'setting.subst_env'"/>
                    </xsl:call-template>

                    <xsl:element name="a">
                        <xsl:attribute name="class">silent</xsl:attribute>
                        <xsl:attribute name="href">
                            <xsl:value-of select="$base_dir"/>
                            <xsl:text>variable_substitution.xml</xsl:text>
                        </xsl:attribute>
                        <xsl:text> </xsl:text>
                        <xsl:call-template name="phrase">
                            <xsl:with-param name="id" select="'setting.subst_env.here'"/>
                        </xsl:call-template>
                    </xsl:element>

                    <xsl:text>).</xsl:text>
                </p>
            </xsl:if>

            <xsl:apply-templates select="." mode="setting_references"/>
            <xsl:apply-templates select="$setting/example"/>
            
            <xsl:apply-templates select="$setting/messages">
                <xsl:with-param name="h" select="'h4'"/>
            </xsl:apply-templates>


            <xsl:apply-templates select="messages">
                <xsl:with-param name="h" select="'h4'"/>
                <xsl:with-param name="show_level" select="true()"/>
            </xsl:apply-templates>

            <xsl:apply-templates select="example"/>
        </div>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~mode=setting_references-->

    <xsl:template match="*" mode="setting_references">
        <!-- Verweise auf andere Stellen, wo der Wert eingestellt werden kann: -->

        <table cellpadding="0" cellspacing="0" class="see_also" style="margin-top: 1em;">
            <xsl:if test="not( self::property )">
                <xsl:variable name="property" select="document( 'api/Spooler.xml' )/api.class/property [ @setting = current()/@setting ] |
                                                      document( 'api/Log.xml'     )/api.class/property [ @setting = current()/@setting ] |
                                                      document( 'api/Mail.xml'    )/api.class/property [ @setting = current()/@setting ] "/>
                <xsl:if test="$property">
                    <tr>
                        <xsl:variable name="has_priority" select="count( $property [ not( @access ) or @access='write' ] ) > 0"/>

                        <td>
                            <xsl:call-template name="phrase">
                                <xsl:with-param name="id" select="concat( 'setting_references.api_property_has_priority=', $has_priority, '.prefix' )"/>
                            </xsl:call-template>
                        </td>

                        <td style="padding-left: 1ex">
                            <xsl:call-template name="scheduler_method">
                                <xsl:with-param name="class"    select="$property/parent::api.class/@name"/>
                                <xsl:with-param name="property" select="$property/@name"/>
                            </xsl:call-template>
                        </td>

                        <td style="padding-left: 1ex">
                            <xsl:call-template name="phrase">
                                <xsl:with-param name="id" select="concat( 'setting_references.api_property_has_priority=', $has_priority, '.suffix' )"/>
                            </xsl:call-template>
                        </td>
                    </tr>

                    <xsl:if test="$debug_phrases">
                        <tr style="text-decoration: line-through">
                            <td>
                                Die Eigenschaft
                            </td>

                            <td style="padding-left: 1ex">
                                <xsl:call-template name="scheduler_method">
                                    <xsl:with-param name="class"    select="$property/parent::api.class/@name"/>
                                    <xsl:with-param name="property" select="$property/@name"/>
                                </xsl:call-template>
                            </td>

                            <td style="padding-left: 1ex">
                                <xsl:choose>
                                    <xsl:when test="$property [ not( @access ) or @access='write' ]">
                                        hat Vorrang.
                                    </xsl:when>
                                    <xsl:otherwise>
                                        liest die Einstellung.
                                    </xsl:otherwise>
                                </xsl:choose>
                            </td>
                        </tr>
                    </xsl:if>
                </xsl:if>
            </xsl:if>

            <xsl:if test="not( self::command_option )">
                <xsl:variable name="command_option" select="document( 'command_line.xml' )/command_line/command_options/command_option[ @setting = current()/@setting ]"/>
                <xsl:if test="$command_option">
                    <tr>
                        <xsl:variable name="has_priority" select="not( self::property )"/>

                        <td>
                            <xsl:call-template name="phrase">
                                <xsl:with-param name="id" select="concat( 'setting_references.option_has_priority=', $has_priority, '.prefix' )"/>
                            </xsl:call-template>
                        </td>

                        <td style="padding-left: 1ex">
                            <xsl:call-template name="scheduler_option">
                                <xsl:with-param name="name"  select="$command_option/@name"/>
                                <!--xsl:with-param name="value" select="document( 'settings.xml' )/settings/setting[ @setting = current()/@setting ]/@initial"/-->
                            </xsl:call-template>
                        </td>

                        <td style="padding-left: 1ex">
                            <xsl:call-template name="phrase">
                                <xsl:with-param name="id" select="concat( 'setting_references.option_has_priority=', $has_priority, '.suffix' )"/>
                            </xsl:call-template>
                        </td>
                    </tr>

                    <xsl:if test="$debug_phrases">
                        <tr style="text-decoration: line-through">
                            <td>
                                Die Option
                            </td>

                            <td style="padding-left: 1ex">
                                <xsl:call-template name="scheduler_option">
                                    <xsl:with-param name="name"  select="$command_option/@name"/>
                                    <!--xsl:with-param name="value" select="document( 'settings.xml' )/settings/setting[ @setting = current()/@setting ]/@initial"/-->
                                </xsl:call-template>
                            </td>

                            <td style="padding-left: 1ex">
                                <xsl:choose>
                                    <xsl:when test="not( self::property )">
                                        hat Vorrang.
                                    </xsl:when>
                                    <xsl:otherwise>
                                        wird damit überschrieben.
                                    </xsl:otherwise>
                                </xsl:choose>
                            </td>
                        </tr>
                    </xsl:if>
                </xsl:if>
            </xsl:if>


            <xsl:variable name="ini_entry" select="( document( 'factory_ini_job.xml'     ) |
                                                     document( 'factory_ini_spooler.xml' ) |
                                                     document( 'sos_ini_mail.xml'        ) |
                                                     document( 'sos_ini_java.xml'        )   )/ini_section/ini_entries/ini_entry[ @setting = current()/@setting ]"/>
            <xsl:variable name="current_setting" select="."/>

            <xsl:for-each select="$ini_entry">
                <xsl:if test="not( $current_setting [ self::ini_entry ] ) or 
                               ancestor::ini_section/@file != $current_setting/ancestor::ini_section/@file  or  
                               ancestor::ini_section/@name != $current_setting/ancestor::ini_section/@name">
                    <tr>
                        <xsl:variable name="has_priority" select="$current_setting/ancestor::ini_section/@weight &lt; ancestor::ini_section/@weight"/>

                        <td>
                            <xsl:call-template name="phrase">
                                <xsl:with-param name="id" select="concat( 'setting_references.ini_has_priority=', $has_priority, '.prefix' )"/>
                            </xsl:call-template>
                        </td>

                        <td style="padding-left: 1ex">
                            <xsl:call-template name="scheduler_ini_entry">
                                <xsl:with-param name="file"    select="ancestor::ini_section/@file"/>
                                <xsl:with-param name="section" select="ancestor::ini_section/@name"/>
                                <xsl:with-param name="entry"   select="@name | @setting"/>
                                <!--xsl:with-param name="value"   select="document( 'settings.xml' )/settings/setting[ @setting = current()/@setting ]/@initial"/-->
                            </xsl:call-template>
                        </td>

                        <td style="padding-left: 1ex">
                            <xsl:call-template name="phrase">
                                <xsl:with-param name="id" select="concat( 'setting_references.ini_has_priority=', $has_priority, '.suffix' )"/>
                            </xsl:call-template>
                        </td>
                    </tr>

                    <xsl:if test="$debug_phrases">
                        <tr style="text-decoration: line-through">
                            <td>
                                Die Einstellung
                            </td>

                            <td style="padding-left: 1ex">
                                <xsl:call-template name="scheduler_ini_entry">
                                    <xsl:with-param name="file"    select="ancestor::ini_section/@file"/>
                                    <xsl:with-param name="section" select="ancestor::ini_section/@name"/>
                                    <xsl:with-param name="entry"   select="@name | @setting"/>
                                    <!--xsl:with-param name="value"   select="document( 'settings.xml' )/settings/setting[ @setting = current()/@setting ]/@initial"/-->
                                </xsl:call-template>
                            </td>

                            <td style="padding-left: 1ex">
                                <xsl:choose>
                                    <!--xsl:when test="not( $current_setting/self::property       ) and
                                                    not( $current_setting/self::command_option ) and 
                                                    not( $current_setting/self::ini_entry and $current_setting/ancestor::ini_section/@name='job' )">
                                        hat Vorrang.                                                            
                                    </xsl:when-->
                                    <xsl:when test="$current_setting/ancestor::ini_section/@weight &lt; ancestor::ini_section/@weight">
                                        hat Vorrang.
                                    </xsl:when>
                                    <xsl:otherwise>
                                        wird damit überschrieben.
                                    </xsl:otherwise>
                                </xsl:choose>
                            </td>
                        </tr>
                    </xsl:if>
                </xsl:if>
            </xsl:for-each>


            <xsl:if test="not( self::xml_attribute )">
                <xsl:variable name="config_attribute" select="document( 'xml/config.xml' )/xml_element/xml_attributes/xml_attribute[ @setting = current()/@setting ]"/>

                <xsl:if test="$config_attribute">
                    <tr>
                        <xsl:variable name="has_priority" select="false()"/>

                        <td>
                            <xsl:call-template name="phrase">
                                <xsl:with-param name="id" select="concat( 'setting_references.xml_has_priority=', $has_priority, '.prefix' )"/>
                            </xsl:call-template>
                        </td>

                        <td style="padding-left: 1ex">
                            <xsl:call-template name="scheduler_element">
                                <xsl:with-param name="name"      select="'config'"/>
                                <xsl:with-param name="attribute" select="$config_attribute/@name | $config_attribute/@setting"/>
                            </xsl:call-template>
                        </td>

                        <td style="padding-left: 1ex">
                            <xsl:call-template name="phrase">
                                <xsl:with-param name="id" select="concat( 'setting_references.xml_has_priority=', $has_priority, '.suffix' )"/>
                            </xsl:call-template>
                        </td>
                    </tr>

                    <xsl:if test="$debug_phrases">
                        <tr style="text-decoration: line-through">

                            <td>
                                Das XML-Attribut
                            </td>

                            <td style="padding-left: 1ex">
                                <xsl:call-template name="scheduler_element">
                                    <xsl:with-param name="name"      select="'config'"/>
                                    <xsl:with-param name="attribute" select="$config_attribute/@name | $config_attribute/@setting"/>
                                </xsl:call-template>
                            </td>

                            <td style="padding-left: 1ex">
                                <!--xsl:choose>
                                    <xsl:when test="$current_setting/self::ini_entry or $current_setting/self::command_option or $current_setting/self::property [ not( @access='read' ) ]">
                                    </xsl:when>
                                </xsl:choose-->
                                wird damit überschrieben.
                            </td>
                        </tr>
                    </xsl:if>
                </xsl:if>
            </xsl:if>
        </table>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~example-->

    <xsl:template match="example">
        <xsl:param name="programming_language"/>

        <xsl:call-template name="example">
            <xsl:with-param name="programming_language" select="$programming_language"/>
        </xsl:call-template>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~example-->

    <xsl:template name="example">
        <xsl:param name="programming_language"/>

        <h3>
            <xsl:call-template name="phrase">
                <xsl:with-param name="id" select="'example.title'"/>
            </xsl:call-template>


            <span style="font-weight: normal">
                <xsl:text>&#160;&#160;</xsl:text>
                <xsl:value-of select="@title"/>

                <xsl:if test="$programming_language">
                    <xsl:if test="@title">
                        <xsl:text>, </xsl:text>
                    </xsl:if>

                    <xsl:call-template name="phrase">
                        <xsl:with-param name="id" select="'common.in_programming_language'"/>
                    </xsl:call-template>

                    <xsl:text> </xsl:text>

                    <xsl:value-of select="$programming_language"/>
                </xsl:if>
            </span>
        </h3>

        <!--xsl:copy-of select="* | text()"/-->
        <xsl:apply-templates select="* | text()" mode="description"/>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/register-->

    <xsl:template match="/register">

        <html>
            <xsl:call-template name="html_head">
                <xsl:with-param name="title" select="@title"/>
            </xsl:call-template>

            <body>
                <xsl:call-template name="body_start">
                    <xsl:with-param name="title" select="@title"/>
                </xsl:call-template>

                <xsl:if test="description/node()">
                    <xsl:apply-templates select="description"/>
                    <p>
                        &#160;
                    </p>
                    <hr/>
                </xsl:if>

                <xsl:apply-templates select="document( 'register_data.xml' )/*"/>

                <xsl:call-template name="bottom"/>
            </body>
        </html>

    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/register_data-->

    <xsl:template match="/register_data">

        <xsl:variable name="title" select="$phrases/phrase [ @id='index.title' ]"/>

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

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~complete_register-->

    <xsl:template name="complete_register">

        <p>
            <a href="#letter__A">A&#160;</a>
            <a href="#letter__B">B&#160;</a>
            <a href="#letter__C">C&#160;</a>
            <a href="#letter__D">D&#160;</a>
            <a href="#letter__E">E&#160;</a>
            <a href="#letter__F">F&#160;</a>
            <a href="#letter__G">G&#160;</a>
            <a href="#letter__H">H&#160;</a>
            <a href="#letter__I">I&#160;</a>
            <a href="#letter__J">J&#160;</a>
            <a href="#letter__K">K&#160;</a>
            <a href="#letter__L">L&#160;</a>
            <a href="#letter__M">M&#160;</a>
            <a href="#letter__N">N&#160;</a>
            <a href="#letter__O">O&#160;</a>
            <a href="#letter__P">P&#160;</a>
            <a href="#letter__Q">Q&#160;</a>
            <a href="#letter__R">R&#160;</a>
            <a href="#letter__S">S&#160;</a>
            <a href="#letter__T">T&#160;</a>
            <a href="#letter__U">U&#160;</a>
            <a href="#letter__V">V&#160;</a>
            <a href="#letter__W">W&#160;</a>
            <a href="#letter__X">X&#160;</a>
            <a href="#letter__Y">Y&#160;</a>
            <a href="#letter__Z">Z&#160;</a>
        </p>

        <p>&#160;</p>

        <table cellspacing="0" cellpadding="0">
            <col valign="baseline"/>
            <col valign="baseline" style="padding-left: 2ex"/>

            <xsl:for-each select="register_keyword">
                <!--xsl:sort select="@keyword"/   Nicht hier sortieren, sonst funktioniert preceding-sibling:self nicht. -->

                <xsl:variable name="first_letter" select="translate( substring( @keyword, 1, 1 ), 'abcdefghijklmnopqrstuvwxyz', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ' )"/>

                <xsl:if test="$first_letter != translate( substring( (preceding-sibling::*)[ position() = last() ]/@keyword, 1, 1 ), 'abcdefghijklmnopqrstuvwxyz', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ' )">
                    <tr>
                        <td colspan="9">
                            <xsl:element name="a">
                                <xsl:attribute name="name">
                                    <xsl:text>letter__</xsl:text>
                                    <xsl:value-of select="$first_letter"/>
                                </xsl:attribute>
                            </xsl:element>
                            <xsl:if test="position() > 1">
                                &#160;<br/><!--<hr size="1" style="color: #f0f0f0"/>-->
                            </xsl:if>
                            <b>
                                <xsl:value-of select="$first_letter"/>
                            </b>
                        </td>
                    </tr>
                </xsl:if>

                <tr>
                    <td style="white-space: nowrap">
                        <xsl:choose>
                            <xsl:when test="register_keyword_display">
                                <xsl:apply-templates select="register_keyword_display/* | register_keyword_display/text()" mode="description"/>
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

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~register_data-->

    <xsl:template name="register_data">
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
                        <xsl:attribute name="href">
                            <xsl:value-of select="@register_file"/>
                        </xsl:attribute>
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:attribute name="href">
                            <xsl:value-of select="concat( @register_file, '#keyword__', @register_keyword )"/>
                        </xsl:attribute>
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
                <xsl:attribute name="href">
                    <xsl:value-of select="concat( @register_file, '#use_entry__', @file, '__', @section, '__', @entry )"/>
                </xsl:attribute>
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
                <span class="register_definition">
                    <xsl:value-of select="@register_title | @register_keyword"/>
                </span>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="@register_title | @register_keyword"/>
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

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~title-->

    <xsl:template match="title">
        <xsl:apply-templates select="node()" mode="description"/>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~description-->

    <xsl:template match="description">
        <xsl:choose>
            <xsl:when test="not( p )">    <!-- <p> vergessen hinzuschreiben? -->
                <p class="first">
                    <xsl:apply-templates select="node()" mode="description"/>
                </p>
            </xsl:when>
            <xsl:otherwise>
                <xsl:apply-templates select="node()" mode="description"/>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~description-->

    <!-- Alles kopieren, außer <scheduler_element> usw. -->
    <xsl:template match="node()" mode="description">
        <xsl:copy>
            <xsl:for-each select="@*">
                <xsl:copy>
                    <xsl:value-of select="."/>
                </xsl:copy>
            </xsl:for-each>
            <xsl:apply-templates mode="description"/>
        </xsl:copy>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~text()-->
    <!-- <description>TEXT</description> ==> <description><p>TEXT</p><description> -->
    <!--
    <xsl:template match="text() [ parent::description ]" mode="description">
        <p>
            <xsl:copy-of select="."/>
        </p>
    </xsl:template>
    -->
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~p-->
    <!-- Das erste <p> in einem <description> bekommt class="first"
         Damit fällt die Leerzeile am Anfang weg. -->

    <xsl:template match="p [ position()=1 and not( @class ) ]" mode="description">
        <xsl:element name="p">
            <xsl:attribute name="class">first</xsl:attribute>
            <xsl:for-each select="@*">
                <xsl:copy>
                    <xsl:value-of select="."/>
                </xsl:copy>
            </xsl:for-each>
            <xsl:apply-templates mode="description"/>
        </xsl:element>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~description.style-->
    <!-- Wird schon von html_head interpretiert -->

    <xsl:template match="description.style" mode="description">
        <!-- style gehört ins <head>, s. html_head -->
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_example-->

    <xsl:template match="scheduler_example" mode="description">
        <xsl:call-template name="example"/>
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

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_ignore-->
    <!-- Falls der Browser kein XML/XSLT kann -->

    <xsl:template match="scheduler_ignore" mode="description">
        <!-- Der Text wird von einem Browser, der nicht XSLT kann, anzeigt -->
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_message-->

    <xsl:template match="scheduler_message" mode="description">

        <xsl:choose>
            <xsl:when test="ancestor::p or normalize-space( preceding-sibling::text() ) != '' or normalize-space( following-sibling::text() ) != ''">
                <span class="message">
                    <xsl:call-template name="scheduler_message">
                        <xsl:with-param name="level"     select="@level"/>
                        <xsl:with-param name="code"      select="@code"/>
                        <xsl:with-param name="show_text" select="@show_text"/>
                    </xsl:call-template>
                </span>
            </xsl:when>

            <xsl:when test="parent::li">
                <div class="message">
                    <xsl:call-template name="scheduler_message">
                        <xsl:with-param name="level"     select="@level"/>
                        <xsl:with-param name="code"      select="@code"/>
                        <xsl:with-param name="show_text" select="@show_text"/>
                    </xsl:call-template>
                </div>
            </xsl:when>

            <xsl:otherwise>
                <xsl:element name="p">
                    <xsl:attribute name="class">message</xsl:attribute>
                    
                    <xsl:if test="preceding-sibling::*[1] [ local-name() = 'scheduler_message' ] or parent::li">
                        <xsl:attribute name="style">margin-top: 0em;</xsl:attribute>
                    </xsl:if>

                    <xsl:call-template name="scheduler_message">
                        <xsl:with-param name="level"     select="@level"/>
                        <xsl:with-param name="code"      select="@code"/>
                        <xsl:with-param name="show_text" select="@show_text"/>
                    </xsl:call-template>
                </xsl:element>
            </xsl:otherwise>
        </xsl:choose>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_message-->

    <xsl:template name="scheduler_message">
        <xsl:param name="level"/>
        <xsl:param name="code"/>
        <xsl:param name="show_text" select="false()"/>

        <xsl:choose>
            <xsl:when test="$level or $show_text">
                <table cellspacing="0" cellpadding="0">
                    <tr>
                        <xsl:if test="$level">
                            <td>
                                <xsl:call-template name="message_level">
                                    <xsl:with-param name="level" select="@level"/>
                                </xsl:call-template>

                                <code>
                                    <xsl:text> </xsl:text>
                                </code>
                            </td>
                        </xsl:if>

                        <td>
                            <code>&#160;</code>
                            <xsl:call-template name="scheduler_message_code">
                                <xsl:with-param name="code" select="@code"/>
                            </xsl:call-template>
                        </td>

                        <td style="padding-left: 1em">
                            <xsl:call-template name="show_message_text">
                                <xsl:with-param name="code" select="$code"/>
                            </xsl:call-template>
                        </td>
                    </tr>
                </table>
            </xsl:when>
            
            <xsl:otherwise>
                <xsl:element name="span">
                    <xsl:attribute name="title">
                        <xsl:call-template name="show_message_text">
                            <xsl:with-param name="code" select="$code"/>
                        </xsl:call-template>
                    </xsl:attribute>

                    <xsl:call-template name="scheduler_message_code">
                        <xsl:with-param name="code" select="@code"/>
                    </xsl:call-template>
                </xsl:element>
            </xsl:otherwise>
        </xsl:choose>
    
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_message-->

    <xsl:template name="scheduler_message_code">
        <xsl:param name="code"/>

        <xsl:element name="a">
            <xsl:attribute name="href">
                <xsl:value-of select="/*/@base_dir"/>
                <xsl:text>messages/index.xml#message_</xsl:text>
                <xsl:value-of select="$code"/>
            </xsl:attribute>
            
            <xsl:element name="code">
                <xsl:attribute name="style">white-space: nowrap</xsl:attribute>
            
                <xsl:value-of select="$code"/>
            </xsl:element>
        </xsl:element>
        
    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~show_message_text-->

    <xsl:template name="show_message_text">
        <xsl:param name="code"/>
        <xsl:apply-templates select="document('messages/scheduler_messages.xml')/messages//message[ @code=$code ]/text[ @xml:lang='en' ]/title"/>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_ignore-->
    <!--
    <xsl:template match="scheduler_see" mode="description">
        <p>
            Siehe
            <xsl:apply-templates mode="description"/>
        </p>
    </xsl:template>
    -->
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_element-->

    <xsl:template match="scheduler_element" mode="description">
        <xsl:call-template name="scheduler_element">
            <xsl:with-param name="directory" select="@directory"/>
            <xsl:with-param name="name"      select="@name"     />
            <xsl:with-param name="attribute" select="@attribute"/>
            <xsl:with-param name="relation"  select="@relation"/>
            <xsl:with-param name="value"     select="@value"    />
            <xsl:with-param name="parameter" select="@parameter"/>
            <xsl:with-param name="child"     select="@child"/>
            <xsl:with-param name="content"   select="node()"/>
        </xsl:call-template>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_element-->

    <xsl:template name="scheduler_element">
        <xsl:param name="name"/>
        <xsl:param name="directory"/>
        <xsl:param name="attribute"/>
        <xsl:param name="relation"/>
        <xsl:param name="value"     select="'…'"/>
        <xsl:param name="parameter"/>
        <xsl:param name="child"/>
        <xsl:param name="content"/>

        <xsl:variable name="my_directory">
            <xsl:choose>
                <xsl:when test="$directory">
                    <xsl:value-of select="$directory"/>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:text>xml/</xsl:text>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:variable>

        <xsl:element name="a">
            <xsl:attribute name="name">
                <xsl:text>use_element__</xsl:text>
                <xsl:value-of select="$name"/>
            </xsl:attribute>
        </xsl:element>

        <xsl:element name="a">
            <xsl:attribute name="class">silent</xsl:attribute>

            <xsl:variable name="href2">
                <xsl:choose>
                    <xsl:when test="$attribute">
                        <xsl:text>#attribute_</xsl:text>
                        <xsl:value-of select="$attribute"/>
                    </xsl:when>
                    <xsl:when test="$child">
                        <xsl:text>#element_</xsl:text>
                        <xsl:value-of select="$child"/>
                    </xsl:when>
                </xsl:choose>
            </xsl:variable>

            <xsl:attribute name="href">
                <xsl:value-of select="concat( $base_dir, $my_directory, $name, '.xml', $href2 )"/>
            </xsl:attribute>

            <xsl:attribute name="title">
                <xsl:value-of select="$phrases/phrase [ @id='xml_element.chapter_title.prefix' ]"/>
                
                <xsl:choose>
                    <xsl:when test="$content">
                        <xsl:call-template name="scheduler_element_text">
                            <xsl:with-param name="name"      select="$name"/>
                            <xsl:with-param name="attribute" select="$attribute"/>
                            <xsl:with-param name="relation"  select="$relation"/>
                            <xsl:with-param name="value"     select="$value"/>
                            <xsl:with-param name="parameter" select="$parameter"/>
                            <xsl:with-param name="child"     select="$child"/>
                        </xsl:call-template>
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:text> &lt;</xsl:text>
                        <xsl:value-of select="$name"/>
                        <xsl:text>></xsl:text>
                    </xsl:otherwise>
                </xsl:choose>
                
                <xsl:variable name="xml_element" select="document( concat( $my_directory, $name, '.xml' ) )/xml_element"/>
                <xsl:variable name="title" select="normalize-space( $xml_element/@title )"/>

                <xsl:if test="$title">
                    <xsl:text>  —  </xsl:text>
                    <xsl:value-of select="$title"/>
                </xsl:if>

                <xsl:if test="$attribute">
                    <xsl:variable name="attribute_title" select="$xml_element/xml_attributes/xml_attribute [ @name=$attribute or @setting=$attribute ]/@title"/>
                    <xsl:if test="$attribute_title">
                        <xsl:text>; </xsl:text>
                        <xsl:value-of select="$attribute_title"/>
                    </xsl:if>
                </xsl:if>
            </xsl:attribute>

            <xsl:choose>
                <xsl:when test="$content">
                    <xsl:copy-of select="$content"/>
                </xsl:when>
                <xsl:otherwise>
                    <code>
                        <xsl:call-template name="scheduler_element_text">
                            <xsl:with-param name="name"      select="$name"/>
                            <xsl:with-param name="attribute" select="$attribute"/>
                            <xsl:with-param name="relation"  select="$relation"/>
                            <xsl:with-param name="value"     select="$value"/>
                            <xsl:with-param name="parameter" select="$parameter"/>
                            <xsl:with-param name="child"     select="$child"/>
                        </xsl:call-template>
                    </code>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:element>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_element_text-->
    
    <xsl:template name="scheduler_element_text">
        <xsl:param name="name"/>
        <xsl:param name="attribute"/>
        <xsl:param name="relation"/>
        <xsl:param name="value"/>
        <xsl:param name="parameter"/>
        <xsl:param name="child"/>

        <xsl:text>&lt;</xsl:text>
        <xsl:value-of select="$name"/>

        <xsl:if test="$attribute">
            <xsl:text> </xsl:text>
            <xsl:value-of select="$attribute"/>
            <xsl:choose>
                <xsl:when test="$relation">
                    <xsl:text>&#x2005;</xsl:text>
                    <xsl:value-of select="$relation"/>
                    <xsl:text>&#x2005;</xsl:text>
                </xsl:when>
                <xsl:otherwise>=</xsl:otherwise>
            </xsl:choose>
            <xsl:text>"</xsl:text>
            <xsl:choose>
                <xsl:when test="$value">
                    <xsl:value-of select="$value"/>
                </xsl:when>
                <xsl:otherwise>…</xsl:otherwise>
            </xsl:choose>
            <xsl:text>"</xsl:text>
        </xsl:if>

        <xsl:if test="$parameter">
            <xsl:text> </xsl:text>
            <xsl:value-of select="$parameter"/>
        </xsl:if>

        <xsl:text>></xsl:text>

        <xsl:if test="$child">
            <xsl:text>&lt;</xsl:text>
            <xsl:value-of select="$child"/>
            <xsl:text>></xsl:text>
        </xsl:if>

    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_setting-->

    <xsl:template match="scheduler_setting" mode="description">

        <xsl:element name="a">
            <xsl:attribute name="href">
                <xsl:text>settings.xml#option_</xsl:text>
                <xsl:value-of select="@setting | @name"/>
            </xsl:attribute>

            <xsl:choose>
                <xsl:when test="ancestor::ini_entry">
                    <code>
                        <xsl:value-of select="@name"/>=
                    </code>
                </xsl:when>
                <xsl:when test="ancestor::command_option">
                    <code>
                        -<xsl:value-of select="@name"/>=
                    </code>
                </xsl:when>
                <xsl:otherwise>
                    <code>
                        <xsl:value-of select="@name | @setting"/>=
                    </code>
                </xsl:otherwise>
            </xsl:choose>

            <xsl:choose>
                <xsl:when test="@value">
                    <code>
                        <xsl:value-of select="@value"/>
                    </code>
                </xsl:when>
                <xsl:otherwise>
                    <span class="type">
                        <xsl:value-of select="@type"/>
                    </span>
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
            <xsl:with-param name="value"   select="@value"/>
        </xsl:call-template>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_ini_entry-->

    <xsl:template name="scheduler_ini_entry">
        <xsl:param name="file"/>
        <xsl:param name="section"/>
        <xsl:param name="entry"/>
        <xsl:param name="value"/>
        <xsl:param name="show_entry_only" select="false()"/>

        <xsl:element name="a">
            <xsl:attribute name="name">
                <xsl:text>use_entry__</xsl:text>
                <xsl:value-of select="$file"/>__<xsl:value-of select="$section"/>__<xsl:value-of select="$entry"/>
            </xsl:attribute>
        </xsl:element>

        <xsl:element name="a">
            <xsl:attribute name="class">silent</xsl:attribute>
            <xsl:attribute name="href">
                <xsl:value-of select="concat( $base_dir, translate( $file, '.', '_' ), '_', $section, '.xml', '#entry_', $entry )"/>
            </xsl:attribute>

            <xsl:if test="not( $show_entry_only )">
                <code>
                    <xsl:value-of select="$file"/>
                </code>

                <xsl:text> (</xsl:text>

                <xsl:call-template name="phrase">
                    <xsl:with-param name="id" select="'ini.section'"/>
                </xsl:call-template>

                <xsl:text> </xsl:text>
                <code>
                    <xsl:text>[</xsl:text>
                    <xsl:value-of select="$section"/>
                    <xsl:text>]</xsl:text>
                </code>
            </xsl:if>

            <xsl:if test="$entry">
                <xsl:if test="not( $show_entry_only )">
                    <xsl:text>, </xsl:text>

                    <xsl:call-template name="phrase">
                        <xsl:with-param name="id" select="'ini.entry'"/>
                    </xsl:call-template>

                    <xsl:text> </xsl:text>
                </xsl:if>
                
                <code>
                    <xsl:value-of select="$entry"/>
                    <xsl:text>=</xsl:text>
                    <xsl:choose>
                        <xsl:when test="$value">
                            <xsl:value-of select="$value"/>
                        </xsl:when>
                        <xsl:otherwise>
                            <xsl:text>…</xsl:text>
                        </xsl:otherwise>
                    </xsl:choose>
                </code>
            </xsl:if>

            <xsl:if test="not( $show_entry_only )">
                <xsl:text>)</xsl:text>
            </xsl:if>
        </xsl:element>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_option-->

    <xsl:template match="scheduler_option" mode="description">
        <xsl:call-template name="scheduler_option">
            <xsl:with-param name="name"  select="@name"/>
            <xsl:with-param name="value" select="@value"/>
            <xsl:with-param name="content"  select="node()"/>
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
        <xsl:param name="value"/>
        <xsl:param name="content"/>

        <xsl:element name="a">
            <xsl:attribute name="name">
                <xsl:text>use_option__</xsl:text>
                <xsl:value-of select="$name"/>
            </xsl:attribute>
        </xsl:element>

        <xsl:element name="a">
            <xsl:attribute name="class">silent</xsl:attribute>
            
            <xsl:attribute name="href">
                <xsl:value-of select="concat( $base_dir, 'command_line.xml#option_', $name )"/>
            </xsl:attribute>

            <xsl:choose>
                <xsl:when test="$content">
                    <xsl:attribute name="title">
                        <xsl:value-of select="$phrases/phrase [ @id='command_line.option.reference' ]"/>
                        <xsl:text> -</xsl:text>
                        <xsl:value-of select="$name"/>
                        <xsl:if test="$value">
                            <xsl:text>=</xsl:text>
                            <xsl:value-of select="$value"/>
                        </xsl:if>
                    </xsl:attribute>
                    <xsl:copy-of select="$content"/>
                </xsl:when>

                <xsl:otherwise>
                    <xsl:variable name="option_element" select="document( 'command_line.xml' )/command_line/command_options/command_option [ @name=$name ]"/>

                    <xsl:element name="span">
                        <xsl:attribute name="title">
                            <xsl:value-of select="normalize-space( $option_element/@title )"/>
                        </xsl:attribute>
                        
                        <code>
                            <xsl:text>-</xsl:text>
                            <xsl:value-of select="$name"/>
                        </code>

                        <xsl:if test="$value">
                            <code>
                                <xsl:text>=</xsl:text>
                                <xsl:value-of select="$value"/>
                            </code>
                        </xsl:if>
                    </xsl:element>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:element>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_option_display-->

    <xsl:template name="scheduler_option_display">
        <xsl:param name="name"/>
        <xsl:param name="value"/>
        
        <code>
            <xsl:text>-</xsl:text>
            <xsl:value-of select="$name"/>
        </code>

        <xsl:if test="$value">
            <code>
                <xsl:text>=</xsl:text>
                <xsl:value-of select="$value"/>
            </code>
        </xsl:if>

    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_method-->

    <xsl:template match="scheduler_method" mode="description">
        <xsl:call-template name="scheduler_method">
            <xsl:with-param name="class"            select="@class"/>
            <xsl:with-param name="object"           select="@object"/>
            <xsl:with-param name="method"           select="@method"/>
            <xsl:with-param name="property"         select="@property"/>
            <xsl:with-param name="java_signature"   select="@java_signature"/>
            <xsl:with-param name="access"           select="@access"/>
            <xsl:with-param name="value"            select="@value"/>
            <xsl:with-param name="programming_language" select="@programming_language"/>
        </xsl:call-template>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_method-->

    <xsl:template name="scheduler_method">
        <xsl:param name="class"          select="/.."/>
        <xsl:param name="object"         select="/.."/>
        <xsl:param name="method"         select="/.."/>
        <xsl:param name="property"       select="/.."/>
        <xsl:param name="java_signature" select="/.."/>
        <xsl:param name="access"         select="/.."/>
        <!-- "read" (default), "write" -->
        <xsl:param name="value"          select="/.."/>
        <xsl:param name="programming_language" select="$selected_programming_language"/>

        <xsl:variable name="plang">
            <xsl:choose>
                <xsl:when test="$programming_language">
                    <xsl:value-of select="$programming_language"/>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:value-of select="$selected_programming_language"/>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:variable>
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

            <xsl:variable name="href_local">
                <xsl:if test="$method or $property">
                    <xsl:text>#method__</xsl:text>
                    <xsl:value-of select="$method | $property"/>
                </xsl:if>
            </xsl:variable>

            <xsl:attribute name="href">
                <xsl:value-of select="concat( $base_dir, 'api/', $class, '-', $plang, '.xml', $href_local )"/>
            </xsl:attribute>

            <xsl:element name="code">
                <xsl:variable name="class_element" select="document( concat( 'api/', $class, '.xml' ) )/api.class"/>

                <xsl:attribute name="title">
                    <xsl:choose>
                        <xsl:when test="$method">
                            <xsl:value-of select="normalize-space( $class_element/method [ @name=$method ]/title )"/>
                        </xsl:when>
                        <xsl:when test="$property">
                            <xsl:value-of select="normalize-space( $class_element/property [ @name=$property ]/title )"/>
                        </xsl:when>
                        <xsl:otherwise>
                            <xsl:value-of select="normalize-space( $class_element/title )"/>
                        </xsl:otherwise>
                    </xsl:choose>
                </xsl:attribute>

                <xsl:choose>
                    <xsl:when test="$object">
                        <xsl:value-of select="$object"/>
                    </xsl:when>
                    <xsl:when test="$class != 'Job_impl' or not( $method or $property )">
                        <xsl:value-of select="$class"/>
                    </xsl:when>
                </xsl:choose>

                <xsl:if test="$class != 'Job_impl' and ( $method or $property ) and not( $object = '' )">
                    <xsl:text>.</xsl:text>
                </xsl:if>

                <xsl:if test="$method">
                    <xsl:value-of select="$method"/>
                    <xsl:text>(</xsl:text>
                    <xsl:if test="$value">
                        <xsl:text> </xsl:text>
                        <xsl:value-of select="$value"/>
                        <xsl:text> </xsl:text>
                    </xsl:if>
                    <xsl:text>)</xsl:text>
                </xsl:if>

                <xsl:if test="$property">
                    <xsl:value-of select="$property"/>
                </xsl:if>

            </xsl:element>
        </xsl:element>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_element-->

    <xsl:template match="scheduler_a" mode="description">
        <xsl:call-template name="scheduler_a">
            <xsl:with-param name="href" select="@href"/>
            <xsl:with-param name="quote" select="@quote='yes'"/>
            <xsl:with-param name="content" select="node()"/>
            <xsl:with-param name="base_dir" select="''"/>
        </xsl:call-template>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_element-->

    <xsl:template name="scheduler_a">
        <xsl:param name="href"/>
        <xsl:param name="quote"/>
        <xsl:param name="content"/>
        <xsl:param name="base_dir" select="/*/@base_dir"/>

        <xsl:variable name="root_element" select="document( $href )/*"/>
        <xsl:variable name="root_title"   select="normalize-space( $root_element/@title )"/>

        <xsl:choose>
            <xsl:when test="local-name( $root_element ) = 'xml_element'">
                <xsl:call-template name="scheduler_element">
                    <xsl:with-param name="name" select="$root_element/@name"/>
                </xsl:call-template>
            </xsl:when>

            <xsl:when test="local-name( $root_element ) = 'api.class' and $root_element/@name != 'api'">
                <xsl:call-template name="scheduler_method">
                    <xsl:with-param name="class" select="$root_element/@name"/>
                </xsl:call-template>
            </xsl:when>

            <xsl:when test="local-name( $root_element ) = 'ini_section'">
                <xsl:call-template name="scheduler_method">
                    <xsl:with-param name="file" select="$root_element/@file"/>
                    <xsl:with-param name="section" select="$root_element/@name"/>
                </xsl:call-template>
            </xsl:when>

            <xsl:otherwise>
                <xsl:if test="$quote">
                    <xsl:text>»</xsl:text>
                </xsl:if>

                <xsl:element name="a">
                    <xsl:attribute name="href">
                        <xsl:value-of select="$base_dir"/>
                        <xsl:value-of select="$href"/>
                    </xsl:attribute>

                    <xsl:choose>
                        <xsl:when test="$content">
                            <xsl:attribute name="title">
                                <xsl:copy-of select="$root_title"/>
                            </xsl:attribute>
                            <xsl:copy-of select="$content"/>
                        </xsl:when>

                        <xsl:when test="$root_title">
                            <xsl:copy-of select="$root_title"/>
                        </xsl:when>
                        <xsl:otherwise>
                            <code><xsl:value-of select="$href"/></code>
                        </xsl:otherwise>
                    </xsl:choose>
                </xsl:element>

                <xsl:if test="$quote">
                    <xsl:text>«</xsl:text>
                </xsl:if>
            </xsl:otherwise>
        </xsl:choose>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_db-->

    <xsl:template match="scheduler_db" mode="description">
        <xsl:call-template name="scheduler_db">
            <xsl:with-param name="table" select="@table"/>
            <xsl:with-param name="column" select="@column"/>
        </xsl:call-template>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_db-->

    <xsl:template name="scheduler_db">
        <xsl:param name="table"/>
        <xsl:param name="column"/>

        <code>
            <xsl:value-of select="$table"/>

            <xsl:if test="$column">
                <xsl:text>.</xsl:text>
                <xsl:value-of select="$column"/>
            </xsl:if>
        </code>
        
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_env-->

    <xsl:template match="scheduler_env" mode="description">
        <xsl:call-template name="scheduler_env">
            <xsl:with-param name="name" select="@name"/>
        </xsl:call-template>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_env-->

    <xsl:template name="scheduler_env">
        <xsl:param name="name"/>

        <code>
            <xsl:value-of select="$name"/>
        </code>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_error-->

    <xsl:template match="scheduler_error" mode="description">

        <code>
            <xsl:value-of select="@code"/>
        </code>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_keyword-->

    <xsl:template match="scheduler_keyword" mode="description">

        <xsl:element name="a">
            <xsl:attribute name="name">
                <xsl:text>keyword__</xsl:text>
                <xsl:value-of select="@keyword"/>
            </xsl:attribute>
        </xsl:element>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~messages-->

    <xsl:template match="messages [ message ]">

        <xsl:param name="h" select="'h2'"/>
        <xsl:param name="show_level" select="true()"/>

        <xsl:choose>
            <xsl:when test="$h='h2'">
                <h2>
                    <xsl:call-template name="phrase">
                        <xsl:with-param name="id" select="'messages.title'"/>
                    </xsl:call-template>
                </h2>
            </xsl:when>
            <xsl:when test="$h='h3'">
                <h3>
                    <xsl:call-template name="phrase">
                        <xsl:with-param name="id" select="'messages.title'"/>
                    </xsl:call-template>
                </h3>
            </xsl:when>
            <xsl:when test="$h='h4'">
                <h4>
                    <xsl:call-template name="phrase">
                        <xsl:with-param name="id" select="'messages.title'"/>
                    </xsl:call-template>
                </h4>
            </xsl:when>
            <xsl:when test="not( $h ) or $h=''">
            </xsl:when>
            <xsl:otherwise>
                <xsl:message terminate="yes">Template messages: Parameter h=h2|h3|h4 erlaubt, aber h=<xsl:value-of select="$h"/>
            </xsl:message>
            </xsl:otherwise>
        </xsl:choose>

        <xsl:apply-templates select="." mode="without_title">
            <xsl:with-param name="show_level" select="$show_level"/>
        </xsl:apply-templates>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~messages-->

    <xsl:template match="messages [ message ]" mode="without_title">

        <xsl:param name="show_level"/>

        <xsl:call-template name="messages">
            <xsl:with-param name="message_set" select="message"/>
            <xsl:with-param name="show_level" select="$show_level"/>
        </xsl:call-template>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~messages-->

    <xsl:template name="messages">

        <xsl:param name="show_level"/>
        <xsl:param name="message_set"/>

        <div class="message">
            <table cellspacing="0" cellpadding="0">
                <xsl:apply-templates select="$message_set[ @level='error' ]">
                    <xsl:sort select="@code"/>
                    <xsl:with-param name="show_level" select="$show_level"/>
                </xsl:apply-templates>

                <xsl:apply-templates select="$message_set[ @level='warn' ]">
                    <xsl:sort select="@code"/>
                    <xsl:with-param name="show_level" select="$show_level"/>
                </xsl:apply-templates>

                <xsl:apply-templates select="$message_set[ @level='info' ]">
                    <xsl:sort select="@code"/>
                    <xsl:with-param name="show_level" select="$show_level"/>
                </xsl:apply-templates>

                <xsl:apply-templates select="$message_set[ starts-with( @level, 'debug') ]">
                    <xsl:sort select="@level"/>
                    <xsl:sort select="@code"/>
                    <xsl:with-param name="show_level" select="$show_level"/>
                </xsl:apply-templates>
            </table>
        </div>
        
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~message-->

    <xsl:template match="messages/message">
        <xsl:param name="show_level"/>

        <tr>
            <xsl:if test="$show_level">
                <td style="padding-right: 1ex">
                    <xsl:call-template name="message_level">
                        <xsl:with-param name="level" select="@level"/>
                    </xsl:call-template>
                </td>
            </xsl:if>

            <td>
                <xsl:call-template name="scheduler_message">
                    <xsl:with-param name="code" select="@code"/>
                </xsl:call-template>
            </td>

            <td style="padding-left: 2ex">
                <xsl:call-template name="show_message_text">
                    <xsl:with-param name="code" select="@code"/>
                </xsl:call-template>
                
                <xsl:text>&#160;</xsl:text> <!-- Ein Zeichen <code>, um Zeilenhöhe zu halten -->
                <br/>
                
                <xsl:apply-templates select="description"/>
            </td>
        </tr>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~message_level-->

    <xsl:template name="message_level">
        <xsl:param name="level"/>
                  
        <xsl:if test="@level">
            <code>
                <xsl:choose>
                    <xsl:when test="@level='info'">
                        <code class="message_info">[info]</code>
                    </xsl:when>
                    <xsl:when test="@level='warn'">
                        <code class="message_warn">[warn]</code>
                    </xsl:when>
                    <xsl:when test="@level='error'">
                        <code class="message_error">[ERROR]</code>
                    </xsl:when>
                    <xsl:otherwise>
                        <code>
                            [<xsl:value-of select="@level"/>]
                        </code>
                    </xsl:otherwise>
                </xsl:choose>
            </code>
        </xsl:if>
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

                <xsl:apply-templates select="description"/>

                <xsl:if test="count( command_options ) &gt; 1">
                    <h2>
                        <xsl:call-template name="phrase">
                            <xsl:with-param name="id" select="'command_line.variants.title'"/>
                        </xsl:call-template>
                    </h2>
                    <ul>
                        <xsl:for-each select="command_options">
                            <li>
                                <xsl:element name="a">
                                    <xsl:attribute name="href">#options__<xsl:value-of select="@name"/></xsl:attribute>
                                    <xsl:value-of select="@title"/>
                                </xsl:element>
                            </li>
                        </xsl:for-each>
                    </ul>
                </xsl:if>

                <xsl:apply-templates select="command_options" mode="table"/>

                <h2>
                    <xsl:call-template name="phrase">
                        <xsl:with-param name="id" select="'command_line.options.title'"/>
                    </xsl:call-template>
                </h2>
                
                <xsl:apply-templates select="command_options/command_option">
                    <xsl:sort select="@name | @setting"/>
                </xsl:apply-templates>

                <xsl:call-template name="bottom"/>
            </body>
        </html>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~command_options-->

    <xsl:template match="command_options" mode="table">

        <xsl:if test="@title">
            <h2>
                <xsl:element name="a">
                    <xsl:attribute name="name">options__<xsl:value-of select="@name"/></xsl:attribute>
                </xsl:element>

                <xsl:value-of select="@title"/>
            </h2>
        </xsl:if>
        
        <p>
            <code>
                <xsl:value-of select="parent::*/@program"/>
            </code>
        </p>

        <div class="indent">
            <table cellspacing="0" cellpadding="0">
                <col/>
                <col/>
                <xsl:for-each select="command_option">
                    <xsl:variable name="command_option" select="parent::command_options/parent::*/command_options/command_option [ @name = current()/@name and ( @type=current()/@type or not(@type) and not(current()/@type) ) and not( @reference='yes' ) ]"/>
                    <xsl:variable name="setting" select="document( 'settings.xml' )/settings/setting[ @setting = $command_option/@setting ]"/>

                    <tr>
                        <td valign="baseline">
                            <xsl:element name="a">
                                <xsl:attribute name="class">silent</xsl:attribute>
                                <xsl:attribute name="href">#option_<xsl:value-of select="@name"/></xsl:attribute>
                                <code>-<xsl:value-of select="@name"/></code>
                            </xsl:element>

                            <xsl:if test="$command_option/@type | $setting/@type">
                                <code>=</code><span class="type"><xsl:value-of select="$command_option/@type | $setting/@type"/></span>
                            </xsl:if>
                        </td>
                        <td valign="baseline" style="padding-left: 4ex">
                            <span class="title">
                                <xsl:value-of select="$command_option/@title | $setting/@title"/>
                            </span>
                        </td>
                    </tr>
                </xsl:for-each>
            </table>
        </div>
        
        <p>&#160;</p>
        <xsl:apply-templates select="description"/>
        <xsl:apply-templates select="example"/>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~command_option-->

    <xsl:template match="command_option [ not( @reference='yes') ]">

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

        <xsl:if test="@multiple='yes'">
            <div class="indent">
                <p>
                    <xsl:call-template name="phrase">
                        <xsl:with-param name="id" select="'command_line.option.multiple'"/>
                    </xsl:call-template>
                </p>
            </div>
        </xsl:if>

        <!--div class="indent">
            <xsl:apply-templates select="messages">
                <xsl:with-param name="h" select="'h4'"/>
                <xsl:with-param name="show_level" select="true()"/>
            </xsl:apply-templates>
        </div-->

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

                <xsl:call-template name="phrase">
                    <xsl:with-param name="id" select="'ini.file.location_remark'"/>
                </xsl:call-template>

                <h2>
                    <xsl:call-template name="phrase">
                        <xsl:with-param name="id" select="'ini.sections.title'"/>
                    </xsl:call-template>
                </h2>

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
            <col valign="baseline"/>

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
                    <td style="padding-left: 4ex">
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

        <xsl:variable name="title"       select="concat( $phrases/phrase [ @id='ini.file.title' ]/text(), ' ', @file, ',&#160; ', $phrases/phrase [ @id='ini.section' ]/text(), '&#160; [', @name, ']' )"/>
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

                <h2>
                    <xsl:call-template name="phrase">
                        <xsl:with-param name="id" select="'ini.entries.title'"/>
                    </xsl:call-template>
                </h2>
                
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
                        <td style="width: 300px">
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

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_all_messages-->

    <xsl:template match="/scheduler_all_messages">

        <html>
            <xsl:call-template name="html_head">
                <xsl:with-param name="title" select="@title"/>
            </xsl:call-template>

            <body>
                <xsl:call-template name="body_start">
                    <xsl:with-param name="title"       select="@title"/>
                    <!--xsl:with-param name="parent_page" select="@parent_page"/-->
                </xsl:call-template>

                <xsl:for-each select="scheduler_messages">
                    <xsl:apply-templates select="document(@path)/messages" mode="inner"/>
                </xsl:for-each>

                <xsl:call-template name="bottom">
                    <xsl:with-param name="title"       select="@title"/>
                    <xsl:with-param name="parent_page" select="@parent_page"/>
                </xsl:call-template>
            </body>
        </html>
        
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~messages-->
    <!--
    <xsl:template match="/messages">

        <html>
            <xsl:call-template name="html_head">
                <xsl:with-param name="title" select="concat( 'Messages of package ', @package )"/>
            </xsl:call-template>

            <body>
                <xsl:call-template name="body_start">
                </xsl:call-template>

                <xsl:apply-templates select="." mode="inner"/>
                
                <xsl:call-template name="bottom">
                    <xsl:with-param name="title"       select="@title"/>
                    <xsl:with-param name="parent_page" select="@parent_page"/>
                </xsl:call-template>
            </body>
        </html>

    </xsl:template>
    -->
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~messages-->

    <xsl:template match="messages" mode="inner">

        <h2>Messages of package <xsl:value-of select="@package"/></h2>
        
        <table cellpadding="0" cellspacing="0">
            <thead>
                <tr>
                    <td style="padding-right: 1ex;">Code</td>
                    <td style="padding-left: 1ex; padding-right: 1ex;"></td>
                    <td style="padding-left: 1ex; padding-right: 1ex;">Text</td>
                </tr>
                <tr>
                    <td colspan="99" style="border-bottom: 1px solid black; line-height: 1pt">
                        &#160;
                    </td>
                </tr>
            </thead>
            <tbody>
                <xsl:apply-templates select=".//message" mode="tr"/>
            </tbody>
        </table>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~messages-->

    <xsl:template match="/message">

        <html>
            <xsl:call-template name="html_head">
                <xsl:with-param name="title" select="concat( 'Message ', @code )"/>
            </xsl:call-template>

            <body>
                <xsl:call-template name="body_start">
                </xsl:call-template>

                <table cellpadding="0" cellspacing="0">
                    <thead>
                        <tr>
                            <td style="padding-right: 1ex;">Code</td>
                            <td style="padding-left: 1ex; padding-right: 1ex;"></td>
                            <td style="padding-left: 1ex; padding-right: 1ex;">Text</td>
                        </tr>
                        <tr>
                            <td colspan="99" style="border-bottom: 1px solid black; line-height: 1pt">
                                &#160;
                            </td>
                        </tr>
                    </thead>
                    <tbody>
                        <xsl:apply-templates select="." mode="tr"/>
                    </tbody>
                </table>

                <xsl:call-template name="bottom">
                    <xsl:with-param name="title"       select="@title"/>
                    <xsl:with-param name="parent_page" select="@parent_page"/>
                </xsl:call-template>
            </body>
        </html>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~message-->

    <xsl:template match="message" mode="tr">

        <tr>
            <td class="message" style="padding-right: 1ex;">
                <xsl:element name="a">
                    <xsl:attribute name="name">message_<xsl:value-of select="@code"/></xsl:attribute>
                </xsl:element>

                <code style="white-space: nowrap">
                    <xsl:value-of select="@code"/>
                </code>
            </td>
            <td class="message" style="padding-left: 1ex; padding-right: 1ex;">
                en:
            </td>
            <td class="message" style="padding-left: 1ex; padding-right: 1ex;">
                <xsl:apply-templates select="text[ @xml:lang='en' ]/title"/>
            </td>
        </tr>
        
        <xsl:if test="text[ @xml:lang='en' ]/description">
            <tr>
                <td></td>
                <td></td>
                <td style="padding-left: 1ex; padding-right: 1ex;">
                    <div style="margin-top: 6pt;">
                        <xsl:apply-templates select="text[ @xml:lang='en' ]/description"/>
                    </div>
                </td>
            </tr>
        </xsl:if>

        <xsl:if test="text[ @xml:lang='de' ]">
            <tr>
                <td> </td>
                <td style="padding-left: 1ex; padding-right: 1ex;"><!--background-color: #f8f8ff;"-->
                    de:
                </td>
                <td style="padding-left: 1ex; padding-right: 1ex;"><!--background-color: #f8f8ff;"-->
                    <xsl:apply-templates select="text[ @xml:lang='de' ]"/>
                </td>
            </tr>
            
            <xsl:if test="description">
                <tr>
                    <td></td>
                    <td></td>
                    <td style="padding-left: 1ex; padding-right: 1ex;">
                        <div style="margin-top: 6pt;">
                            <xsl:apply-templates select="description"/>
                        </div>
                    </td>
                </tr>
            </xsl:if>
        </xsl:if>


        <xsl:variable name="files" select="document( 'files.xml' )/files/file 
                                            [    document( @path )//scheduler_message [ @code=current()/@code ]
                                              or document( @path )//message           [ @code=current()/@code ] ]"/>
        <xsl:if test="$files">
            <tr>
                <td></td>
                <td style="padding-left: 1ex;">
                    &#x2192;
                </td>
                <td colspan="99" style="padding-left: 1ex; padding-right: 1ex;">
                    <xsl:variable name="my_root" select="/*"/>
                    <xsl:for-each select="$files">
                        <xsl:call-template name="scheduler_a">
                            <xsl:with-param name="href" select="@path"/>
                            <xsl:with-param name="base_dir" select="$my_root/@base_dir"/>
                            <xsl:with-param name="quote" select="'yes'"/>
                        </xsl:call-template>

                        <!--xsl:variable name="references" select="document( @path )/*/scheduler_message [ @code=current()/@code ]
                                                                 | document( @path )/*/message           [ @code=current()/@code ] "/-->
                        
                        <xsl:if test="position() &lt; last()">
                            <xsl:text>, &#160; </xsl:text>
                        </xsl:if>
                    </xsl:for-each>
                </td>
            </tr>
        </xsl:if>

        <xsl:if test="position() != last()">
            <tr>
                <td colspan="99" style="border-bottom: 1px solid white; line-height: 4pt">
                    &#160;
                </td>
            </tr>
        </xsl:if>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~message/text-->

    <xsl:template match="message/text">
        <xsl:apply-templates select="title"/>

        <xsl:if test="description">
            <div style="margin-top: 6pt;">
                <xsl:apply-templates select="description"/>
            </div>
        </xsl:if>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~message/title-->

    <xsl:template match="message/text/title">

        <xsl:apply-templates select="node()" mode="message"/>

        <!--xsl:for-each select="node()">
            <xsl:choose>
                <xsl:when test="self::p1"><i style="font-size: 9pt"><xsl:value-of select="@what"/>(1)</i></xsl:when>
                <xsl:when test="self::p2"><i style="font-size: 9pt"><xsl:value-of select="@what"/>(2)</i></xsl:when>
                <xsl:when test="self::p3"><i style="font-size: 9pt"><xsl:value-of select="@what"/>(3)</i></xsl:when>
                <xsl:when test="self::p4"><i style="font-size: 9pt"><xsl:value-of select="@what"/>(4)</i></xsl:when>
                <xsl:otherwise>
                    <xsl:copy-of select="."/>
                </xsl:otherwise>
            </xsl:choose>
        </xsl:for-each-->
        
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~mode="message"-->

    <xsl:template match="node()" mode="message">
        <xsl:copy>
            <xsl:for-each select="@*">
                <xsl:copy>
                    <xsl:value-of select="."/>
                </xsl:copy>
            </xsl:for-each>
            <xsl:apply-templates mode="message"/>
        </xsl:copy>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~p1-->
    
    <xsl:template match="p1" mode="message">
        <span class="message_insertion"><xsl:copy-of select="node()"/>(1)</span>
    </xsl:template>

    <xsl:template match="p2" mode="message">
        <span class="message_insertion"><xsl:copy-of select="node()"/>(2)</span>
    </xsl:template>

    <xsl:template match="p3" mode="message">
        <span class="message_insertion"><xsl:copy-of select="node()"/>(3)</span>
    </xsl:template>

    <xsl:template match="p4" mode="message">
        <span class="message_insertion"><xsl:copy-of select="node()"/>(4)</span>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_link_to_change_log-->

    <xsl:template match="scheduler_link_to_change_log" mode="description">

        <xsl:call-template name="scheduler_a">
            <xsl:with-param name="href">CHANGES.xml</xsl:with-param>
        </xsl:call-template>

        <xsl:text> </xsl:text>
        <xsl:variable name="change" select="document('CHANGES.xml')/changes/change[ position() = 1 ]"/>
        <!--xsl:value-of select="$change/@version"/>
        <xsl:text>.</xsl:text>
        <xsl:value-of select="$change/@subversion_revision"/-->
        <xsl:text> (</xsl:text>            
        <xsl:value-of select="$change/@date"/>
        <xsl:text>)</xsl:text>
        
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~html_head-->

    <xsl:template name="html_head">
        <xsl:param name="title"/>

        <head>
            <title>
                <xsl:text>Scheduler: </xsl:text>
                <xsl:value-of select="$title"/>
            </title>

            <style type="text/css">
                <xsl:text>@import "</xsl:text>
                <xsl:value-of select="$base_dir"/>
                <xsl:text>scheduler.css";</xsl:text>
                
                <xsl:if test="/*/@document_state='work_in_progress' or /*/@document_state='proposal'">
                    <xsl:text>body { color: #404040; background-color: #e8ffe8; }</xsl:text>
                </xsl:if>
                
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
                        
                        <!--&#160; &#160;
                        <xsl:element name="img">
                            <xsl:attribute name="alt">logo</xsl:attribute>
                            <xsl:attribute name="src">
                                <xsl:value-of select="$base_dir"/>
                                <xsl:text>images/logo/job_scheduler_rabbit_gray_69x31.jpg</xsl:text>
                                <!- -xsl:text>images/logo/job_scheduler_rabbit_circle_black_white_60x61.jpg</xsl:text- ->
                            </xsl:attribute>
                        </xsl:element-->
                    </p>
                </td>
            </tr>
        </table>

        <hr style="margin-bottom: 20pt"/>

        <xsl:if test="concat( '', /*/@suppress_logo ) != 'yes'">
            <div style="float: right; width: 60px; height: 60px; position: relative; top: -5px; margin-left: 1em; margin-right: 1em; margin-bottom: 1em; ">
                <xsl:element name="a">
                    <xsl:attribute name="href">
                        <xsl:value-of select="concat( $base_dir, $start_page )"/>
                    </xsl:attribute>
                    <xsl:attribute name="class">nix</xsl:attribute>
                    <xsl:attribute name="style">border: none</xsl:attribute>
                    
                    <xsl:element name="img">
                        <xsl:attribute name="alt">logo</xsl:attribute>
                        <xsl:attribute name="width">60</xsl:attribute>
                        <xsl:attribute name="height">60</xsl:attribute>
                        <xsl:attribute name="src">
                            <xsl:value-of select="$base_dir"/>
                            <xsl:text>images/logo/job_scheduler_rabbit_circle_60x60.gif</xsl:text>
                        </xsl:attribute>
                        <xsl:attribute name="style">border: none; </xsl:attribute>
                        <xsl:attribute name="title">
                            <xsl:value-of select="$phrases/phrase [ @id='head.link_to_first_page' ]"/>
                        </xsl:attribute>
                    </xsl:element>
                </xsl:element>
            </div>
        </xsl:if>        

        <xsl:if test="$title or /*/@document_state='work_in_progress'">
            <h1 style="margin-top: 0pt">
                <xsl:value-of select="$title"/>
                <xsl:if test="/*/@document_state='work_in_progress'">
                    <span class="work_in_progress" style="margin-left: 2em;">
                        — In Arbeit —
                    </span>
                </xsl:if>
            </h1>
            <!--table cellspacing="0" cellpadding="0" width="100%">
                <tr>
                    <td valign="top">
                        <h1 style="margin-top: 0pt">
                            <xsl:value-of select="$title"/>
                            <xsl:if test="/*/@document_state='work_in_progress'">
                                <span class="work_in_progress" style="margin-left: 2em;">
                                    — In Arbeit —
                                </span>
                            </xsl:if>
                        </h1>
                    </td>
                    <td valign="top" align="right">
                        <xsl:element name="img">
                            <xsl:attribute name="alt">logo</xsl:attribute>
                            <xsl:attribute name="width">60</xsl:attribute>
                            <xsl:attribute name="height">61</xsl:attribute>
                            <xsl:attribute name="src">
                                <xsl:value-of select="$base_dir"/>
                                <xsl:text>images/logo/job_scheduler_rabbit_circle_black_white_60x61.jpg</xsl:text>
                            </xsl:attribute>
                        </xsl:element>
                    </td>
                </tr>
            </table-->
        </xsl:if>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~bottom-->

    <xsl:template name="bottom">
        <xsl:param name="parent_page"/>

        <hr style="margin-top: 30pt"/>
        <table cellspacing="0" cellpadding="0" width="100%">
            <tr>
                <td style="vertical-align: top;">
                    <p style="font-size: 8pt; margin-top: 0">
                        <a href="http://www.sos-berlin.com">
                            <xsl:call-template name="phrase">
                                <xsl:with-param name="id" select="'head.link_to_sos'"/>
                            </xsl:call-template>
                        </a>
                    </p>
                    <!--xsl:call-template name="browse_bar">
                        <xsl:with-param name="parent_page" select="$parent_page"/>
                    </xsl:call-template-->
                </td>
                <td style="vertical-align: top;" align="right">
                    <p style="font-size: 8pt; margin-top: 0">
                        <!--xsl:call-template name="browse_bar_register">
                            <xsl:with-param name="parent_page" select="$parent_page"/>
                        </xsl:call-template-->

                        <xsl:call-template name="phrase">
                            <xsl:with-param name="id" select="'head.last_updated_by'"/>
                        </xsl:call-template>
                        
                        <xsl:text> </xsl:text>
                        
                        <span style="white-space: nowrap">
                            <xsl:variable name="name" select="substring-before( substring-after( /*/@author, 'Author: ' ), ' $' )"/>
                            <xsl:choose>
                                <xsl:when test="$name = 'jz'">
                                    <a href="http://www.zschimmer.com">Joacim Zschimmer</a></xsl:when>
                                <xsl:otherwise><xsl:value-of select="$name"/></xsl:otherwise>
                            </xsl:choose>,
                            <!--xsl:variable name="name" select="document('standards.xml')/standards/authors/author[ @author = current()/@author ]/@full_name"/>
                            <xsl:value-of select="$name"/>,-->
                        </span>

                        <xsl:element name="span">
                            <xsl:attribute name="style">white-space: nowrap</xsl:attribute>
                            <xsl:variable name="date" select="translate( substring-before( substring-after( /*/@date,   'Date: '   ), ' (' ), '/', '-' )"/>
                            <xsl:attribute name="title">
                                <xsl:value-of select="$date"/>
                            </xsl:attribute>
                            <xsl:value-of select="substring( $date, 1, 10 )"/>   <!-- yyyy-mm-dd -->
                        </xsl:element>
                    </p>
                </td>
            </tr>
        </table>
    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~browse_bar-->

    <xsl:template name="browse_bar">
        <xsl:param name="parent_page"/>

        <p style="font-size: 8pt; margin-top: 0px; padding-top: 0px">
            <xsl:if test="not( /*/@suppress_browse_bar='yes' )">
                Scheduler &#160; &#160;
                
                <xsl:element name="a">
                    <xsl:attribute name="class">silent</xsl:attribute>
                    <xsl:attribute name="href"><xsl:value-of select="concat( $base_dir, $start_page )"/></xsl:attribute>
                    <xsl:attribute name="rel">start</xsl:attribute>
                    
                    <xsl:call-template name="phrase">
                        <xsl:with-param name="id" select="'head.link_to_first_page'"/>
                    </xsl:call-template>
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

        &#160;

        <!--
        <xsl:call-template name="scheduler_a">
            <xsl:with-param name="href" select="'command_line.xml'"/>
            <xsl:with-param name="content">
                <xsl:call-template name="phrase">
                    <xsl:with-param name="id" select="'head.link_to_options'"/>
                </xsl:call-template>
            </xsl:with-param>
        </xsl:call-template>
        &#160; &#160;
        -->
        
        <xsl:call-template name="scheduler_a">
            <xsl:with-param name="href" select="'xml.xml'"/>
            <xsl:with-param name="content">
                <xsl:call-template name="phrase">
                    <xsl:with-param name="id" select="'head.link_to_xml'"/>
                </xsl:call-template>
            </xsl:with-param>
        </xsl:call-template>
        &#160; &#160;

        <xsl:call-template name="scheduler_a">
            <xsl:with-param name="href" select="'api/api.xml'"/>
            <xsl:with-param name="content">
                <xsl:call-template name="phrase">
                    <xsl:with-param name="id" select="'head.link_to_api'"/>
                </xsl:call-template>
            </xsl:with-param>
        </xsl:call-template>
        &#160; &#160;

        <xsl:call-template name="scheduler_a">
            <xsl:with-param name="href" select="'register.xml'"/>
            <xsl:with-param name="content">
                <xsl:call-template name="phrase">
                    <xsl:with-param name="id" select="'head.link_to_index'"/>
                </xsl:call-template>
            </xsl:with-param>
        </xsl:call-template>
        
    </xsl:template>

</xsl:stylesheet>

<!-- Das ist ein Gedankenstrich: — -->
<!-- Das ist ein langer Strich: – -->
<!-- Das ist drei Punkte: … -->
<!-- Das ist ein Pfeil nach rechts: → &#x2192; -->
<!-- Das ist ein Pfeil nach links: ← -->
<!-- Das ist ein Doppelpfeil: ↔ -->
<!-- »« -->
