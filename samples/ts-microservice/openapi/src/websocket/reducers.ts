/**
export default {
    "WEBSOCKET_ADD_WEBSOCKET": (state: any, { payload }: any) => {
        return state.setIn([payload.server, "ws"], payload.ws);
    },
    "WEBSOCKET_ADD_EVENT": (state: any, { payload }: any) => {
        let arr = state.getIn([payload.server, payload.operationID]);
        if (arr)
            return state.setIn([payload.server, payload.operationID], arr.push(payload.event));
        else
            return state.setIn([payload.server, payload.operationID], [payload.event]);
    }
}
*/

/** */
export const WEBSOCKET_ADD_WEBSOCKET = (state: any, { payload }: any) => {
    return state.setIn([payload.server, "ws"], payload.ws);
}

export const WEBSOCKET_ADD_EVENT = (state: any, { payload }: any) => {
    console.log(payload)

    let check = state.getIn([payload.server, payload.operationID]);
    if (check)
        return state.updateIn([payload.server, payload.operationID], (arr: any) => {

            return arr.concat(payload.event)
        });
    else
        return state.setIn([payload.server, payload.operationID], [payload.event]);
}