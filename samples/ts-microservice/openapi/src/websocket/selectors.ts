export const getWebsocket = (state: any, server: any) => {
    return state.getIn([server, "ws"])
}

export const getEvents = (state: any, server: string, operationID: number) => {
    return state.getIn([server, operationID]) ?? []
}