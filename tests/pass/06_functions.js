// definición de funciones, parámetros, retorno y llamadas
function suma int (x int, y int)
{
	return x + y;
}

function saluda void (nombre string)
{
	echo "hola ";
	echo nombre;
}

function es_positivo boolean (v float)
{
	return v > 0.0;
}

function nada void (void)
{
	return;
}

let r int;
r = suma(1, 2);
saluda("mundo");
nada();
