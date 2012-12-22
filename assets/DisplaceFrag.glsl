uniform float time;
uniform float amplitude;
uniform float amp0;
uniform float amp1;
uniform float disharmony;

#define PI 3.14159265

float wave( float period )
{
	return sin( period * PI * 2. );
}

// calculate displacement based on uv coordinate
float displace( vec2 uv )
{
	// large up and down movement
	float d = wave( ( uv.x * 0.51379 ) - time * 0.17113 );
	// add a large wave from left to right
	d -= 1.3196 * wave( ( uv.x * 0.91003 ) - time * 0.41177 );
	// add diagonal waves from back to front
	d -= 0.2518 * wave( ( ( uv.x + uv.y) * 2.2682 ) - time * 0.53321 );
	// add additional waves for increased complexity
	d += 0.24792 * wave( ( uv.y * 1.19765 ) - time * 0.131312 );
	d -= 0.14591 * wave( ( ( uv.y + uv.x ) * 2.7784 ) - time * 0.9346 );
	d += 0.15098 * wave( ( ( uv.y - uv.x ) * 1.9188 ) - time * 0.8063 );

	return d;
}

float gauss( float sig, float mu, float x )
{
	return 1. / ( sig * sqrt( 2. * PI ) ) * exp( - 3. * ( x - mu ) * ( x - mu ) / ( 2. * sig * sig ) );
}

float beat( vec2 uv )
{
	float b0 = amp0 * gauss( .7, .25, uv.x );
	float b1 = amp1 * gauss( .7, .75, uv.x );

	return mix( b0, b1, smoothstep( .2, .8, uv.x ) );
}

void main()
{
	float d = amplitude * beat( gl_TexCoord[ 0 ].xy );
	d *= 1. + disharmony * displace( gl_TexCoord[ 0 ].xy );
	d *= wave( .5 * gl_TexCoord[ 0 ].y );
	gl_FragColor = vec4( d, d, d, 1.0 );
}

