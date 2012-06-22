<?xml version='1.0'?>
<!-- $Id$ -->

<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform"
                version   = "1.0">

    <xsl:include href="scheduler.xsl" />

    <xsl:variable name="value_example" select="'…'"/>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~api-->

    <xsl:template match="variable_substitution">

        <html>
            <xsl:call-template name="html_head">
                <xsl:with-param name="title" select="@title"/>
            </xsl:call-template>

            <body>
                <xsl:call-template name="body_start">
                    <xsl:with-param name="title" select="@title"/>
                </xsl:call-template>

                <xsl:apply-templates select="description"/>


                <table cellspacing="0" cellpadding="0">
                    <tbody>
                        <xsl:call-template name="variable_substitution_xml"/>

                        <xsl:call-template name="variable_substitution_ini">
                            <xsl:with-param name="ini" select="document( 'factory_ini.xml' )/ini"/>
                        </xsl:call-template>

                        <xsl:call-template name="variable_substitution_ini">
                            <xsl:with-param name="ini" select="document( 'sos_ini.xml' )/ini"/>
                        </xsl:call-template>
                    </tbody>
                </table>


                <xsl:call-template name="bottom"/>
            </body>
        </html>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

    <xsl:template name="variable_substitution_xml">

        <xsl:variable name="all_scheduler_elements" select="document( 'xml.xml')//scheduler_element"/>

        <tr>
            <td colspan="99">
                <h2 style="border-top: none">
                    <xsl:call-template name="phrase">
                        <xsl:with-param name="id" select="'xml_attributes'"/>
                    </xsl:call-template>
                </h2>
            </td>
        </tr>

        <xsl:for-each select="$all_scheduler_elements">
            <xsl:sort select="@name | @setting"/>

            <xsl:variable name="directory">
                <xsl:choose>
                    <xsl:when test="@directory">
                        <xsl:value-of select="@directory"/>
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:text>xml/</xsl:text>
                    </xsl:otherwise>
                </xsl:choose>
            </xsl:variable>

            <xsl:if test="$directory != 'xml/answer/'">
                <xsl:variable name="xml_element" select="document( concat( $directory, @name, '.xml' ) )/xml_element "/>
                <xsl:apply-templates select="$xml_element/xml_attributes/xml_attribute" mode="variable_substitution"/>
            </xsl:if>
        </xsl:for-each>
        
    </xsl:template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

    <xsl:template match="xml_attribute" mode="variable_substitution">

        <xsl:variable name="xml_element" select="parent::xml_attributes/parent::xml_element"/>
        <xsl:variable name="name" select="@name | @setting"/>
        <xsl:variable name="setting" select="document( 'settings.xml' )/settings/setting [ @setting=current()/@setting ]"/>

        <xsl:if test="@subst_env or $setting/@subst_env">
            <tr>
                <td>
                    <xsl:call-template name="scheduler_element">
                        <xsl:with-param name="name"      select="$xml_element/@name"/>
                        <xsl:with-param name="attribute" select="$name"/>
                        <xsl:with-param name="value"     select="$value_example"/>
                    </xsl:call-template>
                </td>
                <td style="padding-left: 2em">
                    <xsl:value-of select="$xml_element/@title"/>
                    <xsl:if test="@title | $setting/@title">
                        <xsl:text>, </xsl:text>
                        <xsl:value-of select="@title | $setting/@title"/>
                    </xsl:if>
                    <xsl:text>&#160;</xsl:text>
                    <!--Für korrekte Zeilenhöhe, wenn Titel fehlt-->
                </td>
            </tr>
        </xsl:if>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

    <xsl:template name="variable_substitution_ini">
        <xsl:param name="ini"/>

        <tr>
            <td colspan="99">
                <h2>
                    <xsl:call-template name="phrase">
                        <xsl:with-param name="id" select="'ini.file'"/>
                    </xsl:call-template>
                    <xsl:text> </xsl:text>
                    <xsl:value-of select="$ini/@file"/>
                </h2>
            </td>
        </tr>

        <xsl:for-each select="$ini/ini_sections/ini_section">
            <xsl:variable name="ini_section" select="document( concat( substring-before( $ini/@file, '.ini' ), '_ini_', @name, '.xml' ) )/ini_section"/>

            <xsl:for-each select="$ini_section/ini_entries/ini_entry">
                <xsl:sort select="@name | @setting"/>

                <xsl:variable name="name" select="@name | @setting"/>
                <xsl:variable name="setting" select="document( 'settings.xml' )/settings/setting [ @setting=current()/@setting ]"/>

                <xsl:if test="@subst_env or $setting/@subst_env">
                    <tr>
                        <td>
                            <code>
                                <xsl:text>[</xsl:text>
                                <xsl:value-of select="$ini_section/@name"/>
                                <xsl:text>] </xsl:text>
                            </code>

                            <xsl:call-template name="scheduler_ini_entry">
                                <xsl:with-param name="file"    select="parent::ini_entries/parent::ini_section/@file"/>
                                <xsl:with-param name="section" select="parent::ini_entries/parent::ini_section/@name"/>
                                <xsl:with-param name="entry"   select="$name"/>
                                <xsl:with-param name="value"   select="$value_example"/>
                                <xsl:with-param name="show_entry_only" select="true()"/>
                            </xsl:call-template>
                        </td>
                        <td style="padding-left: 2em">
                            <xsl:value-of select="@title | $setting/@title"/>
                            <xsl:text>&#160;</xsl:text>  <!--Für korrekte Zeilenhöhe, wenn Titel fehlt-->
                        </td>
                    </tr>
                </xsl:if>

            </xsl:for-each>
        </xsl:for-each>

    </xsl:template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

</xsl:stylesheet>
