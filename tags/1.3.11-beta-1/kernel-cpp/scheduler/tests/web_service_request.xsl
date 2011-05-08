<?xml version='1.0'?>

<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform"
                version   = "1.0">

    <xsl:param name="job_chain"/>
    <xsl:param name="parameter"/>
    <xsl:param name="parameter2"/>
    <xsl:param name="parameter3"/>
    <xsl:param name="parameter4"/>

    <xsl:template match="/service_request">

        <!--xsl:element name="start_job">
            <xsl:attribute name="web_service"><xsl:value-of select="web_service/@name"/></xsl:attribute>
            <xsl:attribute name="job">job</xsl:attribute>
            <xsl:attribute name="at">period</xsl:attribute>
            
            <params>
                <xsl:element name="param">
                    <xsl:attribute name="name">var1</xsl:attribute>
                    <xsl:attribute name="value"><xsl:value-of select="content/my_request/text()"/></xsl:attribute>
                </xsl:element>
            </params>
        </xsl:element-->

        <xsl:element name="add_order">
            <xsl:attribute name="web_service"><xsl:value-of select="web_service/@name"/></xsl:attribute>
            <xsl:attribute name="job_chain"><xsl:value-of select="$job_chain"/></xsl:attribute>
            
            <params>
                <xsl:element name="param">
                    <xsl:attribute name="name">var1</xsl:attribute>
                    <xsl:attribute name="value"><xsl:value-of select="content/my_request/text()"/></xsl:attribute>
                </xsl:element>
                <xsl:element name="param">
                    <xsl:attribute name="name">parameter</xsl:attribute>
                    <xsl:attribute name="value"><xsl:value-of select="$parameter"/></xsl:attribute>
                </xsl:element>
                <xsl:element name="param">
                    <xsl:attribute name="name">parameter2</xsl:attribute>
                    <xsl:attribute name="value"><xsl:value-of select="$parameter2"/></xsl:attribute>
                </xsl:element>
                <xsl:element name="param">
                    <xsl:attribute name="name">parameter3</xsl:attribute>
                    <xsl:attribute name="value"><xsl:value-of select="$parameter3"/></xsl:attribute>
                </xsl:element>
                <xsl:element name="param">
                    <xsl:attribute name="name">parameter4</xsl:attribute>
                    <xsl:attribute name="value"><xsl:value-of select="$parameter4"/></xsl:attribute>
                </xsl:element>
            </params>
        </xsl:element>

    </xsl:template>

</xsl:stylesheet>
