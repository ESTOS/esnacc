//@ts-ignore
import { execute, buildRequest, baseUrl } from "swagger-client/es/execute";
import { eventWs, invokeRest, invokeWs } from "./client";


async function internalUserExecute(system: any, args: any) {
    const urlBase: string = baseUrl(args);
    if (args.spec.paths[args.pathName][args.method].operationId == undefined)
        throw new Error("OperationId needs to be specified in OpenApi otherwise WS cant be used.")
    const operationID = parseInt(args.spec.paths[args.pathName][args.method].operationId)

    let test;
    if (urlBase.startsWith("ws") || urlBase.startsWith("wss")) {

        try {
            test = await execute({
                ...args,
                userFetch: async () => {
                    let header = new Headers();
                    let data: { status: number, data: any };

                    if (args.spec.paths[args.pathName][args.method].responses["200"] && args.spec.paths[args.pathName][args.method].responses["500"]) {
                        // This invoke has a result
                        data = await invokeWs(operationID, urlBase, JSON.parse(args.requestBody))
                    } else {
                        // This is an event
                        eventWs(operationID, urlBase, JSON.parse(args.requestBody))
                        data = { status: 200, data: "Event has been emitted!" }
                    }

                    header.append("content-type", "application/json");
                    let resp: Response = {
                        headers: header,
                        ok: true,
                        redirected: false,
                        status: data.status,
                        statusText: "OK",
                        type: "basic",
                        url: "",
                        clone: function (): Response {
                            throw new Error("Function not implemented.");
                        },
                        body: null,
                        bodyUsed: false,
                        arrayBuffer: async function (): Promise<ArrayBuffer> {
                            throw new Error("Function not implemented.");
                        },
                        blob: async function (): Promise<Blob> {
                            throw new Error("Function not implemented.");
                        },
                        formData: async function (): Promise<FormData> {
                            throw new Error("Function not implemented.");
                        },
                        json: async function (): Promise<any> {
                            return data.data;
                        },
                        text: async function (): Promise<string> {
                            return JSON.stringify(data.data);
                        },
                    };

                    return resp;
                },
            });
        } catch (error) {
            console.error(error);
            throw error;
        }
        return test;
    } else {
        // Not needed 
        /** 
        if (args.requestContentType == "application/json")
            return await execute({
                ...args,
                userFetch: async () => {
                    return await invokeRest(operationID, urlBase, JSON.parse(args.requestBody));
                },
            });
        else 
        */
        return execute(args);
    }
}

export function userExecute(system: any) {
    return (args: any) => internalUserExecute(system, args)
}