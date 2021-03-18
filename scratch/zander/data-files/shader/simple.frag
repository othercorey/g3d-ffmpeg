//In values
varying in vec4 gl_FragColor;
varying in vec4 gl_FragCoord;
varying in vec2 UV;
varying in vec4 f_color;
	
//Out values
varying out vec4 out_color;

// Static
uniform sampler2D map;
uniform vec2 screen_size;
uniform float near;
uniform float far;

//main shader
void main(void)
{
    out_color         = texture2D(map, UV.xy);
    float linearDepth = abs(1./gl_FragCoord.w-near)/(far-near);
    out_color.w       = abs(linearDepth);
}
