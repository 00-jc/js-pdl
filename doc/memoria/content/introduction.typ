#import "@preview/magic-isprs:0.1.0": isprs-heading
#show: isprs-heading

#let impl = sys.inputs.at("impl", default: "true") == "true"

= Introducción

Esta memoria documenta la práctica de la asignatura de Procesadores de Lenguajes,
cuyo objetivo es construir el _frontend_ de un traductor para *_js-pdl_*, un
subconjunto de _JavaScript_ definido por la propia asignatura. El _frontend_
comprende el analizador léxico, el analizador sintáctico y el analizador
semántico, junto con la tabla de símbolos y el gestor de errores que comparten.

Cada grupo trabaja con una variante del lenguaje. Sobre la base común (funciones,
variables con declaración explícita, tipos `int`, `float`, `boolean`, `string` y `void`,
condicional simple, entrada/salida por terminal y expresiones con operadores aritméticos,
relacionales y lógicos), al grupo 49 le corresponden:

- la sentencia repetitiva `do`-`while`;
- la asignación con suma (`+=`);
- los comentarios de línea (`//`);
- las cadenas entre comillas dobles (`" "`);
- el análisis sintáctico descendente recursivo.

El proyecto se ha desarrollado en C23, apoyándose en las extensiones _GNUC_ que
exponen _clang_ y _gcc_ @gccattributes @gccbuiltins @clangbuiltins. La elección
responde a que C era el lenguaje común a todos los integrantes del grupo, lo que
minimizaba la fricción, sin renunciar a constructos modernos ni a la posibilidad de
expresar explícitamente al compilador la semántica del traductor (_gcc >= 15_ y
_clang >= 22_). En lugar de la biblioteca estándar, la implementación se apoya en
*_xft_*, una biblioteca estándar de C desarrollada por Jaime Castro Enrique en otra
institución, por lo que el traductor solo depende del sistema operativo.

Más allá de la corrección, el grupo se ha marcado como objetivo secundario el
rendimiento. De ahí el enfoque orientado a datos @acton2014dod, que toma como
referencia analizadores de alto rendimiento como _simdjson_ @langdale2019simdjson
o el _tokenizer_ de Zig @zigstd @salter2024zigtokenizer.

#if not impl [
En el resto de secciones no se darán detalles de implementación; a partir del código
se puede derivar una versión de esta memoria que sí los incluye (el código typst de la memoria está incluido en el repo),
recomendamos encarecidamente leerla.
] else [
En el resto de secciones se darán detalles de implementación y benchmars; a partir del código
se puede derivar una versión de esta memoria que no los incluye (el código typst de la memoria está incluido en el repo) 
]
