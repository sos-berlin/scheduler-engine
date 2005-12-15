<?xml version='1.0'?>
<!-- $Id$ -->

<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform"
                version   = "1.0">

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:variable name="programming_language"    select="'perl'"/>
<xsl:variable name="language_has_properties" select="false()"/>

<xsl:include href="api.xsl" />

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<!-- Für Perl wird auch die öffnende Klammer der Parameterliste erzeugt (wegen LetProperty) -->

<!--xsl:template match="property [ /*/@programming_language='perl' ] | method [ /*/@programming_language='perl' ]" mode="method_name"-->
<xsl:template match="property | method" mode="method_name">
    <xsl:param name="access"/>

    <xsl:if test="parent::api.class/@object_name">
        <span class="api_object_name">
            $<xsl:value-of select="parent::api.class/@object_name"/>
            <xsl:text>-></xsl:text>
        </span>
    </xsl:if>

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
                <xsl:if test="com/com.parameter">(</xsl:if>
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

<xsl:template match="com.type [ @type='bool' ]" mode="no_array">
    <span class="mono">Boolean</span>
</xsl:template>

<!--xsl:template match="com.type [ @class and /*/@programming_language='java' ]">
    <span class="mono"><xsl:value-of select="concat( 'sos.spooler.', @class )"/></span>
</xsl:template-->

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="api.empty" mode="description">
    <code>undefined</code>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

<xsl:template match="api.null" mode="description">
    <code>undefined</code>
</xsl:template>

<!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->


</xsl:stylesheet>
