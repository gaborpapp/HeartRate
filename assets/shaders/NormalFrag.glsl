// by Paul Houx
// https://github.com/paulhoux/Cinder-Samples/blob/master/SmoothDisplacementMapping/assets/normal_map_frag.glsl
// adapted from: http://www.tartiflop.com/disp2norm/srcview/index.html
// http://www.tartiflop.com/disp2norm/srcview/source/com/tartiflop/PlanarDispToNormConverter.as.html
// note: I have omitted the second and third order processing, because this
//       would require branching, which would stall the GPU needlessly.
//       You will notice incorrect normals at the edges of the normal map.
    
#version 120

uniform sampler2D texture;
uniform float amplitude;
    
float getDisplacement( float dx, float dy )
{   
    vec2 uv = gl_TexCoord[ 0 ].xy;
    return texture2D( texture, uv + vec2( dFdx( uv.s ) * dx, dFdy( uv.t ) * dy ) ).r;
}   
    
void main(void)
{   
    // calculate first order centered finite difference (y-direction)
    vec3 normal;

	if ( amplitude == .0 )
	{
		normal = vec3( 0, 0, 1 );
	}
	else
	{
		normal.x = -0.5 * ( getDisplacement( 1, 0 ) - getDisplacement( -1, 0 ) );
		normal.y = -0.5 * ( getDisplacement( 0, 1 ) - getDisplacement( 0, -1 ) );
		normal.z = 1.0 / amplitude;
		normal = normalize( normal );
	}
    
    gl_FragColor = vec4( normal, 1.0 );
}   

