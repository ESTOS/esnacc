export interface Event {
    timestamp: number,
    direction: "OUT" | "IN",
    payload: any
}

export interface Dictionary<T, K> {
    [key: T]: K
}