/*! \change JS-481 Testcode */
/**
* Für Testzwecke ist es oft notwendig Test-Codezeilen zu aktivieren oder bestehende Codezeilen zu deaktivieren.
* Einfaches ein / -auskommentieren hat den Nachteil, dass es fehleranfällig ist. 
* Außerdem ist Testcode unabhängig vom Debug-Modus.
* Deshalb ist folgendes Vorgehen gewählt worden:
* Die Präprozessordirektive TESTCODE_ACTIVATED aktiviert/deaktiviert allen Jira spezifischen Testcode.   
* Je Jira-Eintrag kann eine eigene Präprozessordirektive definiert werden, um dessen Jira spezifischen Testcode zu aktivieren.
* Dieses Header File ist in den entsprechenden Sourcefiles einzubinden, wo Testcode benötigt wird.
* Ein Beispiel wäre supervisor_client.cxx.
*/

// #define TESTCODE_ACTIVATED

#ifdef TESTCODE_ACTIVATED
	#define TESTCODE_ACTIVATED_JS481
#endif

