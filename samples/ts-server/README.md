# estos NodeJS server template

[TOC]

# Introduction

estos NodeJS server template is a typescript template to ease implementing NodeJS servers that offer:
- full typescript support
- ready to use eslinting (preconfigured ruleset) with typescript support
- express based web service interface for clients using http-post (function calling only) or websocket (event supporting) interface
- An asn1-based, generated server and client interface/stub (supporting websocket and post requests)

# How to start
- Call ```npm install``` in the root directory of this project and the sampleclient directory
- Open two visual studio code instances:
  - ```code sampleclient\sampleclient.code-workspace```
  - ```code microservicetemplate.code-workspace```
- Create [default configuration](#"default-configuration")
- Press ```F5```

# Global managers
Similar to *ProcallServer* there are several Singleton manager classes with the prefix *the* ready to use in */src/lib/globals.ts*.

E.g.:
```typescript
import {
	theConfig, // Access project specific configurations
	theClientConnectionManager, // Managing client connections
	theServers, // Server handling asn routing
	theLogger, // Logger, supports Console, File and GreyLog
} from 'globals';
```


# Project structure
Layout of the project (where to find what).
Starting point is the /src directory which contains all the typescript files

```bash
|-- app.ts # Main entry poin
|-- express
|   +-- expressInit.ts # Logic that initializes the express framework with the files under routes. To add or remove routes you just add or remove a file in routes.
|   +-- expressRouter.ts # Logic that implement the express router (static Helper ERouter that implements the code to dynamic initialized the routes. Contains the EModule interface that every route has to fullfill for beeing loaded by the expressInit)
|--	+-- routes # Contains all the service entry points as .ts files. These files are automatically parsed and loaded from the expressInit.ts.
|		+-- echoSample.ts # Adds a express route under /echo. This sample echoes what you did send to the server in the post body. If no body was specified it answers "Nothing to echo"
|		+-- healthCheck.ts # Adds a express route under /healthcheck. This sample answers with the uccommon EHealthCheck data
|		+-- restSample.ts # Adds a express route under /rest. This sample implements the post interface to call functions in the asn1 generated stub. This sample also uses {theServer}
|		+-- webSocketSample.ts # Adds a express route under /ws. This sample implements the websocket interface to call functions in the asn1 generated stub. As a websocket connection has a state this sample uses other singletons in the server. Uses {theClientConnectionManager} and {theServer}.
|-- lib
|	+-- clientConnection.ts # The websocket connected client is represented by a clientConnection. This connection has some base functions like websocket pingpong, keepalive. It implements the send and receive functions which are user or called by the asn1 stub.
|	+-- healthcheck.ts # Writes statistics about the system load into the logfile every 60 seconds
|	+-- sampleHandler.ts # The sample handler is the endpoint in the server that receveices invokes by the client. It is initialized in theServer constructor and held by the ENetUC_SampleROSE Invoke Handler
|-- singletons
|	+-- globals.ts # All singletons in the project must be created and exported through this file. As typescript and javascript do not have a concept about declaration and implementation you will very very likely otherwise get circular dependencies when one singleton uses another. Just add the singleton here, and export it like the others in the global.ts file.
|	+-- config.ts # The config of the project. In order to always use the same config approach please add your config to this singleton. The init method contains businesslogic that validates that all the required config properties are provided in the environment. If a property is missing the process will fail to start. Please do NOT use environment properties directly. In staging/production everything is provided directly through the environment. In development the properties are provided through a .env file which is located in the root folder of this project.
|	+-- clientConnectionManager.ts # The Client Connection Manager takes care about the connected clients. It holds the list of clients and provides the notify interface to receive events about connecting and disconnecting clients.
|	+-- server.ts # The server holds the asn1 stubs that the microservice is gonna implement. Each used stub is added as readonly member in the class. Within the constructor the methods this stub offers are automatically added to the list of callable functions. To call a function (event) towards a client the singleton which wants to send an event calls: {theServer.sample.Event_asnSomethingHappened(arguments, sessionID)};
|-- stub\* # The files in the stub folder are generated files written by the escnacc4.exe based on the files in the interface folder in the root directory of this project.
```

# Examples

## WebSocket
For the websocket connecting client the sample creates a clientConnection which is held by ```theClientConnectionManager```. This clientConnection has a sessionID which is used to identify a connected client and to be able to send events to this client. ```theClientConnectionManager``` provides a notify interface that is called whenever a client connects or disconnects.

Thus other singletons in ```theServer``` can be notified about new or dying clients.

## Sample client
The sample client contains a simple website to invoke calls towards the server. In order to get the client running type

	npm start

in the terminal in VSCode.

As soon as the package has been build by webpack
```(｢wdm｣: Compiled successfully.)``` you can launch a browser with F5.
(Firefox, Chrome and Edge have been configured as debug applications in VSCode)

MicroService Template:
Be sure to setup an .env file in the root directory according to ## Default configuration in this readme at the end of the file
In VSCode press F5 to launch the server process.

# SSL Key and Certificate
If you want to use SSL please provide a proper certificate as key.pem and cert.pem file.
You can, however, also just run the plain TCP server.

Hint for OpenSSL

	$ openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes

# Behind the curtains
* Initialise singletons in (globals.ts) within app.ts.
* Constructing and initializing the routes in */src/express/routes/*. Mounting the the asn1 based stubs. To handle The incoming invoke (Rest or Websocket) in the express routes restSample.ts or webSocketSample.ts.
* The webSocketSample creates a ClientConnection object for the connecting webSocket client. Receiving data is therefore not handled in the webSocketSample but in the ClientConnection.

Just place a breakpoing in *RestSample.restRequest*,  *ClientConnection.wsClientMessage();* and  *SampleHandler.OnInvoke_asnLetSomethingHappen();*.

You can then step through the code and see how the message is processed through the stub and in the end handled in *SampleHandler.OnInvoke_asnLetSomethingHappen()*.

The incoming invoke creates an appropriate answer right away and starts two timers for events and server side invokes if it is beeing called via a websocket connection.

The *IInvokeContext* in the handling method hand over the used websocket and sessionID if beeing called via websockets.

Adding new functions and events is pretty simple.
- Open the *interface\ENetUC_Sample.asn1* file and add methods and or events like the two already in the file.
- Call ```create_stubs.bat``` to create the stub with the esnacc4.exe.

The estos globals (https://git.estos.de/projects/WGL/repos/global/browse) repository must be available and checked out to
	X:\dev\global
in order to be able to compile using the esnacc4.exe

# Default configuration

Settings are automatically read from the environment using the npm dotenv package
Place a .env file in the root directory of this project and add properties as required.

```
LOGPATH=X:\dev\Conferencing\logs
LOGLEVEL=4
NODE_ENV=DEVELOPMENT
HTTPPORT=3010
HTTPSPORT=3011
CERTFILE=..\cert.pem
KEYFILE=..\key.pem
LISTEN_IP=127.0.0.1 (OPTIONAL)
SERVERNAME=localhost
```
