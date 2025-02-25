# Sugar-Glider-CPP
 An implementation of my language, Sugar Glider, in C++

Best way to run the project is to build from the repo.
Usage:
`sgr run file`
`sgr` gives a help menu

An example program showing off some of SugarGlider's capabilities

```
// This is a single line comment
/* And this is a multiline comment */
var k = "a";
k = k + "b";
Console.println(k);

function do_some_work(num) {
    return Math.sin(num) * Math.cos(num) / Math.sqrt(num) + Math.pow(num, num);
}
var start = clock(); // get the current time in nanoseconds
var iter = 0;
while (iter < 1_000_000) {
    do_some_work(iter);
    iter = iter + 1;
}
Console.print(Console.fg.red + "That took ");
Console.print((clock() - start) / 1_000_000_000);
Console.println(" seconds." + Console.reset);
```