import "@estos/esnacc-openapi-sdk/dist/esnacc-openapi-sdk.css";
import SDK from "@estos/esnacc-openapi-sdk";

SDK({
    domId: "ui", schemas: [
        { schemaUrl: "/schema/ENetUC_Common.json", label: "ENetUC_Common" },
        {
            schemaUrl: "/schema/ENetUC_Settings_Manager.json", label: "ENetUC_Settings_Manager", "injectSpec": {
                servers: [
                    {
                        "url": "ws://localhost:3020/ws"
                    },
                    {
                        "url": "http://localhost:3020/rest"
                    }
                ]
            },
        },
        {
            schemaUrl: "/schema/ENetUC_Event_Manager.json", label: "ENetUC_Event_Manager", "injectSpec": {
                servers: [
                    {
                        "url": "ws://localhost:3020/ws"
                    },
                    {
                        "url": "http://localhost:3020/rest"
                    }
                ]
            },
        },
    ]
})