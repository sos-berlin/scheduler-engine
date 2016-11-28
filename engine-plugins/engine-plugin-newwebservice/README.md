# NewWebServicePlugin

This NewWebServicePlugin will replace the old C++ web server and JettyPlugin.

Usage in configuration file `scheduler.xml`
```xml
<config http_port="..." https_port="...">
  <plugins>
    <plugin java_class="com.sos.scheduler.engine.plugins.newwebservice.NewWebServicePlugin"/>
  </plugins>
  ...
</config>
```
The attributes `http_port` and `https_port` accepts a port number, 
optionally prefixed by the IP number of an interface and a colon, like `http_port="127.0.0.1:4444"`.

---
<div style="letter-spacing: 1pt; color: red; margin-bottom: 3em">
    INOFFICIAL, INTERNALLY USED FEATURES
    <br/>
    This documentation is a work in progress
</div>

## Web service paths

### /api - Overview
Example

    GET https://.../jobscheduler/master/api

## General URI syntax for querying JobScheduler objects (FileBased)

    GET https://.../jobscheduler/master/api/TYPE/PATH

`TYPE` is an object type (`folder`, `lock`, `job`, `jobChain`, `order`, `processClass`, `schedule`) or `fileBased`.
 
`/PATH` is the object path (note starting slash always denoting the root folder), used for HTTP GET. 
An order is designated by the path of the job chain, comma, and the order ID.
A path may denote a 
* a single object, like `/myFolder/myJobChain/myOrder` (not for web service api/fileBased).
* a multiple objects, returned as an array. 
    * A terminating slash (/) denotes a folder with all its descendants.
    * A terminating slash and start (/*) denotes a folders contents.

Parameters for all types are
* `return`
    * `return=FileBasedOverview` (current default) 
    * `return=FileBasedDetailed` 
 
### /api/fileBased - FileBaseds
Example

    GET https://../jobscheduler/master/api/fileBaseds/myFolder/?return=FileBasedOverview

Only folders may be given as path. The path must be terminated with slash (/) or slash and start (/*).

Parameters are
* `return`
    * `return=FileBasedOverview` (current default) 
    * `return=FileBasedDetailed` 


### /api/folder - Folders
Example

    GET https://../jobscheduler/master/api/folder/myFolder?return=FileBasedOverview

Parameters are
* `return`
    * `return=FileBasedOverview` (current default) 
    * `return=FileBasedDetailed` 


### /api/job - Jobs
Example

    GET https://../jobscheduler/master/api/job/myFolder/myJob?return=FileBasedOverview?return=JobDetailed

Parameters are
* `return`
    * `return=FileBasedOverview` 
    * `return=FileBasedDetailed` 
    * `return=JobOverview` (current default) 
    * `return=JobDetailed` 

### /api/lock - Locks
Example

    GET https://../jobscheduler/master/api/lock/myLock?return=FileBasedDetailed

Parameters are
* `return`
    * `return=FileBasedOverview` (current default) 
    * `return=FileBasedDetailed` 


### /api/jobChain - Job chains
Example

    GET https://../jobscheduler/master/api/jobChain/myJobChain?return=FileBasedDetailed

Parameters are
* `return`
    * `return=FileBasedOverview` (current default) 
    * `return=FileBasedDetailed` 


### /api/order - Orders
Example

    GET https://../jobscheduler/master/api/order/?return=OrderTreeComplemented/OrderOverview

Note that for HTTP POST, the URI path is `/jobscheduler/master/api/order`, without trailing slash.

A <a href="#PathQuery">PathQuery</a> must be given. 
For HTTP GET, you may simply append the complete Order path to the URI path.

Parameters are
* `return`
    * `return=OrderOverview` 
    * `return=OrderDetailed` 
    * `return=OrdersComplemented/OrderOverview` 
    * `return=OrdersComplemented/OrderDetailed` 
    * `return=OrderTreeComplemented/OrderOverview` 
    * `return=OrderTreeComplemented/OrderDetailed`

### /api/processClass - Process classes
Example

    GET https://.../jobscheduler/master/api/processClass/?return=ProcessClassOverview

Parameters are
* `return`
    * `return=ProcessClassOverview` 
    * `return=ProcessClassDetailed` 

### /api/schedule - Schedules
Example

    GET https://../jobscheduler/master/api/schedule/myFolder/mySchedule?return=FileBasedDetailed

Parameters are
* `return`
    * `return=FileBasedOverview` (current default) 
    * `return=FileBasedDetailed` 


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
