<?xml version='1.0'?>
<!-- $Id$ -->

<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform" 
                version   = "1.0">

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="com.type [ /*/@programming_language='vbscript' and @type='bool' ]">
    <span class="mono">Boolean</span>
</xsl:template>

<xsl:template match="com.type [ /*/@programming_language='vbscript' and @type='int' ]">
    <span class="mono" title="32bit">Integer</span>
</xsl:template>

<xsl:template match="com.type [ /*/@programming_language='vbscript' and @type='double' ]">
    <span class="mono">Double</span>
</xsl:template>

<xsl:template match="com.type [ /*/@programming_language='vbscript' and @type='BSTR' ]">
    <span class="mono">String</span>
</xsl:template>

<xsl:template match="com.type [ /*/@programming_language='vbscript' and @type='VARIANT*' ]">
    <span class="mono">Variant</span>
</xsl:template>

<!--xsl:template match="com.type [ @class and /*/@programming_language='java' ]">
    <span class="mono"><xsl:value-of select="concat( 'sos.spooler.', @class )"/></span>
</xsl:template-->

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="api.null [ /*/@programming_language='vbscript' ]" mode="description">
    <code>Nothing</code>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
                
    
</xsl:stylesheet>
