package com.sos.scheduler.engine.cplusplus.generator.main

import org.hamcrest.MatcherAssert._
import org.hamcrest.Matchers._
import org.junit.Test


class MavenPropertiesTest {
    val mavenProperties = new MavenProperties(getClass)

    @Test def testGroupId {
        assertThat(mavenProperties.groupId, equalTo("com.sos.scheduler.engine"))
    }

    @Test def testArtefactId {
        assertThat(mavenProperties.artifactId, equalTo("com.sos.scheduler.engine.cplusplus.generator"))
    }
    
    @Test def testVersion {
        //assertThat(Parameters.pom.version.length, greaterThanOrEqualTo(1))
        assert(mavenProperties.version.length >= 1)
    }
}
