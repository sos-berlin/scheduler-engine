<?xml version='1.0'?>
<!-- $Id$ -->

<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform" 
                version   = "1.0">

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<!--xsl:include href="api.xsl" /-->

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

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
                
    
</xsl:stylesheet>
