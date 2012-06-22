<?xml version='1.0'?>
<!-- $Id$ -->

<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform"
                version   = "1.0">

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:variable name="selected_programming_language" select="'powershell'"/>
<xsl:variable name="language_has_properties"       select="true()"/>

<xsl:include href="api.xsl" />

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<!--xsl:include href="api.xsl" /-->

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
<xsl:template match="com.type [ @type='bool' ]" mode="no_array">
    <span class="mono">[bool]</span>
</xsl:template>

<xsl:template match="com.type [ @type='int' ]" mode="no_array">
    <span class="mono" title="32bit">[int]</span>
</xsl:template>

<xsl:template match="com.type [ @type='double' ]" mode="no_array">
    <span class="mono">[double]</span>
</xsl:template>

<xsl:template match="com.type [ @type='BSTR' ]" mode="no_array">
    <span class="mono">[string]</span>
</xsl:template>

<xsl:template match="com.type [ @type='DATE' ]" mode="no_array">
    <span class="mono">[object]</span>
</xsl:template>

<xsl:template match="com.type [ @type='VARIANT' and not( com.type ) ]" mode="no_array">
    <span class="mono">[object]</span>
</xsl:template>





<xsl:template match="api.empty" mode="description">
    <code>""</code>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="property | method" mode="method_name">
    <xsl:param name="access"/>

    <xsl:if test="parent::api.class/@object_name">
        <span class="api_object_name">
            $<xsl:value-of select="parent::api.class/@object_name"/>
            <xsl:text>.</xsl:text>
        </span>
    </xsl:if>

    <span class="mono" style="font-weight: bold">
        <xsl:choose>
            <xsl:when test="self::property[com/com.parameter] and $access='write'">
                <xsl:text>set_</xsl:text>                
                    <xsl:value-of select="@name"/>                
            </xsl:when>
            <xsl:otherwise>
				<xsl:value-of select="@name"/>
			</xsl:otherwise>
        </xsl:choose>
    </span>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->


</xsl:stylesheet>
