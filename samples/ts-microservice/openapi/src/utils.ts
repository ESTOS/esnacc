let counter = 0;
export function getInvokeId() {
    if (counter == 99998)
        counter = 0
    counter++
    return counter
}