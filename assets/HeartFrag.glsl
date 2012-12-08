uniform sampler2D tex;
uniform sampler2D normalMap;
uniform bool textureEnabled;

varying vec3 lightVec;
varying vec3 eyeVec;

void main()
{
	vec3 normal = texture2D( normalMap, gl_TexCoord[ 0 ].st ).rgb;
	normal = normalize( normal );

	vec3 eye = normalize( eyeVec );
	vec3 light = normalize( lightVec );

	// compute diffuse lighting
	float lambertFactor = max( dot( lightVec, normal ), 0.0 );

	gl_FragColor.a = 1.0;

	if ( lambertFactor > 0.0 )
	{
		vec3 diffuseMaterial = gl_Color.rgb * gl_FrontMaterial.diffuse.rgb;
		if ( textureEnabled )
			diffuseMaterial *= texture2D ( tex, gl_TexCoord[0].st ).rgb;

		float specStrenght = 0.9; //gl_FrontMaterial.shininess;
		vec3 reflect = normalize( 2.0 * lambertFactor * normal - light );
		//diffuseLight = gl_LightSource[0].diffuse;
		float shininess = pow( max( dot( reflect, -eye ), 0.0 ), 8.0 ) * specStrenght;

		gl_FragColor.rgb = diffuseMaterial * lambertFactor;
		gl_FragColor.rgb += vec3( shininess, shininess, shininess );
	}
	else
		gl_FragColor.rgb = vec3( 0, 0, 0 );
}
