## Events im JobScheduler

##### Schnappschuss und EventId

Die Webservices des JobScheduler stellen angeforderte Information über Objekte als Schnappschuss bereit.
Das bedeutet, dass die Information mit einer EventId, einer aufsteigenden Nummer, gestempelt ist.
Zum Beispiel werden die `OrderOverview` einer Menge von Aufträgen als Schnappschuss geliefert.

    GET https://.../jobscheduler/master/api/order/aFolder/?return=OrderOverview

Antwort:

```JSON
{
  "orders": [ { "TYPE": "OrderOverview", ... }, ... ],
  "eventId": 1478257655827000
}
```

##### Event

Anschließend können Änderungen an diesen Objekten über Events beobachtet werden.
Jedes Event wird ebenfalls als Schnappschuss, also mit einer EventId gestempelt, geliefert.

Die Events liegen in einem Event-Strom, geordnet nach EventId.
Der Scheduler bewahrt die jüngste Vergangenheit des Event-Stroms auf.
Damit lassen sich nicht nur neue, sondern auch vor kurzer Zeit aufgetretene Events anfordern.

Ein Event(-Schnappschuss) beschreibt eine Änderung an einem Objekt und wird geliefert mit den Feldern
* `key`, der Schlüssel des Objekts (zum Beispiel der Pfad) und
* `eventId` zur Identifikation des Events.

### <span id="Beispiel">Beispiel</span>

Zum Verständnis ein Beispiel mit Webservice-Aufrufen in JSON.

#### Lesen eines Schnappschusses

Zunächst lesen wir einen einen Schnappschuss der zu verfolgenden Objekte.
Das Beispiel sind das Aufträge (als `OrderOverview`) im Ordner /aFolder und darunter.
Die Aufträge lesen wir mit

    POST https://.../jobscheduler/master/api/order?return=OrderOverview
    {
      "path": "/aFolder/"
    }

Antwort:
```JSON
{
  "orders": [
     {
       "TYPE": "OrderOverview",
        ...
     },
     ...
  ],
  "eventId": 1478257655827000
}
```

Die Antwort kommt als Schnappschuss, also mit dem Feld `eventId`,
das wir zum Lesen der folgenden Events brauchen.

#### Lesen der Events

Um nun über Änderungen an den gelesenen Aufträgen informiert zu werden,
fordern wir die Events nach dem Schnappschuss an und
geben hierfür die EventId des Schnappschusses mit dem Parameter `after` an.

Der Webservice-Pfad `/jobscheduler/master/api/order` kann neben den Aufträgen auch deren Events liefern.
Als Rückgabetyp geben wir `return=Event` an.

    POST https://.../jobscheduler/master/api/order?return=Event&after=1478257655827000&timeout=60s
    {
      "path": "/aFolder/"
    }

Nach der üblichen Regel der Webservices ist diese Anfrage mit HTTP POST äquivalent zu der mit HTTP GET:

    GET https://.../jobscheduler/master/api/order/aFolder/?return=Event&after=1478257655827000&timeout=60s

Die URI-Parameter im Einzelnen:
* `/aFolder/` (bei GET) bezeichnet alle Aufträge im und unterhalb des Ordners /aFolder.

    Bei HTTP POST gibt man (wie bei den anderen Rückgabetypen) den Ordnerpfad als JSON-Objekt `PathQuery` an.
    `PathQuery` kennt nur das Feld `path`.
* `return=Event` fordert Events an (und nicht etwa wie oben `OrderOverview`).
* `after=1478257655827000` fordert Events an, die auf der angegebenen EventId folgen,
    nämlich die des uns bereits bekannten Schnappschusses.
* `timeout=60s` ist die Frist oder Wartezeit, die wir auf ein neues Event warten wollen.
    Voreingestellt ist 0s, der Webservice antwortet also sofort.

    Formate:

    * Zahl, zum Beispiel `60`, meint Sekunden. Das kann mit einem anhängten "s" verdeutlicht werden.
    * Zahl + "s", zum Beispiel `60s`
    * ISO-8601 Duration
    <p/>

    > v1.11.0-SNAPSHOT:
    Wenn eine First > 0s angegeben ist, kann die Wartezeit länger als angegeben dauern.
    Der JobScheduler prüft auf Fristablauf immer dann,
    wenn irgendein Event eintrifft.
    Sollte der JobScheduler eine zeitlang nichts tun,
    dann beendet er auch nach Fristablauf die HTTP-Anfrage erst,
    wenn wieder irgendein Event eintrifft.
    Bei einem beschäftigten JobScheduler sollte das keine Rolle spielen.

* `limit=...` (hier nicht verwendet) limitiert die Anzahl der zu liefernden Events.
    Ohne Angabe ist der Umfang unlimitiert.

Die Antwort der vorgehenden Anfrage könnte so aussehen:
```JSON
    {
      "TYPE": "NonEmpty",
      "eventId": 1478257702567000,
      "eventSnapshots": [
        {
          "eventId": 1478257656606001,
          "key": "/aFolder/test-ax,117",
          "TYPE": "OrderStepEnded",
          "nodeTransition": {
            "TYPE": "Success"
          }
        },
        {
          "eventId": 1478257656606004,
          "key": "/aFolder/test-ax,117",
          "TYPE": "OrderNodeChanged",
          "fromNodeId": "A",
          "nodeId": "BB"
        },
        {
          "eventId": 1478257656624001,
          "key": "/aFolder/test-ax,117",
          "TYPE": "OrderStepStarted",
          "nodeId": "BB",
          "taskId": "9"
        },
        {
          "eventId": 1478257657023000,
          "key": "/aFolder/test-ax,114",
          "TYPE": "OrderStepEnded",
          "nodeTransition": {
            "TYPE": "Success"
          }
        },
        ...
      ]
    }
```

(Die Antwort kommt wie üblich als Schnappschuss einer Event-Folge
mit dem Feld `eventId` -
diese EventId ist nur der Zeitstempel der Antwort, ohne weitere Bedeutung.)

Event-Folgen
([EventSeq](https://github.com/sos-berlin/scheduler-engine/blob/master/engine-data/src/main/scala/com/sos/scheduler/engine/data/event/EventSeq.scala),
[JSON](https://github.com/sos-berlin/scheduler-engine/blob/master/engine-data/src/test/scala/com/sos/scheduler/engine/data/event/EventSeqTest.scala))
kommen in den drei Varianten `NonEmpty`, `Empty` und `Torn` vor.
Hier haben wir `NonEmpty`.

#### EventSeq.NonEmpty

`NonEmpty` hat nur das eine Feld
* `eventSnapshots` ist ein nicht-leeres Array von Event-Snapschüssen, nach EventId geordnet.

Die Antwort enthält also die inzwischen eingetroffenen Events.
Nachdem wir sie verarbeitet haben, fordern wir die nächsten Events an.
Dazu werden wir im Parameter `after` die EventId des letzten verarbeiteten Events angeben.

#### EventSeq.Empty
`Empty` hat nur das eine Feld
* `lastEventId`

Die Event-Folge ist `Empty`, wenn in der Frist (Parameter `timeout`) kein Event eingetroffen st.

In diesem Fall wiederholen wir die Anfrage und geben dabei die EventId in `lastEventId` als Parameter `after` an.

#### EventSeq.Torn

Der Event-Strom ist gerissen.
Die angeforderte EventId ist zu alt,
der JobScheduler hat sie (wegen Platzmangels) vergessen.
Der Scheduler bewahrt eine begrenzte Anzahl Events auf
(10000 in v1.11.0-SNAPSHOT).

In diesem Fall beginnen wir <a href="#Beispiel">von vorn</a>,
lesen also den Zustand (Schnappschuss) erneut,
um wieder im Event-Strom aufsetzen zu können.

### Events des JobScheduler
Der JobScheduler kennt eine Reihe von Events.
Die Events sind in einer Hierarchie von Typen angeordnet, mit dem Typ `Event` der Spitze.
Ein `Event` kann ein `FileBasedEvent` oder `OrderEvent` sein
(oder `TaskEvent` und weitere, nicht über Webservices bereitgestellte Typen).

Die Events sollten sich selbst erklären.

* [SchedulerEvent](https://github.com/sos-berlin/scheduler-engine/blob/master/engine-data/src/main/scala/com/sos/scheduler/engine/data/scheduler/SchedulerEvent.scala)
([JSON](https://github.com/sos-berlin/scheduler-engine/blob/master/engine-data/src/test/scala/com/sos/scheduler/engine/data/scheduler/SchedulerEventTest.scala))
    * SchedulerStateChanged(SchedulerState)
    * SchedulerClosed

* [FileBased](https://github.com/sos-berlin/scheduler-engine/blob/master/engine-data/src/main/scala/com/sos/scheduler/engine/data/filebased/FileBasedEvent.scala)
([JSON](https://github.com/sos-berlin/scheduler-engine/blob/master/engine-data/src/test/scala/com/sos/scheduler/engine/data/filebased/FileBasedEventTest.scala))
    * FileBasedAdded
    * FileBasedRemoved
    * FileBasedReplaced
    * FileBasedActivated

* [OrderEvent](https://github.com/sos-berlin/scheduler-engine/blob/master/engine-data/src/main/scala/com/sos/scheduler/engine/data/order/OrderEvent.scala)
([JSON](https://github.com/sos-berlin/scheduler-engine/blob/master/engine-data/src/test/scala/com/sos/scheduler/engine/data/order/OrderEventTest.scala))
    * OrderStarted(NodeId, TaskId)
    * OrderFinished(NodeId)
    * OrderStepStarted
    * OrderStepEnded(NodeTransisition)
    * OrderSetBack(NodeId)
    * OrderNodeChanged(NodeId, from: NodeId)
    * OrderSuspended
    * OrderResumed

* [JobChainEvent](https://github.com/sos-berlin/scheduler-engine/blob/master/engine-data/src/main/scala/com/sos/scheduler/engine/data/order/JobChainEvent.scala)
([JSON](https://github.com/sos-berlin/scheduler-engine/blob/master/engine-data/src/test/scala/com/sos/scheduler/engine/data/order/JobChainEventTest.scala))
    * JobChainStateChanged(JobChainState)
    * JobChainNodeActionChanged(NodeId, JobChainNodeAction)

* [TaskEvent](https://github.com/sos-berlin/scheduler-engine/blob/master/engine-data/src/main/scala/com/sos/scheduler/engine/data/job/TaskEvent.scala)
([JSON](https://github.com/sos-berlin/scheduler-engine/blob/master/engine-data/src/test/scala/com/sos/scheduler/engine/data/job/TaskEventTest.scala))
    * TaskStarted
    * TaskEnded(ReturnCode)
    * TaskClosed

Einige Events haben Parameter.
Zum Beispiel kommt `OrderStepStarted` mit NodeId und TaskId (Felder `nodeId` und `taskId`).

### Event OrderStatisticsChanged

Das Event
[OrderStatisticsChanged](https://github.com/sos-berlin/scheduler-engine/blob/master/engine-data/src/main/scala/com/sos/scheduler/engine/data/order/OrderStatisticsChanged.scala)
weicht von anderen ab, denn es hat keinen Schlüssel.
Man fordert es eigens mit `return=OrderStatisticsChanged` an.
Der Webservice liefert bei der nächsten Änderung der
[OrderStatistics](https://github.com/sos-berlin/scheduler-engine/blob/master/engine-data/src/main/scala/com/sos/scheduler/engine/data/order/OrderStatistics.scala)
([JSON](https://github.com/sos-berlin/scheduler-engine/blob/master/engine-data/src/test/scala/com/sos/scheduler/engine/data/order/OrderStatisticsTest.scala))
genau ein Event.
Der JobScheduler berechnet das Event spontan. Es liegt nicht im Event-Strom.

### Details zur EventId

Die EventId ist eine streng monoton steigende natürliche Zahl.
Sie hat 53 Bits und passt damit in ein Long oder in eine JavaScript Number.
Ein späteres Event hat eine höherere EventId als ein früheres Event.
Die EventId bezeichnet die Position des Events im Event-Strom.

Die EventId codiert außerdem den Zeitstempel als Anzahl der 1000 Millisekunden seit 1970-01-01 00:00 UTC.
Bei mehr als 1000 Events/ms verschiebt sich der Zeitstempel, um die strenge Monotonie zu bewahren.
Das wird in der Praxis nicht vorkommen. So schnell ist der JobScheduler nicht.

```Scala
    def toInstant(eventId: EventId) = java.time.Instant.ofEpochMilli(eventId / 1000)
```
