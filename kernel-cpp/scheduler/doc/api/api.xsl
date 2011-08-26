<?xml version='1.0'?>
<!-- $Id: api.xsl 14199 2011-01-25 12:13:10Z ss $ -->



<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform"
                version   = "1.0">


<xsl:variable name="default_programming_language"  select="'javascript'"/>
<xsl:variable name="default_programming_language2" select="'java'"/>       <!-- Alternative -->

<xsl:include href="../scheduler_base.xsl" />

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~api-->

<xsl:template match="api">

    <xsl:param name="title"/>
    <xsl:param name="base_dir"/>
    <xsl:param name="author"/>
    <xsl:param name="date"/>

    <html>
        <xsl:call-template name="html_head">
            <xsl:with-param name="title" select="$title"/>
        </xsl:call-template>

        <body>
            <xsl:apply-templates select="." mode="api.description"/>
            <!--xsl:apply-templates select="description"/-->
        </body>
    </html>

</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template name="programming_language_selector">
    <xsl:param name="this_programming_language"/>
    <xsl:param name="href"/>

    <xsl:variable name="plang" select="translate( $this_programming_language, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz' )"/>

    <xsl:element name="a">
        <xsl:attribute name="id">programming_language_selector__<xsl:value-of select="$plang"/></xsl:attribute>
        <xsl:attribute name="class">api_programming_language_selector</xsl:attribute>

        <xsl:choose>
            <xsl:when test="$selected_programming_language=$plang">
                <xsl:attribute name="style">font-weight: bold;</xsl:attribute>
            </xsl:when>
            <xsl:when test="$href">
                <xsl:attribute name="href"><xsl:value-of select="$href"/></xsl:attribute>
            </xsl:when>
            <xsl:otherwise>
                <xsl:attribute name="href"><xsl:value-of select="/api.class/@name"/>-<xsl:value-of select="$plang"/>.xml</xsl:attribute>
            </xsl:otherwise>
        </xsl:choose>

        <xsl:value-of select="$this_programming_language"/>
    </xsl:element>

</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
<!--
<xsl:template match="api.classes" mode="description">
    <xsl:apply-templates select="document( 'all_classes.xml' )/class_references/class_reference"/>
</xsl:template>
-->
<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
<!--
<xsl:template match="class_reference">
    <p style="margin-top: 0px">
    <xsl:element name="span">

        <xsl:element name="a">
            <xsl:attribute name="id">class_reference_<xsl:value-of select="@name"/></xsl:attribute>
            <xsl:attribute name="href"><xsl:value-of select="@name"/>.xml</xsl:attribute>
            <xsl:value-of select="@name"/>
        </xsl:element>
    </xsl:element>

    </p>
</xsl:template>
-->
<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="api.class_reference" mode="description">
    <xsl:apply-templates select="."/>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
<!-- Klick auf den Klassennamen ruft im rechten Teil die Beschreibung der Klasse auf -->
<!--
<xsl:template match="class_reference | api.class_reference">
    <xsl:call-template name="all_classes">
        <xsl:with-param name="class" select="@class"/>
        <xsl:with-param name="active_class" select="/api.class/@name"/>
    </xsl:call-template>
</xsl:template>
-->
<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template name="all_classes">
    <xsl:param name="class"/>
    <xsl:param name="class_references"/>
    <xsl:param name="active_class"/>

    <xsl:for-each select="$class_references">
        <xsl:variable name="href">
            <xsl:value-of select="/*/@base_dir"/>api/<xsl:value-of select="@class"/>-<xsl:value-of select="$selected_programming_language"/>.xml
        </xsl:variable>

        <p style="margin-top: 0px;">
            <xsl:element name="a">
                <!--xsl:attribute name="id">class_reference_<xsl:value-of select="$class"/></xsl:attribute-->
                <!--xsl:attribute name="href">javascript:void(0)</xsl:attribute>  <!- - Für ie6 -->
                <!--xsl:attribute name="onclick">api.class_reference__onclick( "<xsl:value-of select="$class"/>" );</xsl:attribute>-->

                <xsl:attribute name="href"><xsl:value-of select="normalize-space( $href )"/></xsl:attribute>

                <xsl:if test="@class=$active_class">
                    <xsl:attribute name="style">font-weight: bold;</xsl:attribute>
                </xsl:if>

                <xsl:choose>
                    <xsl:when test="@class != 'api'">
                        <xsl:value-of select="@class"/>
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:call-template name="phrase">
                            <xsl:with-param name="id" select="'api.overview.title'"/>
                        </xsl:call-template>
                    </xsl:otherwise>
                </xsl:choose>
            </xsl:element>
        </p>
    </xsl:for-each>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
<!--
<xsl:template match="api.class [ @programming_language ]">
    <xsl:apply-templates select="document( concat( @name, '.xml' ) )"/>
    <xsl:call-template name="api.class">
        <xsl:with-param name="xml" select="document( concat( @name, '.xml' ) )"/>
        <xsl:with-param name="programming_language" select="@programming_language"/>
    </xsl:call-template>
</xsl:template>


<xsl:template name="api.class">
    <xsl:param name="xml"/>
    <xsl:param name="programming_language"/>

    <xsl:apply-templates select="$xml"/>
</xsl:template>
-->
<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="api.class [ not(*) ]">
    <xsl:apply-templates select="document( concat( @name, '.xml' ) )"/>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="api.class">
    <html>
        <xsl:variable name="title">
            <xsl:if test="@name!='api'">
                <xsl:value-of select="@name"/> 
                - 
            </xsl:if>
            <xsl:value-of select="$phrases/phrase [ @id='api.title' ]"/>
        </xsl:variable>

        <xsl:call-template name="html_head">
            <xsl:with-param name="title" select="$title"/>
        </xsl:call-template>

        <body>
            <xsl:call-template name="body_start">
                <xsl:with-param name="title" select="$title"/>
            </xsl:call-template>


            <table cellpadding="0" cellspacing="0" width="100%" style="padding-bottom: 4ex">
                <tr>
                    <td style="vertical-align: top; padding-right: 3ex; padding-bottom: 4pt; width: 20ex;">
                        <!-- Besser mit rowspan="2" mit der unteren Zelle im selben <div> zusammenfassen 
                             position: fixed sollte nur vertical gelten, leider bleibt es auch bei horizontaler Verschiebung stehen,
                             deshalb background-color: window, damit wenigstens die Zeichen nicht überblendet werden.
                             2006-08-22 Zschimmer
                        -->
                        <div id="all_classes_1" style="position: fixed; width: 20ex; background-color: window; padding-right: 1em;">  <!-- ie6: Wird mit JavaScript positioniert -->
                            <xsl:call-template name="all_classes">   <!-- Übersicht -->
                                <xsl:with-param name="class_references"  select="document( 'all_classes.xml' )/class_references/class_reference [ @class='api' ]"/>
                                <xsl:with-param name="active_class"      select="/api.class/@name"/>
                            </xsl:call-template>
                        </div>
                    </td>

                    <td style="padding-left: 3ex; border-left: 1px dotted black; padding-bottom: 4pt;">

                        <xsl:call-template name="programming_language_selector">
                            <xsl:with-param name="this_programming_language">Java</xsl:with-param>
                        </xsl:call-template>

                        <xsl:call-template name="programming_language_selector">
                            <xsl:with-param name="this_programming_language">JavaScript</xsl:with-param>
                        </xsl:call-template>

                        <xsl:call-template name="programming_language_selector">
                            <xsl:with-param name="this_programming_language">VBScript</xsl:with-param>
                        </xsl:call-template>

                        <xsl:call-template name="programming_language_selector">
                            <xsl:with-param name="this_programming_language">Perl</xsl:with-param>
                        </xsl:call-template>

                        <xsl:call-template name="programming_language_selector">
                            <xsl:with-param name="this_programming_language">javax.script</xsl:with-param>
                        </xsl:call-template>
                        
                    </td>

                    <xsl:if test="/api.class/@name != 'api' and not( /api.class/not_implemented/@programming_language='java' )">
                        <td style="padding-left: 3ex; padding-bottom: 4pt; text-align: right;">
                            <xsl:call-template name="programming_language_selector">
                                <xsl:with-param name="this_programming_language">Javadoc</xsl:with-param>
                                <xsl:with-param name="href"><xsl:value-of select="@base_dir"/>javadoc/sos/spooler/<xsl:value-of select="@name"/>.html</xsl:with-param>
                            </xsl:call-template>
                        </td>
                    </xsl:if>
                </tr>

                <tr>
                    <td style="font-size: 1pt;">
                        &#160;
                    </td>

                    <td colspan="99" style="border-top: 1px dotted black; border-left: 1px dotted black; font-size: 4pt;">
                        &#160;
                    </td>
                </tr>

                <tr>
                    <td style="vertical-align: top; padding-right: 3ex; width: 20ex;">
                        <!--p style="font-weight: bold; padding-bottom: 1em">Klassen:</p-->
                        <div id="all_classes_2" style="position: fixed; width: 20ex; background-color: window; padding-right: 1em; padding-bottom: 1em;">  <!-- ie6: Wird mit JavaScript positioniert -->
                            <xsl:call-template name="all_classes">
                                <xsl:with-param name="class_references"  select="document( 'all_classes.xml' )/class_references/class_reference [ @class != 'api' ]"/>
                                <xsl:with-param name="active_class"      select="/api.class/@name"/>
                            </xsl:call-template>
                        </div>
                        <div id="all_classes_hidden" style="visibility: hidden">  <!-- Wegen position: fixed, damit das Layout stimmt -->
                            <xsl:call-template name="all_classes">
                                <xsl:with-param name="class_references"  select="document( 'all_classes.xml' )/class_references/class_reference [ @class != 'api' ]"/>
                                <xsl:with-param name="active_class"      select="/api.class/@name"/>
                            </xsl:call-template>
                        </div>
                    </td>

                    <td colspan="2" style="padding-left: 3ex; border-left: 1px dotted black;">
                        <!--xsl:apply-templates select="." mode="headline"/-->
                        <p class="api_headline">
                            <xsl:if test="@name != 'api'">
                                <xsl:value-of select="@name"/>

                                <xsl:if test="title">
                                    &#160;–&#160;
                                </xsl:if>
                            </xsl:if>

                            <xsl:value-of select="title"/>
                        </p>

                        <xsl:if test="property | method">
                            <p>&#160;</p>
                            <xsl:apply-templates select="." mode="table"/>
                        </xsl:if>

                        <xsl:apply-templates select="." mode="example"/>
                        <xsl:apply-templates select="." mode="api.description"/>

                        <p style="margin-top: 0em">&#160;</p>


                        <xsl:apply-templates select="." mode="detailed_methods"/>
                    </td>
                </tr>
            </table>

            <xsl:element name="script">
                <xsl:attribute name="defer">defer</xsl:attribute>
                <xsl:attribute name="type">text/javascript</xsl:attribute>
                <xsl:attribute name="src"><xsl:value-of select="/*/@base_dir"/>api/api.js</xsl:attribute>
            </xsl:element>

            <xsl:call-template name="bottom"/>
        </body>
    </html>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~api.description-->

<xsl:template mode="api.description" match="*">
    <xsl:if test="description [ not ( @programming_language ) ]">
        <p>&#160;</p>
        <!--p style="margin-top: 0em">&#160;</p-->
        <xsl:apply-templates select="description [ not ( @programming_language ) ]"/>
    </xsl:if>

    <xsl:if test="description [ @programming_language = $selected_programming_language ]">
        <p style="margin-top: 0em">&#160;</p>
        <xsl:apply-templates select="description [ @programming_language = $selected_programming_language ]"/>
    </xsl:if>
</xsl:template>
    
    
<!--
<xsl:template match="api.class [ @programming_language ]">

    <xsl:call-template name="api.class-content-only"/>

</xsl:template>


<xsl:template name="api.class-content-only">


</xsl:template>
-->

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
<!--
<xsl:template match="/api.class [ /*/@show_headline ]">
    <xsl:apply-templates select="." mode="headline"/>
</xsl:template>
-->
<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
<!--
<xsl:template match="api.class" mode="headline">

    <xsl:if test="@name != 'api'">
        <p class="api_headline">
            <xsl:value-of select="@name"/>

            <xsl:if test="title">
                &#160;–&#160; <xsl:value-of select="title"/>
            </xsl:if>
        </p>
    </xsl:if>

    <xsl:apply-templates select="." mode="example"/>

    <xsl:if test="description [ not ( @programming_language ) ]">
        <p style="margin-top: 0em">&#160;</p>
        <xsl:apply-templates select="description [ not ( @programming_language ) ]"/>
    </xsl:if>

    <xsl:if test="description [ @programming_language = $selected_programming_language ]">
        <p style="margin-top: 0em">&#160;</p>
        <xsl:apply-templates select="description [ @programming_language = $selected_programming_language ]"/>
    </xsl:if>

    <p style="margin-top: 0em">&#160;</p>
</xsl:template>
-->
<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="/api.class [ /*/@show_table ]">
    <xsl:apply-templates select="." mode="table"/>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="api.class" mode="table">

    <!--h2>Methoden</h2>  <!- - zieht einen Strich -->

    <table cellpadding="0" cellspacing="0">
        <!--tr><td colspan="4" style="padding-top: 4ex; padding-bottom: 1ex; font-weight: bold;">Eigenschaften</td></tr-->
        <!--
        <tr>
            <td class="api_type"       >Typ</td>
            <td class="api_method"     style="font-weight: bold">Eigenschaft&#160;</td>
        </tr>

        <tr>
            <td colspan="5" style="padding-bottom: 4pt;"><hr/></td>
        </tr>

        <xsl:apply-templates select="property" mode="table">
            <xsl:sort select="@name"/>
        </xsl:apply-templates>


        <tr><td colspan="4" style="padding-top: 4ex; padding-bottom: 1ex; font-weight: bold;">&#160;</td></tr>
        <tr>
            <td class="api_type"       >Ergebnistyp</td>
            <td class="api_method"     style="font-weight: bold">Methode&#160;</td>
        </tr>

        <tr>
            <td colspan="5" style="padding-bottom: 4pt;"><hr/></td>
        </tr>
        -->

        <xsl:if test="not( not_implemented/@programming_language=$selected_programming_language )">
            <xsl:apply-templates select="  property [ not( not_implemented/@programming_language=$selected_programming_language ) ]
                                        | method   [ not( not_implemented/@programming_language=$selected_programming_language ) ]" mode="table">
                <xsl:sort select="@name"/>
            </xsl:apply-templates>
        </xsl:if>
    </table>

</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="property | method" mode="table">

    <xsl:if test="position() &gt; 1">
        <!-- Trenner zwischen verschiedenen Eigenschaften oder Methoden -->
        <tr>
            <td style="font-size: 1pt">&#160;</td>
        </tr>
    </xsl:if>

    <xsl:apply-templates select="." mode="table_rows">
        <xsl:with-param name="is_in_table" select="true()"/>
    </xsl:apply-templates>

</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="property | method" mode="table_rows">
    <xsl:param name="is_in_table" select="false()"/>
    <xsl:param name="show_title"  select="true()"/>

    <xsl:apply-templates select="." mode="com_table_rows">
        <xsl:with-param name="is_in_table" select="$is_in_table"/>
        <xsl:with-param name="show_title"  select="$show_title"/>
    </xsl:apply-templates>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="property | method" mode="com_table_rows">
    <xsl:param name="is_in_table" select="false()"/>
    <xsl:param name="show_title"  select="true()"/>

    <xsl:for-each select="com">
        <xsl:choose>
            <xsl:when test="parent::property and ( @access='write' or parent::property/@access='write' )">
                <xsl:apply-templates select="." mode="table_row">
                    <xsl:with-param name="access"      select="'write'"/>
                    <xsl:with-param name="is_in_table" select="$is_in_table"/>
                    <xsl:with-param name="show_title"  select="$show_title"/>
                </xsl:apply-templates>
            </xsl:when>

            <xsl:when test="parent::property and ( @access='read' or parent::property/@access='read' )">
                <xsl:apply-templates select="." mode="table_row">
                    <xsl:with-param name="access"      select="'read'"/>
                    <xsl:with-param name="is_in_table" select="$is_in_table"/>
                    <xsl:with-param name="show_title"  select="$show_title"/>
                </xsl:apply-templates>
            </xsl:when>

            <xsl:when test="parent::property and not( parent::property/@access )">
                <xsl:apply-templates select="." mode="table_row">
                    <xsl:with-param name="access"      select="'write'"/>
                    <xsl:with-param name="is_in_table" select="$is_in_table"/>
                    <xsl:with-param name="show_title"  select="$show_title"/>
                </xsl:apply-templates>

                <xsl:apply-templates select="." mode="table_row">
                    <xsl:with-param name="access"      select="'read'"/>
                    <xsl:with-param name="is_in_table" select="$is_in_table"/>
                    <xsl:with-param name="show_title"  select="false()"/>
                </xsl:apply-templates>
            </xsl:when>

            <xsl:otherwise>
                <xsl:apply-templates select="." mode="table_row">
                    <xsl:with-param name="is_in_table" select="$is_in_table"/>
                    <xsl:with-param name="show_title"  select="$show_title"/>
                </xsl:apply-templates>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:for-each>
    
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="java | com" mode="signature">

    <table cellpadding="0" cellspacing="0">
        <xsl:apply-templates select="." mode="table_row">
            <xsl:with-param name="show_title" select="false()"/>
        </xsl:apply-templates>
    </table>

</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="java | com" mode="table_row">
    <xsl:param name="access"        select="string( @access | parent::*/@access )"/>
    <xsl:param name="show_title"    select="true()"/>
    <xsl:param name="title_rowspan" select="1"/>
    <xsl:param name="is_in_table"   select="false()"/>

    <xsl:variable name="method_tr_id" select="concat( 'tr_', generate-id(parent::*) )"/>
    <xsl:variable name="tr_id"        select="concat( $method_tr_id, '.', generate-id(.), $access )"/>
    <!--xsl:variable name="tr_id"        select="concat( $method_tr_id, '.', position(), $access )"/-->

    <xsl:element name="tr">

        <xsl:choose>
            <xsl:when test="$is_in_table">
                <xsl:attribute name="class">api_method_clickable</xsl:attribute>
                <xsl:attribute name="id"><xsl:value-of select="$tr_id"/></xsl:attribute>
                <xsl:attribute name="onclick">window.location = "#method__<xsl:value-of select="parent::*/@name"/>";</xsl:attribute>
                <xsl:attribute name="onmouseover">api_method_in_table__onmouseover( "<xsl:value-of select="$method_tr_id"/>" )</xsl:attribute>
                <xsl:attribute name="onmouseout" >api_method_in_table__onmouseout ( "<xsl:value-of select="$method_tr_id"/>" )</xsl:attribute>
            </xsl:when>

            <xsl:otherwise>
                <xsl:attribute name="class">api_method</xsl:attribute>
            </xsl:otherwise>
        </xsl:choose>

        <td class="api_type">
            <xsl:choose>
                <xsl:when test="( java.result or com.result ) and not( $access='write' )">
                    <xsl:apply-templates select="java.result/java.type | java.result/com.type | com.result/com.type">
                        <xsl:with-param name="is_in_table" select="$is_in_table"/>
                    </xsl:apply-templates>
                    &#160;
                </xsl:when>
                <xsl:when test="parent::property and $language_has_properties and $access='write'">
                </xsl:when>
                <xsl:when test="not( $selected_programming_language='java' )">
                </xsl:when>
                <xsl:otherwise>
                    <span class="mono">void&#160;</span>
                </xsl:otherwise>
            </xsl:choose>
        </td>

        <xsl:element name="td">
            <xsl:attribute name="class">api_method</xsl:attribute>

            <!--xsl:if test="not( $language_has_properties and parent::property )">
                <xsl:attribute name="colspan">2</xsl:attribute>
            </xsl:if-->

            <xsl:element name="span">
                <xsl:if test="parent::*/@deprecated">
                    <xsl:attribute name="style">text-decoration: line-through</xsl:attribute>
                </xsl:if>

                <xsl:apply-templates select="parent::*" mode="method_name">
                    <xsl:with-param name="access" select="$access"/>
                </xsl:apply-templates>
            </xsl:element>


            <xsl:if test="com.parameter or parent::method or not( $language_has_properties or parent::property/@is_variable )">
                <xsl:choose>
                    <xsl:when test="$access='write' and not( $language_has_properties )">
                        <xsl:apply-templates select="." mode="parameter_list">
                            <xsl:with-param name="parameters" select="java.parameter | java.result | com.parameter | com.result"/>
                            <xsl:with-param name="is_in_table" select="$is_in_table"/>
                        </xsl:apply-templates>
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:apply-templates select="." mode="parameter_list">
                            <xsl:with-param name="parameters" select="java.parameter | com.parameter"/>
                            <xsl:with-param name="is_in_table" select="$is_in_table"/>
                        </xsl:apply-templates>
                    </xsl:otherwise>
                </xsl:choose>
            </xsl:if>

            <xsl:if test="parent::property and $language_has_properties and $access='write'">
                <span class="mono"> = </span>
                <xsl:apply-templates select="com.result">
                    <xsl:with-param name="is_in_table" select="$is_in_table"/>
                </xsl:apply-templates>
            </xsl:if>


            <!--xsl:if test="parent::property and $language_has_properties and $access">
                <span style="font-size: 8pt"> &#160;(<xsl:value-of select="$access"/> only)</span>
            </xsl:if-->

        </xsl:element>


        <xsl:if test="$is_in_table">
            <td class="api_title">
                <xsl:if test="$show_title and $title_rowspan &gt; 0 and position() = 1">
                    <xsl:choose>
                        <xsl:when test="parent::*/title">
                            <xsl:apply-templates select="parent::*/title"/>
                        </xsl:when>
                        <xsl:when test="parent::*/@setting">
                            <xsl:variable name="setting" select="document( '../settings.xml' )/settings/setting[ @setting = current()/parent::*/@setting ]"/>
                            <xsl:value-of select="$setting/@title"/>
                        </xsl:when>
                    </xsl:choose>
                </xsl:if>

                <xsl:apply-templates select="parent::method | parent::property" mode="comment"/>
            </td>
        </xsl:if>

    </xsl:element>

    <!--
    <xsl:if test="parent::*/title and $show_title and $title_rowspan &gt; 0 and position() = last()">
        <xsl:element name="tr">
            <td></td>

            <td colspan="99" class="api_title">
                <xsl:value-of select="parent::*/title"/>
            </td>
        </xsl:element>
    </xsl:if>
    -->

</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="method | property" mode="method_name">

    <xsl:if test="parent::api.class/@object_name">
        <span class="api_object_name">
            <xsl:value-of select="parent::api.class/@object_name"/>
            <xsl:text>.</xsl:text>
        </span>
    </xsl:if>

    <span class="mono" style="font-weight: bold"><xsl:value-of select="@name"/></span>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
<!-- ( parameter ... ) -->

<xsl:template match="java | com" mode="parameter_list">
    <xsl:param name="parameters"/>
    <xsl:param name="is_in_table"/>

    <xsl:if test="not( $selected_programming_language='perl' )">
        <span class="mono"><xsl:text>(</xsl:text></span>
    </xsl:if>

    <xsl:if test="$parameters">
        <span class="mono"><xsl:text> </xsl:text></span>

        <xsl:for-each select="$parameters">
            <xsl:if test="position() &gt; 1">
                <span class="mono"><xsl:text>, </xsl:text></span>
            </xsl:if>

            <span style="white-space: nowrap">
                <xsl:apply-templates select=".">
                    <xsl:with-param name="is_in_table" select="$is_in_table"/>
                </xsl:apply-templates>

                <xsl:if test="@default and not( $is_in_table )">
                    <span class="mono">
                        <xsl:text> = </xsl:text>
                        <xsl:value-of select="@default"/>
                    </span>
                </xsl:if>

                <xsl:if test="@optional and not( $is_in_table )">
                    &#160;<span style="font-size: 8pt">(optional)</span>
                </xsl:if>
            </span>
        </xsl:for-each>

        <span class="mono"><xsl:text> </xsl:text></span>
    </xsl:if>

    <xsl:if test="not( $selected_programming_language='perl' ) or $parameters or parent::method">
        <span class="mono"><xsl:text>)</xsl:text></span>
    </xsl:if>

</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
<!-- Parametertyp und -name -->

<xsl:template match="com.parameter | com.result | java.parameter | java.result">
    <xsl:param name="is_in_table"/>

    <span style="white-space: nowrap">
        <xsl:if test="not( $is_in_table ) or not( @name )">
            <xsl:apply-templates select="java.type | com.type">
                <xsl:with-param name="is_in_table" select="$is_in_table"/>
            </xsl:apply-templates>

            <xsl:if test="@name">
                <span class="mono"><xsl:text> </xsl:text></span>
            </xsl:if>
        </xsl:if>

        <span class="mono"><xsl:value-of select="@name"/></span>
    </span>

</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="com.type">
    <xsl:param name="is_in_table"/>

    <xsl:apply-templates select="." mode="no_array">
        <xsl:with-param name="is_in_table" select="$is_in_table"/>
    </xsl:apply-templates>

    <xsl:if test="@array">
        <span class="mono">[]</span>
    </xsl:if>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="com.type [ @class ]" mode="no_array">
    <xsl:param name="is_in_table"/>

    <xsl:choose>
        <xsl:when test="$is_in_table">
            <span class="mono">
                <xsl:value-of select="@class"/>
            </span>
        </xsl:when>
        <xsl:otherwise>
            <xsl:call-template name="scheduler_method">
                <xsl:with-param name="class" select="@class"/>
            </xsl:call-template>
        </xsl:otherwise>
    </xsl:choose>

    <!--
    <xsl:if test="$selected_programming_language!='java'">
        <xsl:element name="span">
            <xsl:attribute name="title">A Scheduler class name, not a real class name in <xsl:value-of select="$selected_programming_language"/></xsl:attribute>
            <xsl:attribute name="style">cursor: default</xsl:attribute>
            <xsl:text>¹</xsl:text>
        </xsl:element>
    </xsl:if>
    -->

</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="com.type [ @type ]" mode="no_array">
    <span class="mono"><xsl:value-of select="@type"/></span>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="com.type [ @type='BSTR' ]" mode="no_array">
    <span class="mono">
        <xsl:text>String</xsl:text>
    </span>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
<!--
<xsl:template match="com.type [ @type='BSTR' and @array ]">
    <span class="mono" title="Array of Strings">String[]</span>
</xsl:template>
-->
<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="com.type [ @type='VARIANT' ]" mode="no_array">
    <xsl:param name="is_in_table" select="false()"/>

    <xsl:choose>
        <xsl:when test="com.type [ @type ]">
            <xsl:for-each select="com.type">
                <xsl:if test="position() &gt; 1">
                    <span style="padding: 2pt">|</span>
                    <!--xsl:text>&#x2009;|&#x2009;</xsl:text>     ie stellt U+2009 "thin space" als Kästchen dar :-( -->
                </xsl:if>
                <xsl:apply-templates select=".">
                    <xsl:with-param name="is_in_table" select="$is_in_table"/>
                </xsl:apply-templates>
            </xsl:for-each>
        </xsl:when>

        <xsl:otherwise>
            <span class="mono">
                Variant
            </span>
        </xsl:otherwise>
    </xsl:choose>

</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
<!--
<xsl:template match="property | method" mode="result_type">
    <xsl:apply-templates select="com/com.result/com.type"/>
</xsl:template>
-->
<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="api.null" mode="description">
    <code>null</code>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="api.empty" mode="description">
    <code>empty</code>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="api.class" mode="detailed_methods">

    <xsl:if test="not( not_implemented/@programming_language=$selected_programming_language )">

        <xsl:apply-templates select="  method   [ com and not( not_implemented/@programming_language=$selected_programming_language ) ]
                                     | property [ com and not( not_implemented/@programming_language=$selected_programming_language ) ]"
                            mode="detailed_methods">
            <xsl:sort select="@name"/>
        </xsl:apply-templates>

    </xsl:if>

</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
<!--
<xsl:template match="/api.class [ /*/@show_detailed_methods ]">
    <xsl:apply-templates select="." mode="detailed_methods"/>
</xsl:template>
-->
<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="method | property" mode="detailed_methods">

    <xsl:element name="h2">
        <xsl:attribute name="class">bar</xsl:attribute>
        <xsl:attribute name="id">method__<xsl:value-of select="@name"/></xsl:attribute>

        <xsl:attribute name="style">
            <xsl:if test="position() = 1 or true()">border-top: 1px solid black;</xsl:if>
        </xsl:attribute>

        <xsl:value-of select="@name"/>

        <!--xsl:if test="title">
            &#160; – &#160; <xsl:value-of select="title"/>
        </xsl:if-->
    </xsl:element>


    <xsl:choose>
        <xsl:when test="title">
            <xsl:apply-templates select="title"/>
        </xsl:when>
        <xsl:when test="@setting">
            <xsl:variable name="setting" select="document( '../settings.xml' )/settings/setting[ @setting = current()/@setting ]"/>
            <xsl:value-of select="$setting/@title"/>
        </xsl:when>
    </xsl:choose>


    <xsl:if test="@deprecated">
        <p>
            *** 
            <xsl:call-template name="phrase">
                <xsl:with-param name="id" select="'api.method.deprecated'"/>
            </xsl:call-template>"
            ***
        </p>
    </xsl:if>


    <xsl:apply-templates select="." mode="detailed_comment"/>


    <p style="margin-top: 0em">&#160;</p>


    <!-- Signatur -->
    <table cellpadding="0" cellspacing="0">
        <xsl:apply-templates select="." mode="table_rows">
            <xsl:with-param name="show_title" select="false()"/>
        </xsl:apply-templates>
    </table>


    <xsl:apply-templates select="." mode="example"/>


    <xsl:if test="@setting">
        <p>&#160;</p>
        <xsl:apply-templates mode="setting_description" select="."/>
    </xsl:if>

    <xsl:apply-templates select="." mode="api.description"/>
    <!--xsl:if test="description">
        <p>&#160;</p>
        <xsl:apply-templates select="description"/>
    </xsl:if-->


    <!-- Bei einer <property access="write" (oder write und read) ist <com.result> der Parameter, die rechte Seite der Zuweisung -->

    <xsl:variable name="read_result"  select="com                            [ @access='read'  or not( @access ) and ( not( parent::*/@access        ) or parent::property/@access='read'  ) ]/com.result"/>
    <xsl:variable name="write_result" select="com [ not ( parent::method ) ] [ @access='write' or not( @access ) and ( not( parent::property/@access ) or parent::property/@access='write' ) ]/com.result"/>

    <xsl:if test="com/com.parameter | $write_result ">
        <h3>
            <xsl:call-template name="phrase">
                <xsl:with-param name="id" select="'api.method.parameters.title'"/>
            </xsl:call-template>
        </h3>

        <table cellpadding="0" cellspacing="0">
            <xsl:for-each select="com/com.parameter | $write_result">
                <tr>
                    <td>
                        <span class="mono">
                            <xsl:choose>
                                <xsl:when test="@name">
                                    <xsl:value-of select="@name"/>
                                </xsl:when>
                                <xsl:otherwise>
                                    <!--xsl:value-of select="com.type/@type | com.type/@class"/-->
                                    <xsl:apply-templates select="com.type"/>
                                </xsl:otherwise>
                            </xsl:choose>
                        </span>
                        <xsl:if test="@optional">
                            &#160;<span style="font-size: 8pt">(optional)</span>
                        </xsl:if>
                    </td>
                    
                    <td style="padding-left: 2ex">
                        <!--xsl:choose>
                            <xsl:when test="$selected_programming_language='perl' and com.type [ @class or @array ]">
                                <span class="not_for_unix_perl">
                                    <xsl:call-template name="phrase">
                                        <xsl:with-param name="id" select="'api.parameter.not_for_unix_perl'"/>
                                    </xsl:call-template>
                                </span>
                            </xsl:when>
                            <xsl:when test="$selected_programming_language='perl' and .//com.type [ @class or @array ]">
                                <span class="not_for_unix_perl">
                                    <xsl:call-template name="phrase">
                                        <xsl:with-param name="id" select="'api.parameter.restricted_for_unix_perl'"/>
                                    </xsl:call-template>
                                </span>
                            </xsl:when>
                        </xsl:choose-->

                        <xsl:apply-templates select="title"/>
                        <xsl:apply-templates select="description"/>
                    </td>
                </tr>
            </xsl:for-each>
        </table>
    </xsl:if>


    <xsl:if test="$read_result/com.type [ @class or parent::*/description ]">
        <h3>
            <xsl:call-template name="phrase">
                <xsl:with-param name="id" select="'api.method.return.title'"/>
            </xsl:call-template>
        </h3>

        <xsl:apply-templates select="$read_result/com.type"/>&#160;&#160;
        <xsl:apply-templates select="com/com.result/description"/>
    </xsl:if>


    <xsl:if test="messages/message">
        <xsl:if test="messages [ message/@level='error' ]">
            <h3>
                <xsl:call-template name="phrase">
                    <xsl:with-param name="id" select="'api.exceptions.title'"/>
                </xsl:call-template>
            </h3>

            <xsl:call-template name="messages">
                <xsl:with-param name="message_set" select="messages/message[ @level='error' ]"/>
            </xsl:call-template>
        </xsl:if >

        <xsl:if test="messages [ message/@level != 'error' ]">
            <h3>
                <xsl:call-template name="phrase">
                    <xsl:with-param name="id" select="'messages.title'"/>
                </xsl:call-template>
            </h3>
            <xsl:call-template name="messages">
                <xsl:with-param name="message_set" select="messages/message[ @level != 'error' ]"/>
                <xsl:with-param name="show_level"  select="true()"/>
            </xsl:call-template>
        </xsl:if>
    </xsl:if>


    <!--xsl:if test="@setting">
        <div class="see_also">
            <h3 style="margin-bottom: 0em">
                <xsl:call-template name="phrase">
                    <xsl:with-param name="id" select="'common.See'"/>
                </xsl:call-template>
            </h3>
            <xsl:apply-templates mode="setting_references" select="."/>
        </div>  
    </xsl:if-->


</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="*" mode="example">
    <xsl:choose>
        <xsl:when test="example [ not( @programming_language )  or  @programming_language=$selected_programming_language ]">
            <xsl:apply-templates select="example [ not( @programming_language )  or  @programming_language=$selected_programming_language ]"/>
        </xsl:when>
        <xsl:when test="example [ @programming_language=$default_programming_language ]">
            <xsl:apply-templates select="example [ @programming_language=$default_programming_language ]">
                <xsl:with-param name="programming_language" select="$default_programming_language"/>
            </xsl:apply-templates>
        </xsl:when>
        <xsl:otherwise>
            <xsl:apply-templates select="example [ @programming_language=$default_programming_language2 ]">
                <xsl:with-param name="programming_language" select="$default_programming_language2"/>
            </xsl:apply-templates>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="method | property" mode="comment">
    <!-- Default -->
</xsl:template>
    
<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="method | property" mode="detailed_comment">
    <!-- Default -->
</xsl:template>
    
<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

</xsl:stylesheet>
