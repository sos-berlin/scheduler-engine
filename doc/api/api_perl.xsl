<?xml version='1.0'?>
<!-- $Id$ -->

<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform" 
                version   = "1.0">

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="property [ /*/@programming_language='perl' ] | method [ /*/@programming_language='perl' ]" mode="method_name">
    <xsl:param name="access"/>

    <span class="object_name">
        $<xsl:value-of select="parent::api.class/@object_name"/>
        <xsl:text>-></xsl:text>
    </span>        
    
    <span class="mono">
        <xsl:choose>
            <xsl:when test="$access='write'">
                <xsl:text>LetProperty(</xsl:text> 
                '<span style="font-weight: bold">
                    <xsl:value-of select="@name"/>
                </span>',
            </xsl:when>
            
            <xsl:when test="$access='read'">
                <span style="font-weight: bold">
                    <xsl:value-of select="@name"/>
                </span>
            </xsl:when>
            
            <xsl:otherwise>
                <span style="font-weight: bold">
                    <xsl:value-of select="@name"/>
                </span>
                <xsl:text>(</xsl:text> 
            </xsl:otherwise>
        </xsl:choose>
    </span>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="com.type [ /*/@programming_language='perl' and @type='bool' ]">
    <span class="mono">boolean</span>
</xsl:template>

<!--xsl:template match="com.type [ @class and /*/@programming_language='java' ]">
    <span class="mono"><xsl:value-of select="concat( 'sos.spooler.', @class )"/></span>
</xsl:template-->

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->
                
    
</xsl:stylesheet>
