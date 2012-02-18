package com.sos.scheduler.engine.plugins.jetty.rest

import com.google.common.io.Closeables.closeQuietly
import com.sos.scheduler.engine.kernel.folder.{AbsolutePath, FolderSubsystem}
import com.sos.scheduler.engine.plugins.jetty.rest.WebServices.noCache
import java.io.{StringReader, StringWriter}
import javax.inject.Inject
import javax.ws.rs._
import javax.ws.rs.core._
import javax.xml.transform.TransformerFactory
import javax.xml.transform.stream.{StreamResult, StreamSource}
import scala.collection.JavaConversions._

@Path("folders")
class FoldersResource @Inject()(folderSubsystem: FolderSubsystem) {
  import FoldersResource._

  @GET
  @Produces(Array(MediaType.TEXT_HTML))
  def getHtml(@QueryParam("path") @DefaultValue("") pathString: String,
              @QueryParam("type") @DefaultValue("") typeName: String,
              @Context u: UriInfo) = {
    val result = transform(get(pathString, typeName, u))
    Response.ok(result).cacheControl(noCache).build()
  }

  @GET
  @Produces(Array(MediaType.TEXT_XML))
  def getXml(@QueryParam("path") @DefaultValue("") pathString: String,
             @QueryParam("type") @DefaultValue("") typeName: String,
             @Context u: UriInfo) = {
    Response.ok(get(pathString, typeName, u)).cacheControl(noCache).build()
  }

  private def get(pathString: String, typeName: String, u: UriInfo) = {
    //TODO Bei Fehler SCHEDULER-161 404 liefern.
    val path = AbsolutePath.of(pathString)
    def toXml(name: String) = {
      val p = AbsolutePath.of(path, name)
      val uri = u.getBaseUriBuilder.path(typeName).queryParam("path", p.toString).build()
      <name name={name} uri={uri.toString}/>
    }
    val contents = folderSubsystem.names(path, typeName) map toXml
    <names path={path.withTrailingSlash} type={typeName}>{contents}</names>
  }
}

object FoldersResource {
  private lazy val transformer = {
    val in = classOf[FoldersResource].getResourceAsStream("FoldersResource.xsl")
    try TransformerFactory.newInstance().newTransformer(new StreamSource(in))
    finally closeQuietly(in)
  }

  def transform(e: xml.Elem) = {
    val w = new StringWriter
    val x = e.toString()
    transformer.transform(new StreamSource(new StringReader(x)), new StreamResult(w));
    w.toString
  }
}
