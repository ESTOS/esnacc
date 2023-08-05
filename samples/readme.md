# Typescript based client/server sample
This sample is based on typescript and shows how a browser based client can communicate with a node based backend.
Client and server can communicate through REST like stateless calls or a websocket connection that allows eventing from the server to the client.

## The interface directory
The interface folder contains the asn1 interface description for the sample ts-server and ts-client.

It consists of two example modules with dummy functions and events.

## Generating the stubs for client and server

Before you can run the server or client you have to generate the stub code
for both of them.

Running the ```create_stubs.bat``` script will use the esnacc compiler to transform the ASN.1 description form the interface directory to TypeScript files in both projects.

You will find the generated code in the ```src/stub``` directory in each project.

## The ts-server directory
This folder contains the code for an TypeScript server providing an API.

The server is a simple, node based server using the ASN.1 sample interface.

### Running the sample server

__Prerequisits__

This should be installed:
* VSCode
* Node.JS at least Version 18

__Initialization__

```
npm install
```

__Configuration__

In the .env File you find some variables for configuration. Here you can switch to a TLS version too.

__Start the server__

To start the server, just run
```
npm run start
```
In the default case, this should start the program and it will open port 3020 for accepting client connections.

## ts-client
The browser based client show a simple vanilla html ui to call methods on the server side.

### Running the sample client

__Prerequisits__

This should be installed:
* VSCode
* Node.JS at least Version 18

__Initialization__

Then switch into the ```ts-client``` directory and run 

```
npm install
```

__Configuration__

In the .env File you find some variables for configuration. Here you can switch to a TLS version too.

__Start the client__

To start the server, just run
```
npm run start-http
```
This starts the webpack http-server to host the client code. Start a web browser with the default url http://localhost:8080 . This will show you the client ui and you can check if all is running well by clicking the "Get settings" button.

## Debugging the samples

Both the server and the client sample comes with an own VS Code workspace.
So it is easy to debug one or both programs simultaneously.

Don't forget, that the client must have started the webpack server first before you can debug the client in a browser environment.



