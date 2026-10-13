// programa completo mezclando funciones y sentencias globales
let int total;

function int factorial(int n)
{
	let int acc;
	acc = 1;
	if (n <= 1) return 1;
	do
	{
		acc = acc * n;
		n = n - 1;
	} while (n > 1);
	return acc;
}

function boolean en_rango(int v, int lo, int hi)
{
	return v >= lo && v <= hi;
}

function void informe(string titulo, int valor)
{
	echo titulo;
	echo valor;
}

total = factorial(5);
if (en_rango(total, 100, 200)) informe("factorial de 5:", total);
total += factorial(3);
informe("total:", total);
