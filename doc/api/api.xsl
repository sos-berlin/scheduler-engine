<?xml version='1.0'?>
<!-- $Id: scheduler.xsl 3860 2005-09-06 08:38:41Z jz $ -->



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

<xsl:template match="/class [ /*/@show_list ]">
    <xsl:apply-templates select="." mode="list"/>
</xsl:template>    

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="class" mode="list">
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
        
        <xsl:apply-templates select="property" mode="list_properties_and_methods">
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
        
        <xsl:apply-templates select="method" mode="list_properties_and_methods">
            <xsl:sort select="@name"/>
        </xsl:apply-templates>
    </table>

</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="property | method" mode="list_properties_and_methods">
    <xsl:if test="position() &gt; 1">
        <tr>
            <td colspan="99" style="font-size: 4pt">
                <!--hr style="color: lightgrey; background-color: lightgrey"/-->
                &#160;
            </td>
        </tr>
    </xsl:if>

    <xsl:apply-templates select="." mode="list_property_or_method"/>

</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="property | method" mode="list_property_or_method">
    <xsl:choose>
        <xsl:when test="com">
            <xsl:apply-templates select="com" mode="table_row"/>
        </xsl:when>

        <xsl:otherwise>
            <!-- Methode ohne Parameter, ohne Ergebnis -->
            <xsl:apply-templates select="." mode="table_row"/>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
<!-- Methode ohne Rückgabe, ohne Parameter -->

<xsl:template match="*" mode="table_row">
    <xsl:param name="access"/>
    
    <tr>
        <td class="api_type">
                <span class="mono">void</span>
        </td>

        <td class="api_method">
            <xsl:apply-templates select="." mode="method_name">
                <xsl:with-param name="access" select="$access"/>
            </xsl:apply-templates>
        </td>
        
        <td class="api_parameters" colspan="2">
            <span class="mono">()</span>
        </td>
        
        <td class="api_title">
            <xsl:value-of select="parent::property/title"/>
        </td>
    </tr>    
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="java | com" mode="table_row">
    <xsl:param name="access"/>
    <xsl:param name="rowspan" select="1"/>
    
    <tr>
        <td class="api_type">
            <xsl:choose>
                <xsl:when test="not( $access='write' )">
                    <xsl:apply-templates select="." mode="com.type"/>
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
                <xsl:apply-templates select="." mode="parameter_list">
                    <xsl:with-param name="with_property_type" select="$access='write'"/>
                </xsl:apply-templates>
            </xsl:if>
        </xsl:element>
        
        <xsl:if test="parent::property and /*/@language_has_properties">
            <td class="api_access">
                <xsl:if test="@access">
                    <xsl:value-of select="@access"/> only
                </xsl:if>
            </td>
        </xsl:if>
        
        <xsl:if test="$rowspan &gt; 0">
            <xsl:element name="td">
                <xsl:attribute name="class">api_title</xsl:attribute>
                <xsl:attribute name="rowspan"><xsl:value-of select="$rowspan"/></xsl:attribute>
                <xsl:attribute name="style">white-space: nowrap;</xsl:attribute>

                <!--xsl:element name="div">
                    <xsl:attribute name="style">
                        height: <xsl:value-of select="12 * $rowspan"/>pt; 
                        overflow: hidden; 
                    </xsl:attribute-->
                    
                    <xsl:value-of select="parent::*/title"/>
                <!--/xsl:element-->
            </xsl:element>
        </xsl:if>

    </tr>    
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

<xsl:template match="com | java" mode="parameter_list">
    <xsl:param name="with_property_type"/>
    
    <span class="mono"><xsl:text>(</xsl:text></span>

    <xsl:if test="com.parameter or java.parameter or $with_property_type">
        <span class="mono"><xsl:text> </xsl:text></span>
        <xsl:for-each select="com.parameter | java.parameter">
            <xsl:if test="position() &gt; 1">
                <span class="mono"><xsl:text>, </xsl:text></span>
            </xsl:if>
            <xsl:apply-templates select="."/>
        </xsl:for-each>
        
        <xsl:if test="$with_property_type">
            <xsl:if test="com.parameter or java.parameter">
                <span class="mono"><xsl:text>, </xsl:text></span>
            </xsl:if>
            
            <span class="mono" style="white-space: nowrap">
                <xsl:apply-templates select="java.type | com.type"/>
                <xsl:text> </xsl:text>
                <xsl:value-of select="java.type/@parameter_name | com.type/@parameter_name"/>
            </span>
        </xsl:if>
        
        <span class="mono"><xsl:text> </xsl:text></span>
    </xsl:if>

    <span class="mono">)</span>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
<!-- Parametertyp und -name -->

<xsl:template match="com.parameter | java.parameter">
    <xsl:apply-templates select="java.type | com.type"/>
    <span class="mono"><xsl:text> </xsl:text></span>
    <span class="mono"><xsl:value-of select="@name"/></span>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="*" mode="com.type">
    <xsl:param name="access"/>
    
    <xsl:choose>
        <xsl:when test="( not( /*/@language_has_properties ) and $access='write' ) or not( java.type or com.type )">
            <span class="mono">void</span>
        </xsl:when>
        <xsl:otherwise>
            <xsl:apply-templates select="java.type | com.type"/>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="*" mode="com.type">
    <xsl:param name="access"/>
    
    <xsl:choose>
        <xsl:when test="( not( /*/@language_has_properties ) and $access='write' ) or not( java.type or com.type )">
            <span class="mono">void</span>
        </xsl:when>
        <xsl:otherwise>
            <xsl:apply-templates select="java.type | com.type"/>
        </xsl:otherwise>
    </xsl:choose>
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
    
</xsl:stylesheet>
