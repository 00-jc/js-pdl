// programa completo mezclando funciones y sentencias globales
let total int;

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

function en_rango boolean (v int, lo int, hi int)
{
	return v >= lo && v <= hi;
}

function informe void (titulo string, valor int)
{
	echo titulo;
	echo valor;
}

total = factorial(5);
if (en_rango(total, 100, 200)) informe("factorial de 5:", total);
total += factorial(3);
informe("total:", total);
