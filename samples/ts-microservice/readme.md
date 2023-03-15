# Typescript based microservice sample
This sample is based on typescript and shows how a browser based client can communicate with a node based backend.
Client and server can communcate through REST like stateless calls or a websocket connection that allows eventing from the server to the client.

# interface
The interface folder contains the asn1 interface description between server and client
It consists of two example modules with dummy functions and events.

# client
The browser based client show a simple vanilla html ui to call methods on the server side.
Depending on the methog beeing called the server just answers or will create delayed events to show how the websocket connection is working.

# server
The server is node based 

* Running the sample
** Have at least node 18 and VSCode installed.
** Server:
*** Navigate into the server folder and call npm ci to install the dependencies
*** Adopt .env.sample and save it as .env
**** The listen port of the server is required on the client side 

** Client:
*** Navigate into the client folder and call npm ci to install the dependencies
*** Adopt .env.sample and save it as .env
*** Open the microservicetemplate_client.code-workspace in VSCode
*** Call npm start in the console of VSCode
*** In Run and Debug select the appropriate client and http:// or https:// depending on your configuration

