varying vec3 v; // eye space position
varying vec3 n; // normal
 
void main()
{
	vec3 L = normalize( gl_LightSource[ 0 ].position.xyz );   
	vec3 E = normalize( -v ); 
	vec3 N = normalize( n );
	vec3 R = normalize( -reflect( L, N ) );  

	// ambient term
	vec4 ambient = gl_LightSource[ 0 ].ambient *
				   gl_FrontMaterial.ambient;

	// diffuse term
	vec4 diffuse = gl_LightSource[ 0 ].diffuse * 
				   gl_FrontMaterial.diffuse;
	diffuse *= max( dot( N, L ), 0. );

	// specular term
	vec4 specular = gl_LightSource[ 0 ].specular *
					gl_FrontMaterial.specular;
	specular *= pow( max( dot( E, R ), 0. ), gl_FrontMaterial.shininess );

	gl_FragColor = ambient + diffuse + specular;
}
