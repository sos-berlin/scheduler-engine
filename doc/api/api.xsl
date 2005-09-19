<?xml version='1.0'?>
<!-- $Id$ -->



<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform"
                version   = "1.0">

<xsl:param name="fixed_programming_language"/>

<xsl:variable name="default_programming_language"  select="'javascript'"/>
<xsl:variable name="default_programming_language2" select="'java'"/>       <!-- Alternative -->

<xsl:include href="../scheduler_base.xsl" />
<!-- Nachrangige <xsl:include> sind am Ende dieses Stylesheets -->

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
            <xsl:apply-templates select="description"/>
        </body>
    </html>

</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<!--xsl:template name="api.programming_language_selector" mode="description">
    <xsl:apply-templates
</xsl:template-->

<xsl:template name="programming_language_selector">
    <xsl:param name="this_programming_language"/>

    <xsl:variable name="plang" select="translate( $this_programming_language, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz' )"/>

    <xsl:element name="a">
        <xsl:attribute name="id">programming_language_selector__<xsl:value-of select="$plang"/></xsl:attribute>
        <xsl:attribute name="class">api_programming_language_selector</xsl:attribute>

        <xsl:choose>
            <xsl:when test="$programming_language=$plang">
                <xsl:attribute name="style">font-weight: bold;</xsl:attribute>
            </xsl:when>
            <xsl:otherwise>
                <xsl:attribute name="href"><xsl:value-of select="/api.class/@name"/>-<xsl:value-of select="$plang"/>.xml</xsl:attribute>
            </xsl:otherwise>
        </xsl:choose>

        <!--
        <xsl:if test="$fixed_programming_language and not( $fixed_programming_language=$plang )">
            <xsl:attribute name="style">color: lightgray;</xsl:attribute>
            <xsl:attribute name="title">Use XML-Version of this document to show other programming languages</xsl:attribute>
        </xsl:if>

        <xsl:if test="not( $fixed_programming_language )">
            <xsl:attribute name="onclick">api.programming_language_selector__onclick( "<xsl:value-of select="$plang"/>" )</xsl:attribute>
            <xsl:attribute name="href">javascript:void(0)</xsl:attribute>
        </xsl:if>
        -->

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
            <xsl:value-of select="/*/@base_dir"/>api/<xsl:value-of select="@class"/>-<xsl:value-of select="$programming_language"/>.xml
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
                        <xsl:text xml:lang="de">Übersicht</xsl:text>
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
            <xsl:choose>
                <xsl:when test="@name='api'">Programmschnittstelle (API)</xsl:when>
                <xsl:otherwise><xsl:value-of select="@name"/> - Programmschnittstelle (API)</xsl:otherwise>
            </xsl:choose>
        </xsl:variable>

        <xsl:call-template name="html_head">
            <xsl:with-param name="title" select="$title"/>
        </xsl:call-template>

        <body>
            <xsl:call-template name="body_start">
                <xsl:with-param name="title" select="$title"/>
            </xsl:call-template>


            <table cellpadding="0" cellspacing="0" style="padding-bottom: 4ex">
                <tr>
                    <td style="vertical-align: top; padding-right: 3ex; padding-bottom: 4pt;">
                        <xsl:call-template name="all_classes">   <!-- Übersicht -->
                            <xsl:with-param name="class_references"  select="document( 'all_classes.xml' )/class_references/class_reference [ @class='api' ]"/>
                            <xsl:with-param name="active_class"      select="/api.class/@name"/>
                        </xsl:call-template>
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
                    </td>
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
                    <td style="vertical-align: top; padding-right: 3ex; width: 20pt;">
                        <!--p style="font-weight: bold; padding-bottom: 1em">Klassen:</p-->
                        <xsl:call-template name="all_classes">
                            <xsl:with-param name="class_references"  select="document( 'all_classes.xml' )/class_references/class_reference [ @class != 'api' ]"/>
                            <xsl:with-param name="active_class"      select="/api.class/@name"/>
                        </xsl:call-template>
                    </td>

                    <td style="padding-left: 3ex; border-left: 1px dotted black;">
                        <!--xsl:apply-templates select="." mode="headline"/-->
                        <xsl:if test="@name != 'api'">
                            <p class="api_headline">
                                <xsl:value-of select="@name"/>

                                <xsl:if test="title">
                                    &#160;–&#160; <xsl:value-of select="title"/>
                                </xsl:if>
                            </p>
                        </xsl:if>

                        <p>&#160;</p>
                        <xsl:apply-templates select="." mode="table"/>
                        
                        <xsl:apply-templates select="." mode="example"/>

                        <xsl:if test="description [ not ( @programming_language ) ]">
                            <p>&#160;</p>
                            <!--p style="margin-top: 0em">&#160;</p-->
                            <xsl:apply-templates select="description [ not ( @programming_language ) ]"/>
                        </xsl:if>
                        
                        <xsl:if test="description [ @programming_language = $programming_language ]">
                            <p style="margin-top: 0em">&#160;</p>
                            <xsl:apply-templates select="description [ @programming_language = $programming_language ]"/>
                        </xsl:if>
                        
                        <p style="margin-top: 0em">&#160;</p>
                        

                        <xsl:apply-templates select="." mode="detailed_methods"/>
                    </td>
                </tr>
            </table>

            <!--
                <xsl:element name="script">
                    <xsl:attribute name="defer">defer</xsl:attribute>
                    <xsl:attribute name="type">text/javascript</xsl:attribute>
                    <xsl:attribute name="src"><xsl:value-of select="/*/@base_dir"/>scripts/browser_dependencies.js</xsl:attribute>
                </xsl:element>

                <xsl:element name="script">
                    <xsl:attribute name="defer">defer</xsl:attribute>
                    <xsl:attribute name="type">text/javascript</xsl:attribute>
                    <xsl:attribute name="src"><xsl:value-of select="/*/@base_dir"/>scripts/sarissa.js</xsl:attribute>
                </xsl:element>
            -->
                <xsl:element name="script">
                    <xsl:attribute name="defer">defer</xsl:attribute>
                    <xsl:attribute name="type">text/javascript</xsl:attribute>
                    <xsl:attribute name="src"><xsl:value-of select="/*/@base_dir"/>api/api.js</xsl:attribute>
                </xsl:element>

            <!--
                <script defer="defer" type="text/javascript" for="window" event="onload">

                    href_base = document.location.href.replace( /\/[^\/]*$/, "/" );   // Alles bis zum letzten Schräger
                    base_dir = href_base + "<xsl:value-of select="/*/@base_dir"/>";

                    api = new Api();

                    api._class_name = "<xsl:value-of select="/api.class/@name"/>";

                    api.show();
                    api.highlight_html_selectors( true );

                </script>
            -->

            <xsl:call-template name="bottom"/>
        </body>
    </html>
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
    
    <xsl:if test="description [ @programming_language = $programming_language ]">
        <p style="margin-top: 0em">&#160;</p>
        <xsl:apply-templates select="description [ @programming_language = $programming_language ]"/>
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

        <xsl:apply-templates select="  property [ not( not_implemented/@programming_language=$programming_language ) ] 
                                     | method   [ not( not_implemented/@programming_language=$programming_language ) ]" mode="table">
            <xsl:sort select="@name"/>
        </xsl:apply-templates>

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

    <xsl:choose>
        <xsl:when test="$language_has_properties and false()">
            <xsl:apply-templates select="com" mode="table_row">
                <xsl:with-param name="is_in_table" select="$is_in_table"/>
                <xsl:with-param name="show_title"  select="$show_title"/>
            </xsl:apply-templates>
        </xsl:when>
        <xsl:otherwise>
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
        </xsl:otherwise>
    </xsl:choose>
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
    <xsl:variable name="tr_id"        select="concat( $method_tr_id, '.', position(), $access )"/>

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
                <xsl:when test="not( $programming_language='java' )">
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
                    <xsl:apply-templates select="parent::*/title"/>
                </xsl:if>
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

    <xsl:if test="not( $programming_language='perl' )">
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

    <xsl:if test="not( $programming_language='perl' ) or $parameters or parent::method">
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

            <span class="mono"><xsl:text> </xsl:text></span>
        </xsl:if>

        <span class="mono"><xsl:value-of select="@name"/></span>
    </span>

</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="com.type [ @class ]">
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
    <xsl:if test="$programming_language!='java'">
        <xsl:element name="span">
            <xsl:attribute name="title">A Scheduler class name, not a real class name in <xsl:value-of select="$programming_language"/></xsl:attribute>        
            <xsl:attribute name="style">cursor: default</xsl:attribute>        
            <xsl:text>¹</xsl:text>
        </xsl:element>
    </xsl:if>
    -->
    
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="com.type [ @type ]">
    <span class="mono"><xsl:value-of select="@type"/></span>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="com.type [ @type='BSTR' ]">
    <span class="mono">
        <xsl:text>String</xsl:text>
        <xsl:if test="@array">[]</xsl:if>
    </span>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="com.type [ @type='BSTR' and @array ]">
    <span class="mono" title="Array of Strings">String[]</span>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="com.type [ @type='VARIANT*' ]">

    <xsl:choose>
        <xsl:when test="com.type [ @type ]">
            <xsl:for-each select="com.type [ @type ]">
                <xsl:if test="position() &gt; 1">|</xsl:if>
                <xsl:apply-templates select="."/>
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

<xsl:template match="api.null" mode="description">
    <code>null</code>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="api.empty" mode="description">
    <code>empty</code>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="api.class" mode="detailed_methods">

    <xsl:apply-templates select="  method   [ com and not( not_implemented/@programming_language=$programming_language ) ]
                                 | property [ com and not( not_implemented/@programming_language=$programming_language ) ]" 
                         mode="detailed_methods">
        <xsl:sort select="@name"/>                         
    </xsl:apply-templates>

</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="/api.class [ /*/@show_detailed_methods ]">
    <xsl:apply-templates select="." mode="detailed_methods"/>
</xsl:template>

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


    <xsl:apply-templates select="title"/>


    <xsl:if test="@deprecated">
        <p>
            *** Der Aufruf sollte nicht mehr verwendet werden ***
        </p>
    </xsl:if>


    <p style="margin-top: 0em">&#160;</p>


    <!-- Signatur -->
    <table cellpadding="0" cellspacing="0">
        <xsl:apply-templates select="." mode="table_rows">
            <xsl:with-param name="show_title" select="false()"/>
        </xsl:apply-templates>
    </table>


    <xsl:apply-templates select="." mode="example"/>


    <xsl:if test="description">
        <p>&#160;</p>
        <xsl:apply-templates select="description"/>
    </xsl:if>


    <xsl:variable name="read_result"  select="com [ @access='read' or not( @access ) and ( not( parent::*/@access ) or parent::*/@access='read' ) ]/com.result"/>
    <xsl:variable name="write_result" select="com [ @access='write' or not( @access ) and ( not( parent::*/@access ) or parent::*/@access='write' ) ]/com.result"/>

    <xsl:if test="com/com.parameter [ description ] | $write_result [ description ]">
        <h3>Parameter</h3>

        <table cellpadding="0" cellspacing="0">
            <xsl:for-each select="com/com.parameter | $write_result">
                <tr>
                    <td>
                        <span class="mono"><xsl:value-of select="@name"/></span>
                    </td>
                    <td style="padding-left: 2ex">
                        <xsl:apply-templates select="title"/>
                        <xsl:apply-templates select="description"/>
                    </td>
                </tr>
            </xsl:for-each>
        </table>
    </xsl:if>


    <xsl:if test="$read_result/com.type [ @class or parent::*/description ]">
        <h3>Rückgabe</h3>
        <xsl:apply-templates select="$read_result/com.type"/>&#160;&#160;
        <xsl:apply-templates select="com/com.result/description"/>
    </xsl:if>

</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="*" mode="example">
    <xsl:choose>
        <xsl:when test="example [ not( @programming_language )  or  @programming_language=$programming_language ]">
            <xsl:apply-templates select="example [ not( @programming_language )  or  @programming_language=$programming_language ]"/>
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
<!--
<xsl:include href="api_java.xsl" />
<xsl:include href="api_javascript.xsl" />
<xsl:include href="api_vbscript.xsl" />
<xsl:include href="api_perl.xsl" />
-->
<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

</xsl:stylesheet>
