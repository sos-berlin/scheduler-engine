# NewWebServicePlugin

This NewWebServicePlugin will replace the old C++ web server and JettyPlugin.

Usage in configuration file `scheduler.xml`
```xml
<plugins>
  <plugin java_class="com.sos.scheduler.engine.plugins.newwebservice.NewWebServicePlugin"/>
</plugins>
```
---
<span style="letter-spacing: 1pt; color: red">INOFFICIAL, INTERNALLY USED FEATURES DRAFT</span>

## Web service paths

### /api - Overview
Example

    GET https://.../jobscheduler/master/api

### /api/order - Orders
Example

    GET https://../jobscheduler/master/api/order/

Note that for HTTP POST, the URI path is `/jobscheduler/master/api/order`, without trailing slash.

A <a href="#PathQuery">PathQuery</a> must be given. 
For HTTP GET, you may simply append the complete Order path to the URI path.

Parameters are
* `return`
    * `return=OrderOverview`, returning an array of `OrderOverview`. 
    * `return=OrderDetailed`, returning an array of `OrderDetailed` 
    * `return=OrdersComplemented/OrderOverview` 
    * `return=OrdersComplemented/OrderDetailed` 
    * `return=OrderTreeComplemented/OrderOverview` 
    * `return=OrderTreeComplemented/OrderDetailed` 

### /api/processClass - Process classes
Example

    GET https://.../jobscheduler/master/api/processClass/


### Events

[Siehe hier](doc/events.md).


### PathQuery
<span id="PathQuery"/>

A `PathQuery` can be given
* with HTTP GET in the URI path, like 
    ```
    GET /jobscheduler/master/api/order/aFolder/aJobChain,anOrderId
    ```     
* with HTTP POST and JSON in the field `path`, like
    ```JSON
    {
        "path": "/aFolder/aJobChain,anOrderId"
    }
    ```

A `PathQuery` has three forms.
* the complete path of an object, for example for an Order: `/aFolder/aJobChain,anOrderId`
* the Path of the containing folder, appended with 
  * `/`, a slash, like `/aFolder/`, 
    designating all objects in and below this folder.
  * `/*`, a slash and star, like `/aFolder/*`, 
    designating the objects in this folder only, without subfolders.
