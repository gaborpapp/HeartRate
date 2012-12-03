uniform sampler2D displacement;
uniform float scale;

varying vec3 v;
varying vec3 N;

void main()
{
	v = vec3( gl_ModelViewMatrix * gl_Vertex );
	N = normalize( gl_NormalMatrix * gl_Normal );

	gl_FrontColor = gl_Color;        
	gl_TexCoord[ 0 ] = gl_MultiTexCoord0;

	vec3 normal = gl_Normal;
	vec2 uv = gl_MultiTexCoord0.st;
	vec4 color = texture2D( displacement, uv );
	float offset = color.r * scale;
	vec4 position = gl_Vertex;
	position.xyz += normal * offset;
	gl_Position = gl_ModelViewProjectionMatrix * position;
}

