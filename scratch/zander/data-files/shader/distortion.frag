//In values
varying in vec4 gl_FragColor;
varying in vec4 gl_FragCoord;
varying in vec2 UV;
varying in vec4 f_color;

//Out values
varying out vec4 out_color;

// Uniform constants.
uniform float bright;

// Uniforms passed from memory.
uniform sampler2D fb_result;

// Screen resolution.
uniform vec2 screen_size;
uniform vec2 proj_size;
uniform vec2 rot_ang;
uniform vec2 left_center;
uniform vec2 right_center;

// Temporary value.
vec2  uv          = vec2(0,0);
vec2  center      = vec2(0,0);
vec2  center_orig = vec2(0,0);
float px          = 0;
float py          = 0;
float ang         = 0;

//main shader
void main()
{   
    px        = UV.x;
    py        = UV.y;
    uv        = vec2(int(px),int(py));
    out_color = texture2D(fb_result, uv/screen_size)*bright;
}
/*
    px = gl_FragCoord.x;
    py = gl_FragCoord.y;
    uv = vec2(int(px),int(py));
    // Check if process to be applied on left eye image.
    if (gl_FragCoord.x <= screen_size.x/2)
    {   
        ang         = rot_ang.x;   
        center      = vec2(proj_size.x/2,proj_size.y/2); 
        center_orig = left_center;
//        uv.y        = proj_size.y-uv.y;
//        uv.x        = proj_size.x-uv.x;
    }

    if (gl_FragCoord.x > screen_size.x/2)
    {
        ang         = rot_ang.y;
        center      = vec2(proj_size.x*3/2,proj_size.y/2);
        center_orig = right_center;
    }    

    // Rotation
    float sin_factor = sin(ang);
    float cos_factor = cos(ang);
    uv               = (uv - center) * mat2(cos_factor, sin_factor, -sin_factor, cos_factor);
    uv              += center_orig;
    
    // Normalization
    uv               = uv/screen_size;
    out_color        = texture2D(fb_result, uv)*bright;
    out_color.w      = 1;
}
*/
