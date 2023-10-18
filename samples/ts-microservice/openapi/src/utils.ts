let counter = 0;
export const eventOperationId = 99999
export function getInvokeId() {
    if (counter == 99998)
        counter = 0
    counter++
    return counter
}