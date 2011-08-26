<?xml version='1.0'?>
<!-- $Id: messages_to_c++.xslt 12973 2007-09-09 11:10:05Z jz $ -->


<stylesheet xmlns   = "http://www.w3.org/1999/XSL/Transform"
            version = "1.0">

    <output method="text" encoding="iso-8859-1"/>

    <template match="messages">
        <text>
// Generiert mit XSLT. 
// DO NOT MODIFY!

</text>

        <if test="@include">
            <text>#include "</text>
            <value-of select="@include"/>
            <text>"&#10;&#10;</text>
        </if>

        <text>#include "../zschimmer/base.h"&#10;</text>
        <text>#include "../zschimmer/message.h"&#10;</text>
        <text>&#10;</text>
        
        <apply-templates select="cplusplus"/>

        <text>        
Message_code_text </text><value-of select="@name"/><text>[] =
{
</text>
        <apply-templates select=".//message"/>
        <text>    {}
};
</text>
        <!--text>

        Z_INIT( init_</text><value-of select="@name"/><text> )
{
    add_error_code_texts( error_codes ); 
};

</text-->

        <for-each select="cplusplus[ @namespace ]">
            <text>} //namespace </text>
            <value-of select="@namespace"/>
            <text>
</text>
        </for-each>
    </template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~<cplusplus>-->

    <template match="cplusplus">
        <text>namespace </text>
        <value-of select="@namespace"/>
        <text> { 
</text>
    </template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~<message>-->

    <template match="message">
        <text>    { "</text>
        <value-of select="@code"/>
        <text>", "</text>
        
        <choose>
            <when test="text[ @xml:lang='en' ]/title/node()">
                <apply-templates select="text[ @xml:lang='en' ]/title"/>
            </when>
            <otherwise>
                <text>(german only) </text>
                <apply-templates select="text[ @xml:lang='de' ]/title"/>
            </otherwise>
        </choose>
        
        <text>" },
</text>
</template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~<title>-->

    <template match="title">
        <for-each select="node()">
            <apply-templates select="." mode="text"/>
        </for-each>
    </template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~mode=text-->
    <!-- Alles kopieren, außer <scheduler_element> usw. -->

    <template match="node()" mode="text">
        <copy>
            <for-each select="@*">
                <copy>
                    <value-of select="."/>
                </copy>
            </for-each>
            <apply-templates mode="text"/>
        </copy>
    </template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

    <template match="text()" mode="text">
        <call-template name="replace-substring">
            <with-param name="value" select="translate( string(.), '&#10;', ' ' )"/>
            <with-param name="from" select="'&quot;'"/>
            <with-param name="to" select="'\&quot;'"/>
        </call-template>
    </template>

    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~<p1>-->

    <template match="p1" mode="text">
        <text>$1</text>
    </template>

    <template match="p2" mode="text">
        <text>$2</text>
    </template>

    <template match="p3" mode="text">
        <text>$3</text>
    </template>
    
    <template match="p4" mode="text">
        <text>$4</text>
    </template>
    
    <template match="p5" mode="text">
        <text>$5</text>
    </template>
    
    <template match="p6" mode="text">
        <text>$6</text>
    </template>
    
    <template match="p7" mode="text">
        <text>$7</text>
    </template>
    
    <template match="br" mode="text">
        <text>\n</text>
    </template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

    <template name="replace-substring">
        <param name="value" />
        <param name="from" />
        <param name="to" />
        
        <choose>
            <when test="contains($value,$from)">
                <value-of select="substring-before($value,$from)" />
                <value-of select="$to" />
                
                <call-template name="replace-substring">
                    <with-param name="value" select="substring-after($value,$from)" />
                    <with-param name="from"  select="$from" />
                    <with-param name="to"    select="$to" />
                </call-template>
            </when>
            <otherwise>
                <value-of select="$value" />
            </otherwise>
        </choose>
    </template>
    
    <!--~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~-->

</stylesheet>
