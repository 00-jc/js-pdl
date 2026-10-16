// expresiones con operadores aritméticos, relacionales y lógicos
let a int;
let b int;
let r boolean;

a = 10;
b = 3;
a = a + b * 2 - (a / b) % 4;
r = a == b;
r = a != b;
r = a < b || a > b;
r = a <= b && a >= b;
r = !r;
r = !(a < b) && (b > 0 || a == 0);
