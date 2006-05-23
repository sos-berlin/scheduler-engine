<?xml version='1.0'?>
<!-- $Id$ -->

<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform"
                version   = "1.0">

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:variable name="selected_programming_language" select="'vbscript'"/>
<xsl:variable name="language_has_properties"       select="true()"/>

<xsl:include href="api.xsl" />

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="com.type [ @type='bool' ]" mode="no_array">
    <span class="mono">Boolean</span>
</xsl:template>

<xsl:template match="com.type [ @type='int' ]" mode="no_array">
    <span class="mono" title="32bit">Integer</span>
</xsl:template>

<xsl:template match="com.type [ @type='double' ]" mode="no_array">
    <span class="mono">Double</span>
</xsl:template>

<xsl:template match="com.type [ @type='BSTR' ]" mode="no_array">
    <span class="mono">String</span>
</xsl:template>

<!--xsl:template match="com.type [ @type='BSTR' and @array ]">
    <span class="mono" title="Array of strings">String[]</span>
</xsl:template-->

<xsl:template match="com.type [ @type='VARIANT' and not( com.type ) ]" mode="no_array">
    <span class="mono">Variant</span>
</xsl:template>

<!--xsl:template match="com.type [ @class and /*/@programming_language='java' ]">
    <span class="mono"><xsl:value-of select="concat( 'sos.spooler.', @class )"/></span>
</xsl:template-->

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="api.null" mode="description">
    <code>Nothing</code>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="api.empty" mode="description">
    <code>Empty</code>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->


</xsl:stylesheet>
