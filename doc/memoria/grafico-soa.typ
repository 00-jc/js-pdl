#import "@preview/cetz:0.3.4"

// Una línea de caché de 64 bytes en AoS y en SoA (sección «Tokens en memoria»).
// AoS: t_token ocupa 20 bytes (tipo 1, relleno 3, cuerpo 8, símbolo 8; alineación 4).
// SoA: cada campo en su propio array.

#let tinta = black
#let tenue = luma(40%)
#let borde = 0.4pt + black
#let leido = luma(15%)      // lo que lee el sintáctico (el tipo)
#let otro = luma(88%)       // campos que no lee
#let relleno-alin = luma(100%)

#let grafico-soa(ancho) = {
  import cetz.draw: *
  let etq = 1.35 // ancho de las etiquetas de fila, en cm
  let w = ancho / 1cm - etq
  let b = w / 64 // ancho de un byte
  let hf = 0.32
  let sep = 0.22
  let fila(y, nombre, celdas, nota) = {
    content((-0.1, y + hf / 2), anchor: "east", text(size: 6.5pt, fill: tinta, nombre))
    let x = 0
    for (n, fill, et) in celdas {
      let xb = calc.min(x + n, 64)
      rect((x * b, y), (xb * b, y + hf), fill: fill, stroke: borde)
      if et != none and (xb - x) * b > 0.35 {
        content(((x + xb) / 2 * b, y + hf / 2), text(size: 5pt, fill: if fill == leido { white } else { tinta }, et))
      }
      x = xb
    }
    if nota != none {
      content((w, y - 0.04), anchor: "north-east", text(size: 5.5pt, fill: tenue, nota))
    }
  }
  cetz.canvas(length: 1cm, {
    // AoS: 3 tokens completos y 4 bytes del cuarto
    let tok = ((1, leido, [T]), (3, relleno-alin, none), (8, otro, [cuerpo]), (8, otro, [símbolo]))
    let aos = tok + tok + tok + tok
    let y0 = 3 * (hf + sep) + 0.35
    content((-etq, y0 + hf + 0.12), anchor: "south-west", text(size: 6.5pt, weight: "bold", [AoS]))
    fila(y0, [_tokens_], aos, [3,2 _tokens_ por línea: 3 _bytes_ útiles de 64])
    // SoA: una línea de cada array
    let ys = 2 * (hf + sep)
    content((-etq, ys + hf + 0.12), anchor: "south-west", text(size: 6.5pt, weight: "bold", [SoA]))
    fila(ys, [tipos], range(64).map(_ => (1, leido, none)), none)
    content((w / 2, ys + hf / 2), box(fill: leido, inset: (x: 2pt, y: 1pt), text(size: 5pt, fill: white, [64 tipos])))
    fila(hf + sep, [cuerpos], range(8).map(_ => (8, otro, [cuerpo])), none)
    fila(0, [símbolos], range(8).map(_ => (8, otro, [símbolo])), [64 _bytes_ útiles de 64 en la línea de tipos])
    // escala: 64 bytes
    let ye = y0 + hf + 0.5
    line((0, ye), (w, ye), stroke: 0.4pt + tenue, mark: (start: "|", end: "|", scale: 0.5))
    content((w / 2, ye + 0.05), anchor: "south", text(size: 5.5pt, fill: tenue, [una línea de caché: 64 _bytes_]))
  })
}

#let leyenda-soa = {
  set text(size: 6.5pt)
  let m(f) = box(baseline: 0.1em, rect(width: 0.75em, height: 0.75em, fill: f, stroke: borde))
  grid(
    columns: 3,
    column-gutter: 1.4em,
    m(leido) + h(0.35em) + [tipo (lo que lee el sintáctico)],
    m(otro) + h(0.35em) + [otros campos],
    m(relleno-alin) + h(0.35em) + [relleno de alineación],
  )
}
