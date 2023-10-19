export const WEBSOCKET_ADD_WEBSOCKET = (state: any, { payload }: any) => {
    return state.setIn([payload.server, "ws"], payload.ws);
}

export const WEBSOCKET_ADD_EVENT = (state: any, { payload }: any) => {
    let check = state.getIn([payload.server, payload.operationID]);
    if (check)
        return state.updateIn([payload.server, payload.operationID], (arr: any) => {

            return arr.concat(payload.event)
        });
    else
        return state.setIn([payload.server, payload.operationID], [payload.event]);
}