export function reverseStr(str: string) {
    let reversed = '';
    // Loop backward and append characters
    for (let i = str.length - 1; i >= 0; i--) {
        reversed += str.charAt(i); // Or str[i]
    }
    return reversed;
}