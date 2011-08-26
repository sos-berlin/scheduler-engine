<?xml version='1.0'?>
<!-- $Id: api-javascript.xsl 12125 2006-06-14 12:00:39Z jz $ -->

<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform"
                version   = "1.0">

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:variable name="selected_programming_language" select="'javascript'"/>
<xsl:variable name="language_has_properties"       select="true()"/>

<xsl:include href="api.xsl" />

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="com.type [ @type='VARIANT' and not( com.type ) ]" mode="no_array">
    <span class="mono">var</span>
</xsl:template>

<xsl:template match="com.type [ @type='bool' ]" mode="no_array">
    <span class="mono">boolean</span>
</xsl:template>

<xsl:template match="com.type [ @type='DATE' ]" mode="no_array">
    <span class="mono">Date</span>
</xsl:template>

<xsl:template match="com.type [ @type='BSTR' ]" mode="no_array">
    <span class="mono">string</span>
</xsl:template>

<!--xsl:template match="com.type [ @type='BSTR' and @array ]">
    <span class="mono" title="Array of strings">string[]</span>
</xsl:template-->

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="api.empty" mode="description">
    <code>undefined</code>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->


</xsl:stylesheet>
