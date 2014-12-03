<?xml version='1.0'?>

<xsl:stylesheet xmlns:xsl = "http://www.w3.org/1999/XSL/Transform"
                version   = "1.0">

    <xsl:template match="/">
    
        <service_response>
            <content>
                 <xsl:copy-of select="/*" />
            </content>
        </service_response>

    </xsl:template>

</xsl:stylesheet>
