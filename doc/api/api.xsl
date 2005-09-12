<?xml version='1.0'?>
<!-- $Id$ -->



<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform" 
                version   = "1.0">
                
<xsl:include href="../scheduler.xsl" />
<xsl:include href="api_java.xsl" />

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

<xsl:template match="api_programming_language_selector" mode="description">
    <xsl:variable name="plang" select="translate( @programming_language, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz' )"/>
    <xsl:element name="a">
        <xsl:attribute name="id">programming_language_selector__<xsl:value-of select="$plang"/></xsl:attribute>
        <xsl:attribute name="onclick">api.programming_language_selector__onclick( "<xsl:value-of select="$plang"/>" )</xsl:attribute>
        <xsl:attribute name="class">api_programming_language_selector</xsl:attribute>
        <xsl:attribute name="href">javascript:void(0)</xsl:attribute>
        
        <xsl:value-of select="@programming_language"/>
    </xsl:element>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="api_classes" mode="description">
    <xsl:apply-templates select="document( 'all_classes.xml' )/class_references/class_reference" mode="script"/>
</xsl:template>

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
<!-- Klick auf den Klassennamen ruft im rechten Teil die Beschreibung der Klasse auf -->

<xsl:template match="class_reference" mode="script">
    <p style="margin-top: 0px">
        <xsl:element name="a">
            <xsl:attribute name="id">class_reference_<xsl:value-of select="@name"/></xsl:attribute>
            <xsl:attribute name="href">javascript:void(0)</xsl:attribute>  <!-- Für ie6 -->
            <xsl:attribute name="onclick">api.class_reference__onclick( "<xsl:value-of select="@name"/>" );</xsl:attribute>
            <xsl:value-of select="@name"/>
        </xsl:element>
    </p>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="/class [ /*/@show_headline ]">
    <xsl:apply-templates select="." mode="headline"/>
</xsl:template>    

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="class" mode="headline">
    <p style="margin-top: 0ex; padding-bottom: 3ex; font-size: 14pt; font-weight: bold;">
        <xsl:value-of select="@name"/>

        <xsl:if test="@title">
             - <xsl:value-of select="@title"/>
        </xsl:if>
    </p>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="/class [ /*/@show_table ]">
    <xsl:apply-templates select="." mode="table"/>
</xsl:template>    

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="class" mode="table">
    <table cellpadding="0" cellspacing="0">
        <!--tr><td colspan="4" style="padding-top: 4ex; padding-bottom: 1ex; font-weight: bold;">Eigenschaften</td></tr-->
        <tr>
            <td class="api_type"       >Typ</td>
            <td class="api_method"     style="font-weight: bold">Eigenschaft&#160;</td>
            <td class="api_parameters" >Parameter</td>
            <td class="api_access"     > </td>
            <td class="api_title"      > </td>
        </tr>

        <tr>
            <td colspan="5" style="padding-bottom: 4pt;"><hr/></td>
        </tr>
        
        <xsl:apply-templates select="property" mode="table">
            <xsl:sort select="@name"/>
        </xsl:apply-templates>

        
        <tr><td colspan="4" style="padding-top: 6ex; padding-bottom: 1ex; font-weight: bold;">&#160;</td></tr>
        <tr>
            <td class="api_type"       >Ergebnistyp</td>
            <td class="api_method"     style="font-weight: bold">Methode&#160;</td>
            <td class="api_parameters" >Parameter</td>
            <td class="api_access"     > </td>
            <td class="api_title"      > </td>
        </tr>
        
        <tr>
            <td colspan="5" style="padding-bottom: 4pt;"><hr/></td>
        </tr>
        
        <xsl:apply-templates select="method" mode="table">
            <xsl:sort select="@name"/>
        </xsl:apply-templates>
    </table>

</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="property | method" mode="table">

    <xsl:if test="position() &gt; 1">
        <!-- Trenner zwischen verschiedenen Eigenschaften oder Methoden -->
        
        <tr>
            <td colspan="99" style="font-size: 4pt">
                <!--hr style="color: lightgrey; background-color: lightgrey"/-->
                &#160;
            </td>
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
        <xsl:when test="/*/@language_has_properties">
            <xsl:apply-templates select="com" mode="table_row">
                <xsl:with-param name="is_in_table" select="$is_in_table"/>
                <xsl:with-param name="show_title"  select="$show_title"/>
            </xsl:apply-templates>
        </xsl:when>
        <xsl:otherwise>
            <xsl:for-each select="com">
                <xsl:choose>
                    <xsl:when test="parent::property and parent::property/@access='write'">
                        <xsl:apply-templates select="." mode="table_row">
                            <xsl:with-param name="access"      select="'write'"/>
                            <xsl:with-param name="is_in_table" select="$is_in_table"/>
                            <xsl:with-param name="show_title"  select="$show_title"/>
                        </xsl:apply-templates>
                    </xsl:when>
                        
                    <xsl:when test="parent::property and parent::property/@access='read'">
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
    
    <xsl:variable name="tr_id" select="concat( local-name(.), '.', position(), $access )"/>
    
    <xsl:element name="tr">
        <xsl:if test="$is_in_table">
            <xsl:attribute name="id">tr_<xsl:value-of select="$tr_id"/></xsl:attribute>
            <xsl:attribute name="class">api_method</xsl:attribute>
            <xsl:attribute name="onclick">window.location = "#method__<xsl:value-of select="parent::*/@name"/>";</xsl:attribute>
            <xsl:attribute name="style">cursor: hand;</xsl:attribute>
        </xsl:if>
    
    
        <td class="api_type">
            <xsl:choose>
                <xsl:when test="( java.result or com.result ) and not( $access='write' )">
                    <xsl:apply-templates select="java.result/java.type | com.result/com.type"/>
                </xsl:when>
                <xsl:otherwise>
                    <span class="mono">void</span>
                </xsl:otherwise>
            </xsl:choose>
        </td>

        <td class="api_method">
            <xsl:apply-templates select="parent::*" mode="method_name">
                <xsl:with-param name="access" select="$access"/>
            </xsl:apply-templates>
        </td>
        
        <xsl:element name="td">
            <xsl:attribute name="class">api_parameters</xsl:attribute>
            
            <xsl:if test="not( /*/@language_has_properties and parent::property )">
                <xsl:attribute name="colspan">2</xsl:attribute>
            </xsl:if>
            
            <xsl:if test="com.parameter or parent::method or not( /*/@language_has_properties )">
                <xsl:choose>
                    <xsl:when test="$access='write'">
                        <xsl:apply-templates select="." mode="parameter_list">
                            <xsl:with-param name="parameters" select="java.parameter | java.result | com.parameter | com.result"/>
                        </xsl:apply-templates>
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:apply-templates select="." mode="parameter_list">
                            <xsl:with-param name="parameters" select="java.parameter | com.parameter"/>
                        </xsl:apply-templates>
                    </xsl:otherwise>
                </xsl:choose>
            </xsl:if>
        </xsl:element>
        
        <xsl:if test="parent::property and /*/@language_has_properties">
            <td class="api_access">
                <xsl:if test="$access">
                    <xsl:value-of select="$access"/> only
                </xsl:if>
            </td>
        </xsl:if>

    </xsl:element>    
    
    
    <xsl:if test="parent::*/title and $show_title and $title_rowspan &gt; 0 and position() = last()">
        <tr>
            <td> </td>
            
            <td colspan="99" class="api_title">
                <xsl:value-of select="parent::*/title"/>
            </td>
        </tr>
    </xsl:if>

</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="method | property" mode="method_name">
    <!--
    <span style="font-size: 8pt"><xsl:value-of select="parent::*/parent::class/@object_name"/></span>
    <xsl:text>.</xsl:text>
    -->
    <span class="mono" style="font-weight: bold"><xsl:value-of select="@name"/></span>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
<!-- ( parameter ... ) -->

<xsl:template match="java | com" mode="parameter_list">
    <xsl:param name="parameters"/>
    
    <span class="mono"><xsl:text>(</xsl:text></span>

    <xsl:if test="$parameters">
        <span class="mono"><xsl:text> </xsl:text></span>
        <xsl:for-each select="$parameters">
            <xsl:if test="position() &gt; 1">
                <span class="mono"><xsl:text>, </xsl:text></span>
            </xsl:if>
            <xsl:apply-templates select="."/>
        </xsl:for-each>
        
        <span class="mono"><xsl:text> </xsl:text></span>
    </xsl:if>

    <span class="mono">)</span>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
<!-- Parametertyp und -name -->

<xsl:template match="com.parameter | com.result | java.parameter | java.result">

    <span style="white-space: nowrap">
        <xsl:apply-templates select="java.type | com.type"/>
        <span class="mono"><xsl:text> </xsl:text></span>
        <span class="mono"><xsl:value-of select="@name"/></span>
    </span>
    
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="com.type [ /*/@programming_language='javascript' and @type='VARIANT*' ]">
    <span class="mono">var</span>
</xsl:template>

<xsl:template match="com.type [ /*/@programming_language='javascript' and @type='bool' ]">
    <span class="mono">boolean</span>
</xsl:template>

<xsl:template match="com.type [ /*/@programming_language='javascript' and @type='BSTR' ]">
    <span class="mono">String</span>
</xsl:template>

<xsl:template match="com.type [ /*/@programming_language='javascript' and @class ]">
    <span class="mono">Object</span> (<xsl:value-of select="@class"/>)
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="com.type">
    <span class="mono"><xsl:value-of select="@type | @class"/></span>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="java.type">
    <span class="mono"><xsl:value-of select="@type | @class"/></span>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="class" mode="detailed_methods">
    
    <p style="padding-top: 3em">&#160;</p>
    
    <xsl:apply-templates select="method | property" mode="detailed_methods"/>
    
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="/class [ /*/@show_detailed_methods ]">
    <xsl:apply-templates select="." mode="detailed_methods"/>
</xsl:template>    

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="method | property" mode="detailed_methods">
    
    <xsl:element name="h2">
        <xsl:attribute name="id">method__<xsl:value-of select="@name"/></xsl:attribute>
        
        <xsl:value-of select="@name"/>
        
        <xsl:if test="title">
            &#160; – &#160; <xsl:value-of select="title"/>
        </xsl:if>
    </xsl:element>

    <table cellpadding="0" cellspacing="0">
        <xsl:apply-templates select="." mode="table_rows">
            <xsl:with-param name="show_title" select="false()"/>
        </xsl:apply-templates>
    </table>

    <xsl:if test="description">
        <p>&#160;</p>
        <xsl:apply-templates select="description"/>
    </xsl:if>
    
    <xsl:if test="com.parameter">
        <h3>Parameter</h3>
        
        <table cellpadding="0" cellspacing="0">
            <xsl:for-each select="com.parameter">
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
    
    <xsl:if test="com.type[ title or description ]">
        <h3>Rückgabe</h3>
        <xsl:apply-templates select="com.type/title"/>
        <xsl:apply-templates select="com.type/description"/>
    </xsl:if>
    
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="com" mode="detailed_methods">
    
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
    
</xsl:stylesheet>
