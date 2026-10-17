#import "@preview/magic-isprs:0.1.0": isprs-heading
#import "@preview/fletcher:0.5.8": diagram, node, edge
#import "../automata-lexico.typ": automata-lexico, leyenda-automata
#import "../grafico-comparacion.typ": grafico-comparacion
#import "../grafico-ablacion.typ": grafico-ablacion
#import "../grafico-soa.typ": grafico-soa, leyenda-soa
#show: isprs-heading

#let impl = sys.inputs.at("impl", default: "true") == "true"

= Gramática

La gramática de _js-pdl_ para el grupo 49 se recoge en la @fig-gramatica. Está escrita para un
analizador descendente recursivo, por lo que no tiene recursividad por la izquierda y está
factorizada: cada no terminal elige su producción mirando un único _token_. Los terminales son
los _tokens_ que emite el analizador léxico; `id`, `cint`, `cfloat` y `cad` son, respectivamente,
los identificadores, las constantes enteras, las reales y las cadenas, y $lambda$ es la cadena vacía.

#let nt(x) = text(style: "italic", x)
#let prod(lhs, ..rhs) = (
  nt(lhs), $->$, rhs.pos().map(r => r).join([ #h(0.3em)|#h(0.3em) ]),
)
#[
#show figure: set block(breakable: true)
#figure(
  {
    set par(justify: false)
    set align(left)
    show raw: set text(weight: "bold")
    grid(
      columns: (auto, auto, 1fr),
      column-gutter: 0.5em,
      row-gutter: 0.55em,
      ..prod("P", [#nt("B") #nt("P")], [#nt("F") #nt("P")], $lambda$),
      ..prod("B", [`let` `id` #nt("T") `;`], [`if` `(` #nt("E") `)` #nt("S")],
        [`do` `{` #nt("C") `}` `while` `(` #nt("E") `)` `;`], nt("S")),
      ..prod("T", `int`, `float`, `boolean`, `string`),
      ..prod("S", [`id` #nt("S'")], [`echo` #nt("E") `;`], [`prompt` `id` `;`],
        [`return` #nt("X") `;`]),
      ..prod("S'", [`=` #nt("E") `;`], [`+=` #nt("E") `;`], [`(` #nt("L") `)` `;`]),
      ..prod("X", nt("E"), $lambda$),
      ..prod("L", [#nt("E") #nt("Q")], $lambda$),
      ..prod("Q", [`,` #nt("E") #nt("Q")], $lambda$),
      ..prod("F", [`function` `id` #nt("H") `(` #nt("A") `)` `{` #nt("C") `}`]),
      ..prod("H", nt("T"), `void`),
      ..prod("A", [`id` #nt("T") #nt("K")], `void`),
      ..prod("K", [`,` `id` #nt("T") #nt("K")], $lambda$),
      ..prod("C", [#nt("B") #nt("C")], $lambda$),
      ..prod("E", [#nt("R") #nt("E'")]),
      ..prod("E'", [`||` #nt("R") #nt("E'")], $lambda$),
      ..prod("R", [#nt("U") #nt("R'")]),
      ..prod("R'", [`&&` #nt("U") #nt("R'")], $lambda$),
      ..prod("U", [#nt("V") #nt("U'")]),
      ..prod("U'", [#nt("O") #nt("V")], $lambda$),
      ..prod("O", `==`, `!=`, `<`, `>`, `<=`, `>=`),
      ..prod("V", [#nt("W") #nt("V'")]),
      ..prod("V'", [`+` #nt("W") #nt("V'")], [`-` #nt("W") #nt("V'")], $lambda$),
      ..prod("W", [#nt("Z") #nt("W'")]),
      ..prod("W'", [`*` #nt("Z") #nt("W'")], [`/` #nt("Z") #nt("W'")],
        [`%` #nt("Z") #nt("W'")], $lambda$),
      ..prod("Z", [`!` #nt("Z")], [`-` #nt("Z")], [`(` #nt("E") `)`], [`id` #nt("Y")],
        `cint`, `cfloat`, `cad`, `true`, `false`),
      ..prod("Y", [`(` #nt("L") `)`], $lambda$),
    )
  },
  kind: table,
  caption: [Gramática LL(1) de _js-pdl_ (grupo 49). #nt("P") es el axioma.],
) <fig-gramatica>
]

El `if` no tiene `else` y controla una única sentencia simple (#nt("S")). La precedencia, de
menor a mayor, es `||`, `&&`, relacionales, aditivos, multiplicativos y unarios; los binarios
asocian por la izquierda, salvo los relacionales, que no se encadenan.

// El analizador léxico empieza siempre en la segunda página.
#pagebreak(weak: true)

= Analizador Léxico

Para el analizador léxico se ha apostado por un modelo "_on-demand_" (o "_lazy_") porque es el modelo que
se explica en las sesiones de teoría. Dicho analizador tiene como propósito principal discernir los inicios
y terminaciones del contenido semánticamente relevante de todo el input que se le proporciona.

#if impl [
Para modelar este analizador como un autómata finito determinista de la manera más equivalente posible (tanto
desde un punto de vista semántico como programático), modelamos las transiciones del autómata con una tabla
de "_computed gotos_", de tal forma el flujo de control del programa representa exactamente el paso de un estado
a otro. De esta manera el código fuente queda más legible a la vez que mejora la eficiencia del analizador notablemente,
por dos motivos en especial:

- La tabla de entrada es estática en _.bss_ (reduce el tamaño del ejecutable).

- El analizador no tiene que clasificar el estado de la tabla con un "_switch_" u otro constructo, la ejecución salta, modelando la transición exactamente.
] else [El analizador se modela como un autómata finito determinista (@fig-automata-lexico), cuyos estados
se recogen en la @tab-lex-def-estados.
]

#if impl [Dicha tabla es unidimensional de 256 entradas, una por _byte_ que podamos encontrar, y los estados a los que puede saltar se recogen en la @tab-lex-def-estados.]

// Va al pie de la página del autómata.
#let tabla-estados = [
#figure({
  set par(justify: false)
  table(
    columns: (auto, auto, 1fr),
    align: left,
    inset: (x: 5pt, y: 2.5pt),
    stroke: (x, y) => {
      let res = (bottom: 1pt, top: 0pt)
      if (y < 2) {
        res.top = 1pt
      }
      res
    },
    table.header([Estados], [Etiqueta en `lexer.c`], [Descripción]),
    [0], [`whitespace`], [Salta los caracteres blancos.],
    [1], [`success`], [Estado final: emite el _token_.],
    [2–3], [`identifier`], [Identificadores y palabras reservadas.],
    [4–5], [`numeric_constant`], [Constantes enteras.],
    [6–8], [`float_constant`], [Constantes reales (con parte decimal).],
    [9–11], [`string_literal`], [Cadenas entre comillas.],
    [12–14], [`possible_operator_followed_by_eq`], [Operadores que pueden ir seguidos de `=`.],
    [15–16], [`double_op_or_error`], [Operadores lógicos `&&` y `||`.],
    [17–19], [`div_or_comment`], [División o comentario de línea.],
    [20], [`single_char_token`], [Operadores y separadores de un carácter.],
    [$E$], [`unexpected_char` y demás], [Estados de error, con la etiqueta indicada en el autómata (@tab-lex-errores).],
  )},
  caption: [Estados del autómata, con la etiqueta de `lexer.c` que implementa cada uno.],
) <tab-lex-def-estados>
]

Antes de saltar a ningún estado, el analizador léxico hace dos cosas:

- Se salta los "carácteres blancos", siendo estos el salto de línea, el tabulador y el espacio, entre otros.

#if impl [
- En caso de que el _byte_ sobre el que tengamos que hacer la transición sea una letra mayúscula,
  se pone en minúscula para disminuir el posible salto a la mitad de entradas en la tabla, tanto para
  mejorar la predicción de saltos como para que en la memoria caché haya la mitad de entradas.

  Como mayúsculas y minúsculas acaban en el mismo estado, esta transformación es matemática, no
  condicional: en ASCII cada minúscula es su mayúscula con el bit 5 activado, así que basta con
  activarlo cuando el _byte_ es una mayúscula (@fig-phi).

  Esta transformación también depende de que ambos posibles resultados son expresiones puras:
  como ni $x_0$ ni $x_1$ tienen efectos secundarios, ambos pueden calcularse antes de conocer
  $c_0$, y el nodo $Phi$ se reduce a una selección sin saltos. Tras unificarse en él se realiza
  la transición modelada por la @fig-automata-lexico.

  #figure(
    diagram(
      spacing: (1.5em, 1.8em),
      node-stroke: 0.6pt,
      edge-stroke: 0.6pt,
      node-inset: 5pt,
      label-size: 0.85em,
      node((0, 0), $x_0 = "dispatched" \ c_0 = [(x_0 - 65) mod 2^8 <= 25]$, name: <entry>),
      node((-1, 1), $x_1 = x_0 or 2^5$, name: <upper>),
      node((0, 2), $x_2 = Phi(c_0, x_1, x_0)$, name: <phi>),
      node((0, 3), [`goto states[`$x_2$`]`], name: <goto>, stroke: none),
      edge(<entry>, <upper>, "-|>", $c_0 = 1$, label-side: right),
      edge(<entry>, <phi>, "-|>", $c_0 = 0$, label-side: left, bend: 45deg),
      edge(<upper>, <phi>, "-|>"),
      edge(<phi>, <goto>, "-|>"),
    ),
    caption: [Nodo $Phi$ que une los dos caminos de la conversión a minúsculas.],
  ) <fig-phi>
] else [- Las letras mayúsculas y minúsculas conducen al mismo estado, de modo que la transición no distingue entre ellas.

  Tras esto se realiza la transición modelada por la @fig-automata-lexico.
]

#if impl [

== Reconocimento de palabras reservadas

Una vez reconocido un identificador, falta distinguir si es una palabra reservada. Todas las
de _js-pdl_ tienen entre 2 y 8 caracteres, así que solo se comprueban los lexemas de esa
longitud, de nuevo con una única comparación sin signo:

$ 2 <= n <= 8 <==> (n - 2) mod 2^32 < 7 $

Un lexema $w = b_0 b_1 dots b_(n-1)$ de como mucho 8 _bytes_ cabe en un registro de 64 bits,
así que se codifica como un entero en _little endian_:

$ x(w) = sum_(i=0)^(n-1) b_i dot 2^(8i) in bb(Z)_(2^64) $

Los _bytes_ por encima de $n$ quedan a cero, de modo que $x$ es inyectiva y comparar dos
lexemas se reduce a comparar dos enteros. Para construir $x$ sin copiar el lexema _byte_ a
_byte_ se hacen cuatro lecturas solapadas de 16 y 32 bits, que cubren cualquier longitud
entre 2 y 8. La codificación de cada palabra reservada, así como la constante $M$
no se calculan en tiempo de ejecución: se precomputaron con _repr_ #footnote[https://codeberg.org/00jc/repr],
una herramienta del propio grupo que imprime la representación en _little endian_ de cada cadena.

Sobre $x$ se aplica un _hash_ multiplicativo que conserva los 4 bits altos del producto:

$ h(x) = floor((x dot M mod 2^64) / 2^60) in {0, dots, 15} $

$ M = #raw("0x1FB2E0CFCF4095CB") $

La constante $M$ hace que $h$ sea inyectiva sobre las 15 palabras reservadas (un _hash_
perfecto), por lo que basta una tabla $K$ de 16 entradas con la codificación de
cada palabra y otra $T$ con su tipo de _token_:

$ "tipo"(w) = cases(T[h(x(w))] & "si" K[h(x(w))] = x(w), "ID" & "en otro caso") $

La entrada 2 está vacía ($K[2] = 0$), y como ningún lexema de dos o más caracteres se
codifica como $0$, nunca coincide. $K$ y $T$ se guardan juntas en un mismo vector de 32
enteros (`kw_table`), de modo que la consulta son dos lecturas relativas a la misma
dirección, y la elección final entre $T[h]$ e `ID` es de nuevo una selección sin saltos.

== Constantes, cadenas y operadores

=== Constantes enteras (estados 4 y 5)

Se consumen los dígitos $d_0 dots d_(n-1)$ y el valor
se acumula mientras se leen, sin volver a recorrer el lexema, con la recurrencia de Horner
(módulo $2^64$, la aritmética de un `uint64_t`):

$ v_0 = 0, quad v_(i+1) = 10 v_i + d_i = (v_i << 3) + (v_i << 1) + d_i $

=== Constantes reales (estados 6 a 8)

Si tras los dígitos aparece un `.`, la parte entera se
calcula igual y se exige al menos un dígito decimal $e_1 dots e_m$. Estos se recorren de
derecha a izquierda, de modo que no hace falta calcular ninguna potencia $10^m$:

$ q_(m+1) = 0, quad q_j = (q_(j+1) + e_j) / 10 quad (j = m, dots, 1), quad
  "valor" = v + q_1 $

=== Cadenas (estados 9 a 11)

El lexema guardado excluye las comillas: empieza en el _byte_
siguiente a la comilla de apertura y termina en el de la comilla de cierre, y su longitud no
puede superar los 64 _bytes_. La barra invertida salta el carácter siguiente, de modo que
`\"` no cierra la cadena; el contenido se guarda sin interpretar las secuencias de escape.

=== Operadores con posible `=` (estados 12 a 14)

Sea $c in {mono("="), mono("!"), mono("<"),
mono(">"), mono("+")}$ el carácter leído y $n$ el siguiente. Como todos son ASCII, su bit 7
está libre, y el tipo de _token_ y cuántos _bytes_ se consumen son:

$ t(c, n) = c or 2^7 dot [n = mono("=")], quad "consumidos" = [n = mono("=")] $

Así `<` es `LT` y `<=` es `LE`, `0x3C` y `0xBC`, sin ninguna bifurcación.

=== Operadores lógicos (estados 15 y 16).

`&&` y `||` se reconocen si el siguiente carácter
$n$ es igual a $c$, y su tipo de _token_ es el propio $c$; un `&` o `|` aislado es un error,
ya que el lenguaje no tiene aritmética de bits.

== División y comentarios

Tras una `/` (estados 17 a 19 de la @fig-automata-lexico), si el siguiente carácter es otra
`/` se descarta todo hasta el salto de línea (o el final del fichero) y el analizador vuelve
al estado 0 sin emitir ningún _token_; en cualquier otro caso se emite `DIV`.

== Tokens de un carácter

Los nueve separadores y operadores que no pueden extenderse (`, ; ( ) { } - * %`, estado 20 de
la @fig-automata-lexico)
no necesitan mirar el carácter siguiente, así que se aceptan en
cuanto se leen. El truco es que el tipo del _token_ es el propio carácter:

$ t(c) = c, quad c in {mono(","), mono(";"), mono("("), mono(")"), mono("{"), mono("}"),
  mono("-"), mono("*"), mono("%")} $

`TOK_COMMA` vale `','`, `TOK_LPAREN` vale `'('`, etc., por lo que el estado se reduce a
`tok.type = dispatched`, sin tabla de conversión. Los tipos sin carácter propio
(constantes, identificadores y palabras reservadas) usan los valores 1 a 17, por debajo de
`0x20`, y no chocan con ningún carácter imprimible. El mismo valor sirve además de
clave de la tabla de saltos, de modo que el _byte_ leído determina a la vez el estado y,
en estos casos, el _token_.

== Utf-8

La manera que se ha usado para manejar carácteres que no corresponden con ningún carácter _ASCII_ es,
a nuestro entender, pragmática, pese a no formar parte de la especificación del lenguaje.

Como el lenguaje define su lenguaje en ASCII y el resto son parte del vocabulario que el usuario decida,
cualquier carácter que exista en el código con el bit más significativo se tratará como un identificador,
ya que ésta es la única parte que permite al usuario interceder en la semántica de lo que significa cada carácter,
esto quiere decir que se admiten identificadores que contengan o comienzen por, emojis, letras especiales como la ñ, etc.

]

== Acciones Semánticas

No corresponde a esta entrega


== Estados de error

Todo lo que no encaja en el autómata acaba en uno de los estados de error de la
@tab-lex-errores. El analizador no intenta recuperarse: el primer error detiene el análisis.

#[
#show figure: set block(breakable: false)
#figure(
  {
    set par(justify: false)
    table(
      columns: (auto, 1fr),
      align: left,
      stroke: (x, y) => {
        let res = (bottom: 1pt, top: 0pt)
        if (y < 2) {
          res.top = 1pt
        }
        res
      },
      table.header([Etiqueta], [Causa]),
      [`unexpected_char`], [El _byte_ no tiene entrada en la tabla de saltos.],
      [`ident_too_big`], [Identificador de más de 64 _bytes_.],
      [`float_follow_dot_error`], [Un `.` no va seguido de ningún dígito.],
      [`strlit_too_big`], [Cadena de más de 64 _bytes_.],
      [`error_unterminated_string`], [Fin de fichero antes de la comilla de cierre.],
      [`no_double_error`], [`&` o `|` sin el mismo carácter detrás.],
    )
  },
  placement: auto,
  caption: [Estados de error del analizador léxico.],
) <tab-lex-errores>
]

#if impl [
Todos confluyen en una misma rutina que informa de dónde está el fallo. Como es un camino
"_frío_"#footnote[El código rara vez se ejecuta], la tabla $L$ con el desplazamiento del
inicio de cada línea no se construye al arrancar, sino al producirse el primer error.
Con $p$ el desplazamiento del _token_ erróneo (para una cadena sin cerrar, el de la comilla de apertura),
la línea y la columna son:

$ "línea"(p) = max { i : L_i <= p }, quad "columna"(p) = p - L_("línea"(p)) + 1 $

y el mensaje se emite con el formato `error in <fichero>:<línea>:<columna>: <causa>`.

Todos los saltos a handlers de errores están marcados como "_fríos_", ya que los compiladores
normalmente asumen que las condiciones son verdad, por ejemplo _gcc_ por defecto optimiza para
que las condiciones sean ciertas un 90% de las veces @gccbuiltins.
]

#if impl [
== Tokens en memoria

Un _token_ tiene tres campos: el tipo (1 _byte_), el cuerpo (8 _bytes_), que es o bien el
inicio y el fin del lexema o bien el valor numérico, y el índice del símbolo (8 _bytes_).

=== Estructura de arrays

Lo habitual en C sería guardar los _tokens_ en un array de estructuras (AoS). Cada _token_
ocuparía entonces 20 _bytes_: tras el tipo, el compilador deja 3 _bytes_ de relleno para alinear
el cuerpo.

En su lugar, cada campo vive en su propio array contiguo (SoA), y los tres arrays comparten una
única reserva de memoria. Así no hay relleno, y cada _token_ ocupa 17 _bytes_.

La ventaja principal está en la lectura. El analizador sintáctico casi siempre necesita solo el
tipo, y en el AoS leerlo trae a la caché el _token_ entero: de cada línea de 64 _bytes_ solo
aprovecha 3. En el SoA los tipos están seguidos, así que una línea contiene 64 tipos
(@fig-soa-aos).

#figure(
  {
    leyenda-soa
    v(0.6em)
    layout(size => grafico-soa(size.width))
  },
  caption: [Una línea de caché de 64 _bytes_ en cada formato. En negro, lo que lee el analizador
    sintáctico cuando solo necesita el tipo.],
) <fig-soa-aos>

Cada array se obtiene con `xft_soa_fields(soa, ctx, i)`. Los _tokens_ se añaden con una
función propia del analizador, `ad_hoc_token_push`, que conoce en compilación el tamaño y el
desplazamiento de cada campo; con LTO se reduce a tres almacenamientos, sin llamadas.

=== Análisis bajo demanda

`lex_next_token` reconoce *un único* _token_, lo añade a los arrays y devuelve el control. Entre
llamadas solo necesita recordar un desplazamiento $p$ en el fichero y dos referencias a la tabla
de símbolos.

Quien consume los _tokens_ lleva un índice $i$, y solo pide uno nuevo cuando $i$ alcanza el
número de _tokens_ ya producidos. Así, el primer error léxico se informa sin haber analizado el
resto del fichero. Como todavía no hay analizador sintáctico, el programa principal se limita a
repetir la llamada hasta `EOF`.

=== Referencias estables

Los arrays pueden moverse en memoria al crecer, así que nada de lo que sale del analizador es un
puntero: son enteros, que siguen siendo válidos tras cualquier reasignación.

- El lexema es un par de desplazamientos de 32 bits sobre el fichero fuente. El fichero se
  proyecta en memoria una sola vez y no cambia durante el análisis, de modo que no se copia ni
  el texto del lexema ni las claves de la tabla de símbolos.
- Un identificador no guarda su nombre, sino su índice en el array de símbolos, que se asigna
  por orden de aparición:

  $ s(w) = cases(|S| & "la primera vez que se lee" w, S(w) & "en las siguientes") $

  Los índices son densos ($0, dots, |S|-1$), porque nunca se borra ningún símbolo, y no cambian:
  dos identificadores son el mismo si y solo si tienen el mismo índice.
- Los punteros a cada array del SoA sí se invalidan al añadir, así que no se guardan: se vuelven
  a pedir con `xft_soa_fields` en cada vuelta del bucle.

== Evaluación de las optimizaciones

Para saber cuánto aporta cada optimización, se quita una a una y se mide cuánto empeora el
analizador (@fig-ablacion).

#figure(
  layout(size => grafico-ablacion(size.width)),
  caption: [Efecto de quitar cada optimización en el tiempo total y en el pico de memoria
    (positivo: empeora sin ella; $approx 0$: no significativo; N/A: no afecta a la memoria).],
) <fig-ablacion>

=== Metodología

Se usan seis familias de corpus: un programa realista, identificadores todos distintos, cadenas,
comentarios, constantes numéricas y operadores sin espacios. Cada una se genera en tres tamaños
(64 KiB, 1 MiB y 16 MiB), 18 ficheros en total.

Cada variante quita una sola optimización, y una última las quita todas. Todas producen los
mismos _tokens_. Se compilan con `-O2` en lugar de `-O3`, para que el compilador no transforme por
su cuenta lo que se mide.

Las variantes se ejecutan intercaladas y en orden aleatorio, 30 veces por corpus (15 en los de
16 MiB), sin imprimir la salida. De cada ejecución se mide el tiempo de pared y el pico de memoria
residente, y el tiempo total es la suma de las medianas de los 18 corpus. Una diferencia se
considera no significativa ($approx 0$) #footnote[Aunque las diferencias sean no significativas,
_xft_ y el propio diseño del programa hacen que incluso en -O0 el código sea considerablemente
más rápido de lo que se esperaría de un "baseline", por ende se ha subido el umbral de ruido ligeramente]
si su intervalo de confianza del 95~% incluye el cero.

El equipo donde se realizan las pruebas se compone de un Intel Core i7-11800H con Linux 6.18,
_gcc_ 15.3 y un _governor_ `performance`.

=== Resultados

Con todas las optimizaciones, el analizador procesa unos 138 MB/s, unos 30 millones de _tokens_
por segundo. Sin ninguna, tarda un 54~% más.

=== AoS frente a SoA

Según la @fig-ablacion, guardar los _tokens_ como AoS no cambia de forma significativa el tiempo
del análisis, pero aumenta un 11~% el pico de memoria. Esa medida solo incluye el coste de
*construir* los _tokens_, y ahí los dos formatos están parejos:

- En el SoA, cada _token_ son tres almacenamientos de tamaño fijo.
- En el AoS, el _token_ se copia entero con `xft_vec_push_back`. Como 20 no es potencia de 2, la
  copia es una llamada a `xft_memcpy`.
- El SoA no tiene relleno, así que escribe 3 _bytes_ menos por _token_.

Lo que más justifica el SoA es el coste de *recorrer* los _tokens_, que es lo que hará el
analizador sintáctico. Para medirlo, se construyen los mismos _tokens_ en los dos formatos y se
mide solo el recorrido con _tailor_, el banco de pruebas de _xft_ basado en los contadores de
`perf`. Se usan los 3,6 millones de _tokens_ del programa de 16 MiB, que no caben en caché
(@tab-acceso).

#figure(
  {
    set par(justify: false)
    set text(size: 0.9em)
    table(
      columns: (1fr, auto),
      align: (left, right),
      inset: (x: 4pt, y: 2.5pt),
      stroke: (x, y) => {
        let res = (bottom: 0pt, top: 0pt)
        if y < 2 { res.top = 1pt }
        if y == 5 { res.bottom = 1pt }
        res
      },
      table.header([Patrón de acceso], [SoA frente a AoS]),
      [Secuencial, solo el tipo], [×5,7 más rápido],
      [Secuencial, filtra por tipo y lee el símbolo], [×1,2 más rápido],
      [Secuencial, todos los campos], [×2,2 más rápido],
      [Aleatorio, solo el tipo], [×4,9 más rápido],
      [Aleatorio, todos los campos], [×1,6 más lento],
    )
  },
  caption: [Recorrido de los _tokens_ ya construidos según el patrón de acceso.],
) <tab-acceso>

Cuando solo se lee el tipo, que es lo que hace casi siempre el analizador sintáctico, el SoA es
hasta 5,7 veces más rápido: por cada _token_ lee 1 _byte_ en lugar de 20 (@fig-soa-aos). El AoS
solo gana si se accede en orden aleatorio a todos los campos, porque entonces el SoA toca tres
líneas de caché por _token_.

El patrón más parecido a un analizador acoplado que necesite el lexema apenas gana: lo limitan
los fallos de predicción de saltos, no la memoria. Esto sugiere que el diseño del propio
analizador sintáctico influirá en las mediciones si está muy acoplado con fases como el
reconocimiento de usos y declaraciones.

El SoA es, por tanto, el formato adecuado: construirlo cuesta lo mismo, ocupa menos memoria y es
más rápido de recorrer en todos los patrones salvo el acceso aleatorio a todos los campos.

== Comparación con otras implementaciones

#figure(
  layout(size => grafico-comparacion(ancho: size.width - 1.8em.to-absolute() - 2 * 1.5cm)),
  scope: "parent",
  placement: top,
  caption: [Analizador léxico completo de cuatro grupos sobre los mismos 18 corpus, por tamaño de
    corpus. (a) y (c): suma de las medianas de los seis corpus de cada tamaño y pico de memoria
    residente. (b) y (d): cociente entre cada implementación y este trabajo.
    #super[\*]A no termina el corpus de 16 MiB de identificadores en 300 s; se cuenta como 300 s
    (cota inferior) y su pico de memoria no incluye ese corpus.],
) <fig-comparacion>

Se compara el analizador léxico con el de otros tres grupos de la asignatura, anonimizados como
A, B y C.

*Metodología.*

- *Mismos _tokens_:* cada grupo implementa una variante distinta del lenguaje (comillas,
  operadores y palabras reservadas). Por eso cada corpus se genera como una secuencia abstracta de
  _tokens_ y se escribe en el dialecto de cada grupo. Los cuatro analizan los mismos _tokens_, con
  los mismos identificadores, y el tamaño de los ficheros difiere menos de un 4~%.
- *Comprobación:* los cuatro reconocen exactamente el número de _tokens_ esperado, sin errores.
- *Sin cambios en su código:* no se modifican sus algoritmos ni sus opciones. Cuando su analizador
  léxico solo se invoca desde el sintáctico, se añade un programa que pide los _tokens_ en bucle,
  como lo haría el sintáctico.
- *Medida:* el programa completo, que escribe la lista de _tokens_ en un fichero, compilado con
  `-O3` como en el repositorio. Se hacen 10 rondas intercaladas en los corpus pequeños y medianos,
  y 5 en los grandes.

*Resultados.* Cada barra de tiempo de la @fig-comparacion es la suma de las medianas de los seis
corpus de ese tamaño.
]

= Tabla de Símbolos

No corresponde a esta entrega

#let pagina-automata = page(columns: 1)[
  #layout(size => {
    // Escala uniforme (sin deformar): la figura (autómata, leyenda y pie) ocupa el 70 % del
    // alto de la página.
    let leyenda = measure(block(width: size.width, leyenda-automata)).height
    let alto = 0.7 * size.height - leyenda - 3em.to-absolute()
    let d = measure(automata-lexico())
    let f = calc.min(size.width / d.width, alto / d.height)
    [#figure(
      {
        scale(f * 100%, reflow: true, automata-lexico())
        v(0.8em)
        leyenda-automata
      },
      kind: image,
      caption: [Autómata del analizador léxico. La @tab-lex-def-estados da la etiqueta de `lexer.c` de cada estado.],
    ) <fig-automata-lexico>]
  })
  #v(1fr)
  #tabla-estados
]

#pagina-automata

#if impl [#pagebreak(weak: true)]
