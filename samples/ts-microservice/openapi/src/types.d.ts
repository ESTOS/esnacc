
export interface Dictionary<T, K> {
    [key: T]: K
}

export interface RoseMessage<T> {
    result?: {
        invokeID: number,
        result: {
            result: T,
            resultValue: 0
        }
    },
    invoke?: {
        operationID: number,
        invokeID: number,
        argument: T
    },
    reject?: {
        details: string,
        invokedID: {
            invokedID: number
        },
    }
}