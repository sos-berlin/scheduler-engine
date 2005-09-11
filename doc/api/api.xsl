<?xml version='1.0'?>
<!-- $Id: scheduler.xsl 3860 2005-09-06 08:38:41Z jz $ -->



<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform" 
                version   = "1.0">
                
<xsl:include href="../scheduler.xsl" />

<xsl:output doctype-public="-//W3C//DTD HTML 4.01//EN" />  <!--"http://www.w3.org/TR/html4/strict.dtd"-->
<!--xsl:output doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"/> <!- - "http://www.w3.org/TR/html4/loose.dtd"-->

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

<xsl:template match="api_classes" mode="description">
    <xsl:apply-templates select="document( 'all_classes.xml' )/class_references/class_reference" mode="script"/>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="class_reference">
    <xsl:element name="a">
        <xsl:attribute name="href"><xsl:value-of select="@name"/>.xml</xsl:attribute>
        <xsl:value-of select="@name"/>
    </xsl:element>
    <br/>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="class_reference" mode="script">
    <xsl:element name="a">
        <xsl:attribute name="href">javascript:void(0)</xsl:attribute>  <!-- Für ie6 -->
        <xsl:attribute name="onclick">api.class_reference_onclick( "<xsl:value-of select="@name"/>" ); api.show(); </xsl:attribute>
        <xsl:value-of select="@name"/>
    </xsl:element>
    <br/>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="/class [ /*/@show_list ]">
    <xsl:apply-templates select="." mode="list"/>
</xsl:template>    

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="class" mode="list">
    <p style="margin-top: 0px; padding-bottom: 3ex; font-weight: bold;">
        <xsl:value-of select="@name"/>
        <xsl:if test="@title">
             - <xsl:value-of select="@title"/>
        </xsl:if>
    </p>

    <table cellpadding="0" cellspacing="0">
        <!--tr><td colspan="4" style="padding-top: 4ex; padding-bottom: 1ex; font-weight: bold;">Eigenschaften</td></tr-->
        <tr>
            <td class="api_type"       style="font-size: 8pt">Typ</td>
            <td class="api_method"     style="font-size: 8pt">Eigenschaft&#160;</td>
            <td class="api_parameters" style="font-size: 8pt">Parameter</td>
            <td class="api_access"     style="font-size: 8pt"> </td>
            <td class="api_title"      style="font-size: 8pt"> </td>
        </tr>
        <tr>
            <td colspan="5"><hr size="1"/></td>
        </tr>
        
        <xsl:apply-templates select="property" mode="list_properties_and_methods">
            <xsl:sort select="@name"/>
        </xsl:apply-templates>

        
        <tr><td colspan="4" style="padding-top: 6ex; padding-bottom: 1ex; font-weight: bold;"> </td></tr>
        <tr>
            <td class="api_type"       style="font-size: 8pt">Ergebnistyp</td>
            <td class="api_method"     style="font-size: 8pt">Methode&#160;</td>
            <td class="api_parameters" style="font-size: 8pt">Parameter</td>
            <td class="api_access"     style="font-size: 8pt"> </td>
            <td class="api_title"      style="font-size: 8pt"> </td>
        </tr>
        
        <tr>
            <td colspan="5"><hr size="1"/></td>
        </tr>
        
        <xsl:apply-templates select="method" mode="list_properties_and_methods">
            <xsl:sort select="@name"/>
        </xsl:apply-templates>
    </table>

</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="property | method" mode="list_properties_and_methods">
    <xsl:if test="position() &gt; 1">
        <tr><td colspan="99" style="font-size: 1px"><hr style="color: lightgrey; background-color: lightgrey"/></td></tr>
    </xsl:if>

    <xsl:choose>
        <xsl:when test="com">
            <xsl:apply-templates select="com" mode="table_row"/>
        </xsl:when>
        <xsl:otherwise>
            <xsl:apply-templates select="." mode="table_row"/>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="property [ /*/@programming_language='java' ] | method [ /*/@programming_language='java' ]" mode="list_properties_and_methods">

    <xsl:if test="position() &gt; 1">
        <tr><td colspan="99" style="font-size: 1px"><hr style="color: lightgrey; background-color: lightgrey"/></td></tr>
    </xsl:if>

    <xsl:choose>
        <xsl:when test="java">
            <xsl:apply-templates select="java" mode="table_row"/>
        </xsl:when>
        
        <xsl:when test="com">
            <xsl:for-each select="com">
                <xsl:choose>
                    <xsl:when test="parent::property and @access='write'">
                            <xsl:apply-templates select="." mode="table_row">
                                <xsl:with-param name="access" select="'write'"/>
                            </xsl:apply-templates>
                    </xsl:when>
                        
                    <xsl:when test="parent::property and @access='read'">
                        <xsl:apply-templates select="." mode="table_row">
                            <xsl:with-param name="access" select="'read'"/>
                        </xsl:apply-templates>
                    </xsl:when>
                    
                    <xsl:when test="parent::property and not( @access )">
                        <xsl:apply-templates select="." mode="table_row">
                            <xsl:with-param name="access" select="'write'"/>
                        </xsl:apply-templates>
                    
                        <xsl:apply-templates select="." mode="table_row">
                            <xsl:with-param name="access" select="'read'"/>
                        </xsl:apply-templates>
                    </xsl:when>
                    
                    <xsl:otherwise>
                        <xsl:apply-templates select="." mode="table_row"/>
                    </xsl:otherwise>
                </xsl:choose>
            </xsl:for-each>
        </xsl:when>
        
        <xsl:otherwise>
            <xsl:apply-templates select="." mode="table_row"/>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
<!--
<xsl:template match="property | method" mode="list">
    <p>
        <xsl:apply-templates select="com" mode="list"/>
    </p>
</xsl:template>
-->

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
<!--
<xsl:template match="com" mode="table_row">
    <tr>
        <td class="api_type">
            <xsl:apply-templates select="." mode="com.type"/>
        </td>

        <td class="api_method">
            <xsl:apply-templates select="." mode="method_name"/>
        </td>
        
        <xsl:element name="td">
            <xsl:attribute name="class">api_parameters</xsl:attribute>
            <xsl:if test="not( parent::property )">
                <xsl:attribute name="colspan">2</xsl:attribute>
            </xsl:if>
            
            <xsl:if test="com.parameter or parent::method or /*/@programming_language='java'">
                <xsl:apply-templates select="." mode="parameter_list"/>
            </xsl:if>
        </xsl:element>
        
        <xsl:if test="parent::property">
            <td class="api_access">
                <xsl:if test="parent::property/@access or @access">
                    <xsl:value-of select="parent::property/@access | @access"/> only
                </xsl:if>
            </td>
        </xsl:if>
        
        <td class="api_title">
            <xsl:value-of select="parent::property/title"/>
        </td>
    </tr>
    
</xsl:template>
-->
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

<xsl:template match="com" mode="table_row">
    <xsl:param name="access"/>
    
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
            <xsl:if test="not( parent::property )">
                <xsl:attribute name="colspan">2</xsl:attribute>
            </xsl:if>
            
            <xsl:if test="com.parameter or parent::method or /*/@programming_language='java'">
                <xsl:apply-templates select="." mode="parameter_list">
                    <xsl:with-param name="with_property_type" select="$access='write'"/>
                </xsl:apply-templates>
            </xsl:if>
        </xsl:element>
        
        <xsl:if test="parent::property and not( /*/@programming_language='java' )">
            <td class="api_access">
                <xsl:if test="@access">
                    <xsl:value-of select="@access"/> only
                </xsl:if>
            </td>
        </xsl:if>
        
        <td class="api_title">
            <xsl:value-of select="parent::property/title"/>
        </td>

    </tr>    
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="java [ /*/@programming_language='java' ]" mode="table_row">
    <xsl:param name="access"/>

    <xsl:element name="tr">
        <!--xsl:attribute name="style">
            <xsl:if test="parent::* [ position() =2 ]">
                background-color: lightgrey;
            </xsl:if>
        </xsl:attribute-->
        
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
                <xsl:with-param name="access" select="parent::property/@access"/>
            </xsl:apply-templates>
        </td>
        
        <td class="api_parameters" colspan="2">
            <xsl:apply-templates select="." mode="parameter_list">
                <xsl:with-param name="with_property_type" select="parent::property/@access='write'"/>
            </xsl:apply-templates>
        </td>
        
        <td class="api_title">
            <xsl:value-of select="parent::property/title"/>
        </td>
    </xsl:element>
    
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

<xsl:template match="method [ /*/@programming_language='java' ] | property [ /*/@programming_language='java' ] " mode="method_name">
    <xsl:param name="access"/>

    <!--    
    <span style="font-size: 8pt"><xsl:value-of select="parent::*/parent::class/@object_name"/></span>
    <xsl:text>.</xsl:text>
    -->
    
    <span class="mono" style="font-weight: bold">
        <xsl:if test="$access='write'">set_</xsl:if>
        <xsl:if test="$access='read'">&#160;&#160;&#160;&#160;</xsl:if>
        <xsl:value-of select="@name"/>
    </span>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

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
            
            <span class="mono">
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

<xsl:template match="com.parameter | java.parameter">
    <xsl:apply-templates select="java.type | com.type"/>
    <span class="mono"><xsl:text> </xsl:text></span>
    <span class="mono"><xsl:value-of select="@name"/></span>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="*" mode="com.type">
    <xsl:param name="access"/>
    
    <xsl:choose>
        <xsl:when test="( /*/@programming_language='java' and $access='write' ) or not( java.type or com.type )">
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

<xsl:template match="com.type [ /*/@programming_language='java' and @type='bool' ]">
    <span class="mono">boolean</span>
</xsl:template>

<xsl:template match="com.type [ /*/@programming_language='java' and @type='BSTR' ]">
    <span class="mono">java.lang.String</span>
</xsl:template>

<xsl:template match="com.type [ /*/@programming_language='java' and @type='VARIANT*' ]">
    <span class="mono">java.lang.String</span>
</xsl:template>

<xsl:template match="com.type [ @class and /*/@programming_language='java' ]">
    <span class="mono"><xsl:value-of select="concat( 'sos.spooler.', @class )"/></span>
</xsl:template>

<xsl:template match="com.type">
    <span class="mono"><xsl:value-of select="@type | @class"/></span>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="java.type">
    <span class="mono"><xsl:value-of select="@type | @class"/></span>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~scheduler_commands-->
    
</xsl:stylesheet>

<!-- Das ist ein Gedankenstrich: – -->
<!-- Das ist drei Punkte: … -->
