uniform sampler2D txt;

void main()
{
	vec2 uv = gl_TexCoord[ 0 ].st;
	vec4 clr = texture2D( txt, uv );
	const float fadeRange = .2;
	clr.a *= smoothstep( .0, fadeRange, uv.x );
	clr.a *= smoothstep( 1., 1. - fadeRange, uv.x );
	gl_FragColor = clr;
}
