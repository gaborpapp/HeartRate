varying vec3 v; // eye space position
varying vec3 n; // normal

void main()
{
	// transform vertex into eye space
	v = vec3( gl_ModelViewMatrix * gl_Vertex );
	// transform normal into eye space
	n = gl_NormalMatrix * gl_Normal;

	// standard pass-through transformations
	gl_TexCoord[ 0 ] = gl_MultiTexCoord0;
	gl_Position = ftransform();
}

