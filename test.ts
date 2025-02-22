function log2ff(l) {
    var m = 0;
    while (l >= 1) {
        l = l * 0.5;
        m = m + 1;
    }
    return m;
}

let start = performance.now();
let ind = 1;
while (ind < 1_000_000_0) {
    var m = log2ff(ind);
    ind = ind + 1;
}
console.log((performance.now() - start) / 1_000);