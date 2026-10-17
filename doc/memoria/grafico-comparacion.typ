#import "@preview/cetz:0.3.4"

// Comparación con otras implementaciones (sección «Comparación con otras implementaciones»).
// Por tamaño de corpus: suma de las medianas de los seis corpus de ese tamaño y pico de memoria
// residente (máximo de los seis). A no termina identificadores_L en 300 s: su tiempo en L es una
// cota inferior (cuenta 300 s) y su pico en L no incluye ese corpus.

#let tamanos = ([64 KiB], [1 MiB], [16 MiB])

// (nombre, relleno, tiempos en s por tamaño, picos en MiB por tamaño); la primera es la referencia
#let series = (
  ([Este trabajo (C23)], luma(15%), (0.0061, 0.0797, 1.289), (13.0, 17.0, 162.6)),
  ([C (Java)], luma(45%), (0.4433, 1.1996, 6.3805), (93.4, 197.8, 1087.3)),
  ([B (Java)], luma(72%), (0.9733, 3.2829, 31.7787), (124.4, 241.0, 1117.8)),
  ([A (Java)], luma(93%), (0.9047, 28.0109, 349.8863), (112.4, 247.8, 227.1)),
)
// Valores que son cota inferior o están incompletos: (serie, tamaño)
#let cota = ((3, 2),)

#let tinta = black
#let tenue = luma(35%)
#let borde = 0.4pt + black

// Dos cifras significativas, con coma decimal y sin perder ceros finales (0,90).
#let sig2(v) = {
  let d = calc.max(0, 1 - calc.floor(calc.log(v, base: 10)))
  let n = int(calc.round(v * calc.pow(10, d)))
  if d == 0 { str(n) } else {
    let s = str(n)
    while s.len() <= d { s = "0" + s }
    s.slice(0, s.len() - d) + "," + s.slice(s.len() - d)
  }
}
#let fmt-tiempo(v) = if v < 0.1 { sig2(v * 1000) + " ms" } else { sig2(v) + " s" }
#let fmt-mem(v) = str(int(calc.round(v)))
#let fmt-factor(v) = "×" + if v < 10 { sig2(v) } else { str(int(calc.round(v))) }

// Un panel de barras agrupadas por tamaño de corpus.
//   valores(s, g): valor de la serie s en el tamaño g
//   escala: "log" o "lin"; ticks: ((valor, etiqueta), ...)
//   refs: índices de las series que se dibujan
//   linea-uno: dibuja la referencia ×1 (este trabajo)
#let panel(
  titulo, eje, valores, fmt, lo, hi, ticks,
  escala: "log", refs: none, ancho: 7cm, alto: 3.2cm, linea-uno: false, tam-etiqueta: 5.5pt,
) = {
  import cetz.draw: *
  let refs = if refs == none { range(series.len()) } else { refs }
  let lg(v) = calc.log(v, base: 10)
  let y(v) = if escala == "log" {
    (lg(v) - lg(lo)) / (lg(hi) - lg(lo)) * alto / 1cm
  } else {
    (v - lo) / (hi - lo) * alto / 1cm
  }
  let w = ancho / 1cm
  let h = alto / 1cm
  let n = refs.len()
  let hueco-grupo = 0.3
  let ancho-grupo = (w - hueco-grupo * tamanos.len()) / tamanos.len()
  let bw = ancho-grupo / n
  let gap = 0.02
  cetz.canvas(length: 1cm, {
    for (v, et) in ticks {
      if v != lo {
        line((0, y(v)), (w, y(v)), stroke: (paint: luma(80%), thickness: 0.3pt, dash: "dotted"))
      }
      line((-0.06, y(v)), (0, y(v)), stroke: borde)
      content((-0.1, y(v)), anchor: "east", text(size: 6pt, fill: tinta, et))
    }
    if linea-uno {
      line((0, y(1)), (w, y(1)), stroke: (paint: black, thickness: 0.5pt, dash: "dashed"))
    }
    for (g, _) in tamanos.enumerate() {
      let x0 = hueco-grupo / 2 + g * (ancho-grupo + hueco-grupo)
      for (k, s) in refs.enumerate() {
        let v = valores(s, g)
        let xa = x0 + k * bw + gap
        let xb = x0 + (k + 1) * bw - gap
        rect((xa, y(lo)), (xb, y(v)), fill: series.at(s).at(1), stroke: borde)
        let et = fmt(v)
        if cota.contains((s, g)) { et = et + "*" }
        content(((xa + xb) / 2, y(v) + 0.05), anchor: "south", text(size: tam-etiqueta, fill: tinta, et))
      }
      let xc = x0 + ancho-grupo / 2
      line((xc, 0), (xc, -0.06), stroke: borde)
      content((xc, -0.12), anchor: "north", text(size: 6.5pt, fill: tinta, tamanos.at(g)))
    }
    // ejes
    line((0, h), (0, 0), (w, 0), stroke: borde)
    content((-1, h / 2), angle: 90deg, text(size: 6.5pt, fill: tinta, eje))
    content((w / 2, h + 0.3), anchor: "south", text(size: 7pt, fill: tinta, weight: "bold", titulo))
  })
}

#let muestra(relleno) = box(baseline: 0.1em, rect(width: 0.75em, height: 0.75em, fill: relleno, stroke: borde))

#let leyenda-comparacion = {
  set text(size: 6.5pt, fill: tinta)
  grid(
    columns: series.len() + 1,
    column-gutter: 1.4em,
    ..series.map(s => muestra(s.at(1)) + h(0.35em) + s.at(0)),
    box(baseline: -0.2em, line(length: 1.4em, stroke: (paint: black, thickness: 0.5pt, dash: "dashed")))
      + h(0.35em) + [×1 = este trabajo],
  )
}

#let tiempo(s, g) = series.at(s).at(2).at(g)
#let memoria(s, g) = series.at(s).at(3).at(g)

// ancho: ancho total de los dos paneles de una fila. Se reparte para que todas las barras midan
// lo mismo: los absolutos tienen 4 barras por grupo y los relativos, 3.
#let grafico-comparacion(ancho: 13cm) = {
  set align(center)
  let otras = range(1, series.len())
  let n = series.len()
  let fija = 2 * 0.3cm * tamanos.len() // huecos entre grupos de los dos paneles
  let bw = (ancho - fija) / (tamanos.len() * (2 * n - 1))
  let abs = bw * tamanos.len() * n + 0.3cm * tamanos.len()
  let rel = ancho - abs
  let pa(..args) = panel(..args, ancho: abs, tam-etiqueta: 5pt)
  let pr(..args) = panel(..args, ancho: rel)
  stack(
    dir: ttb,
    spacing: 0.9em,
    leyenda-comparacion,
    grid(
      columns: 2,
      column-gutter: 1.8em,
      row-gutter: 1.2em,
      pa(
        [(a) Tiempo total], [Tiempo (escala log.)],
        tiempo, fmt-tiempo, 0.001, 1000,
        ((0.001, [1 ms]), (0.01, [10 ms]), (0.1, [100 ms]), (1, [1 s]), (10, [10 s]), (100, [100 s]), (1000, [1000 s])),
      ),
      pr(
        [(b) Tiempo relativo a este trabajo], [Factor de tiempo],
        (s, g) => tiempo(s, g) / tiempo(0, g), fmt-factor, 0, 400,
        range(0, 401, step: 100).map(v => (v, [×#v])),
        escala: "lin", refs: otras,
      ),
      pa(
        [(c) Pico de memoria], [Memoria (MiB, escala log.)],
        memoria, fmt-mem, 10, 2000,
        ((10, [10]), (100, [100]), (1000, [1000])),
      ),
      pr(
        [(d) Memoria relativa a este trabajo], [Factor de memoria],
        (s, g) => memoria(s, g) / memoria(0, g), fmt-factor, 0, 16,
        range(0, 17, step: 4).map(v => (v, [×#v])),
        escala: "lin", refs: otras, linea-uno: true,
      ),
    ),
  )
}
