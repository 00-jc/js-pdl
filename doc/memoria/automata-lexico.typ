#import "@preview/fletcher:0.5.8" as fletcher: diagram, node, edge

// Autómata del analizador léxico de js-pdl (`lex_next_token` en src/lexer.c).
//
// Organización en rejilla:
//   columna 0      estado inicial
//   columnas 1-3   estados intermedios de cada familia de tokens
//   columna 4      resultado: estado final (doble círculo) o de error (discontinuo)
//   columna 5      token emitido o etiqueta de error de lexer.c
// Cada familia ocupa una fila; sus errores van en las filas inmediatamente debajo.

#let st(pos, label, name) = node(pos, label, name: name, shape: circle, stroke: 0.6pt, width: 1.9em)
#let fin(pos, label, name) = node(pos, label, name: name, shape: circle, stroke: 0.6pt, width: 1.9em, extrude: (-2, 0))
#let err(pos, name) = node(pos, $E$, name: name, shape: circle, stroke: (thickness: 0.6pt, dash: "dashed"), width: 1.9em)
#let out(y, body) = node((5, y), text(size: 0.8em, body), stroke: none)
#let loop(at, label, angle) = edge(at, at, label, "-|>", bend: 130deg, loop-angle: angle)
#let go(from, to, label, ..args) = edge(from, to, label, "-|>", ..args)
// Transición desde el estado inicial: sale en horizontal, baja/sube por un bus vertical
// común y entra en horizontal en la fila de destino, con la etiqueta sobre ese último tramo.
#let bus-x = -1.1
#let from-q0(y, to, label, pos: 0.5) = edge(
  <q0>, (bus-x, 8), (bus-x, y), to, label, "-|>",
  label-pos: (2, pos), label-side: left, corner-radius: 4pt,
)

#let automata-lexico(col: 5.2em) = diagram(
  spacing: (col, 1.5em),
  edge-stroke: 0.6pt,
  label-size: 0.8em,
  mark-scale: 70%,

  // ── Estado inicial ─────────────────────────────────────────────
  st((-1.8, 8), $0$, <q0>),
  edge((-2.5, 7.2), <q0>, "-|>"),
  loop(<q0>, [del], 270deg),

  // ── Fin de fichero ─────────────────────────────────────────────
  fin((4, -0.7), $1$, <eof>),
  from-q0(-0.7, <eof>, [eof, `\0`], pos: 0.15),
  out(-0.7, [`EOF`]),

  // ── Identificadores / palabras reservadas ─────────────────────
  st((1, 1), $2$, <id>),
  from-q0(1, <id>, [l, `_`, utf8]),
  loop(<id>, [l, d, `_`, utf8], 90deg),
  fin((4, 1), $3^*$, <fid>),
  go(<id>, <fid>, [o.c.]),
  out(1, [`ID` / `PR`]),
  err((4, 2), <e-id>),
  go(<id>, <e-id>, [$> 64$], label-side: right),
  out(2, [`ident_too_big`]),

  // ── Constantes numéricas ───────────────────────────────────────
  st((1, 3), $4$, <int>),
  from-q0(3, <int>, [d]),
  loop(<int>, [d], 90deg),
  fin((4, 3), $5^*$, <fint>),
  go(<int>, <fint>, [o.c.]),
  out(3, [`CONST_INT`]),

  st((2, 4), $6$, <dot>),
  go(<int>, <dot>, [`.`], label-side: right),
  st((3, 4), $7$, <frac>),
  go(<dot>, <frac>, [d]),
  loop(<frac>, [d], 90deg),
  fin((4, 4), $8^*$, <ffloat>),
  go(<frac>, <ffloat>, [o.c.]),
  out(4, [`CONST_FLOAT`]),
  err((4, 5), <e-dot>),
  go(<dot>, <e-dot>, [o.c.], label-side: right),
  out(5, [`float_follow_dot_error`]),

  // ── Cadenas ────────────────────────────────────────────────────
  st((1, 6), $9$, <str>),
  from-q0(6, <str>, raw("\"")),
  loop(<str>, [c $!=$ #raw("\"") #raw("\\")], 90deg),
  st((1, 7.4), $10$, <esc>),
  go(<str>, <esc>, raw("\\"), bend: 30deg, label-side: left),
  go(<esc>, <str>, [c], bend: 30deg, label-side: left),
  fin((4, 6), $11$, <fstr>),
  go(<str>, <fstr>, [#raw("\"") ($<= 64$)]),
  out(6, [`STRING_LITERAL`]),
  err((4, 7), <e-strbig>),
  go(<str>, <e-strbig>, [#raw("\"") ($> 64$)], label-side: right, label-pos: 0.6),
  out(7, [`strlit_too_big`]),
  err((4, 8), <e-unterm>),
  go(<str>, <e-unterm>, [eof], label-side: right, label-pos: 0.75),
  out(8, [`error_unterminated_string`]),

  // ── Operadores con posible `=` ─────────────────────────────────
  st((1, 9), $12$, <opeq>),
  from-q0(9, <opeq>, [`= ! < > +`]),
  fin((4, 9), $13$, <fopeq>),
  go(<opeq>, <fopeq>, [`=`]),
  out(9, [`== != <= >= +=`]),
  fin((4, 10), $14^*$, <fop>),
  go(<opeq>, <fop>, [o.c.], label-side: right),
  out(10, [`= ! < > +`]),

  // ── Operadores lógicos dobles ──────────────────────────────────
  st((1, 11), $15$, <dbl>),
  from-q0(11, <dbl>, [`& |`]),
  fin((4, 11), $16$, <fdbl>),
  go(<dbl>, <fdbl>, [mismo c]),
  out(11, [`&& ||`]),
  err((4, 12), <e-dbl>),
  go(<dbl>, <e-dbl>, [o.c.], label-side: right),
  out(12, [`no_double_error`]),

  // ── Tokens de un carácter ──────────────────────────────────────
  fin((4, 13), $20$, <single>),
  from-q0(13, <single>, [`, ; ( ) { } - * %`], pos: 0.15),
  out(13, [`, ; ( ) { } - * %`]),

  // ── Carácter no reconocido ─────────────────────────────────────
  err((4, 14), <e-other>),
  from-q0(14, <e-other>, [o.c.], pos: 0.15),
  out(14, [`unexpected_char`]),

  // ── División / comentario ──────────────────────────────────────
  st((1, 15), $17$, <div>),
  from-q0(15, <div>, [`/`]),
  fin((4, 15), $18^*$, <fdiv>),
  go(<div>, <fdiv>, [o.c.]),
  out(15, [`/`]),
  st((2, 16), $19$, <com>),
  go(<div>, <com>, [`/`], label-side: right),
  loop(<com>, [c $!=$ `\n`], 0deg),
  edge(<com>, (2, 17), (-2.4, 17), (-2.4, 8), <q0>, [`\n`, eof], "-|>",
    corner-radius: 6pt, label-pos: 0.25, label-side: left),
)

// Abreviaturas de las transiciones. La etiqueta de `lexer.c` de cada estado va en la tabla de estados.
#let leyenda-automata = {
  set text(size: 0.8em)
  [l: letra #h(0.8em) d: dígito #h(0.8em) c: cualquier carácter #h(0.8em) o.c.: otro carácter
   #h(0.8em) del: delimitador #h(0.8em) $*$: no consume el último carácter]
}
