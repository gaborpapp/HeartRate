uniform float theta;

const float twoPi = 3.1415926 * 2.0;

void main()
{
	vec2 offset = vec2( cos( theta * twoPi ), sin( theta * twoPi ) ) * .25 + vec2( 0.5, 0.5 );
	float dist = 1. - 3. * distance( gl_TexCoord[ 0 ].st, offset );
	//float dist = cos( 13. * sin( .9723 * theta ) + 15. * gl_TexCoord[ 0 ].x * gl_TexCoord[ 0 ]. y );
	gl_FragColor = vec4( dist, dist, dist, 1.0 );
}

