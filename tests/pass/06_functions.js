// definición de funciones, parámetros, retorno y llamadas
function int suma(int x, int y)
{
	return x + y;
}

function void saluda(string nombre)
{
	echo "hola ";
	echo nombre;
}

function boolean es_positivo(float v)
{
	return v > 0.0;
}

function void nada(void)
{
	return;
}

let int r;
r = suma(1, 2);
saluda("mundo");
nada();
