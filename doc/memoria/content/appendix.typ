#import "@preview/magic-isprs:0.1.0": isprs-heading
#show: isprs-heading

#let impl = sys.inputs.at("impl", default: "true") == "true"

#if (impl) [Se implementó un tokenizador _SIMD_ que sólo tuviese un camino de flujo de control,
no obstante, por la complejidad que representaba formalizarlo como un _AFD_,
se descartó pese a ser ordenes de magnitud más eficiente.

Las referencias utilizadas para implementarlo han quedado reflejadas igualmente.
#colbreak()
]


#let caso(titulo, codigo, ok: true, salida: none) = {
  block(breakable: false, width: 100%, above: 1em, below: 1em, {
    set block(spacing: 0pt)
    set par(justify: false)
    show raw.where(block: true): set block(above: 0pt, below: 0pt, spacing: 0pt)
    block(width: 100%, inset: (y: 3pt), text(size: 0.85em, emph(titulo)))
    block(
      width: 100%,
      fill: luma(245),
      stroke: (top: 0.5pt + luma(120), bottom: if salida == none { 0.5pt + luma(120) } else { 0pt }),
      inset: (x: 6pt, y: 5pt),
      {
        set text(size: 0.85em)
        codigo
      },
    )
    if salida != none {
      block(
        width: 100%,
        stroke: (top: 0.5pt + luma(180), bottom: 0.5pt + luma(120)),
        inset: (x: 6pt, y: 5pt),
        {
          set text(size: 0.8em)
          [_Salida de_ #raw("js-pdl " + salida.at(0) + ":")]
          v(0.3em)
          raw(salida.at(1), block: true)
        },
      )
    }
  })
}

= Tests de aceptación

#caso("Aceptación 1", ```js
function suma int (x int, y int)
{
	return x + y;
}
let r int;
r = suma(1, 2);
saluda("mundo");
nada();
```)

#caso("Aceptación 2", ```js
function factorial int (n int)
{
	let acc int;
	acc = 1;
	if (n <= 1) return 1;
	do
	{
		acc = acc * n;
		n = n - 1;
	} while (n > 1);
	return acc;
}
total = factorial(5);
if (en_rango(total, 100, 200)) informe("factorial de 5:", total);
total += factorial(3);
informe("total:", total);
```)

#caso("Aceptación 3", ```js
let a int  ;
let  b  int    ;
let bbb boolean ;
a = 3;
b=a					    		 	 	 	 	 	 	 	 	 	 	 	 	 	 	 																;
let rrr float;
let ccc float;
let  aaaa int;
let ssss string;
let bbbb boolean ;
```)

= Tests de rechazo

#caso("Rechazo 1", ```js
let b boolean;
b = true & false;
```, ok: false, salida: ("rechazo1.js", "error in rechazo1.js:2:10: && and || have to appear as logical operators, no bit arithmetic is supported"))

#caso("Rechazo 2", ```js
let a2345678901234567890123456789012345678901234567890123456789012345 int;
```, ok: false, salida: ("rechazo2.js", "error in rechazo2.js:1:5: identifier too big"))

#caso("Rechazo 3", ```js
let s string;
s = "esto nunca se cierra;
```, ok: false, salida: ("rechazo3.js", "error in rechazo3.js:2:5: unterminated string"))

