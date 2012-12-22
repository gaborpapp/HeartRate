//#version 120

uniform sampler2D displacement;
uniform float scale;

attribute vec3 tangent;

varying vec3 lightVec;
varying vec3 eyeVec;

void main()
{
	gl_FrontColor = gl_Color;        
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;

	vec4 color = texture2D( displacement, gl_MultiTexCoord0.st );
	float offset = color.r * scale;
	vec4 position = gl_Vertex;
	position.xyz += gl_Normal * offset;
	gl_Position = gl_ModelViewProjectionMatrix * position;

	// Building the matrix Eye Space -> Tangent Space
	vec3 n = normalize( gl_NormalMatrix * gl_Normal );
	vec3 t = normalize( gl_NormalMatrix * tangent );
	vec3 b = cross( n, t );

	vec3 vertexPosition = vec3( gl_ModelViewMatrix * position );
	vec3 lightDir = normalize( gl_LightSource[0].position.xyz - vertexPosition );

	// transform the vectors by tangent basis
	vec3 v;
	v.x = dot( lightDir, t );
	v.y = dot( lightDir, b );
	v.z = dot( lightDir, n );
	lightVec = normalize( v );

	v.x = dot( vertexPosition, t );
	v.y = dot( vertexPosition, b );
	v.z = dot( vertexPosition, n );
	eyeVec = normalize( v );
}

