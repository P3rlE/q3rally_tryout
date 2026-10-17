// Q3Rally: camera motion blur, see RB_MotionBlur in tr_postprocess.c
uniform sampler2D u_TextureMap;
uniform sampler2D u_ScreenDepthMap;

uniform mat4   u_ModelMatrix;	// this frame's NDC -> last frame's clip space
uniform vec4   u_ViewInfo;		// x = zNear, y = zFar
uniform vec4   u_Color;			// x = blur scale, y = longest blur, z/w = depth range of the fade in
uniform vec2   u_InvTexRes;

varying vec2   var_Tex1;

#define SAMPLES 8

float BlurMask(float depth)
{
	float n = u_ViewInfo.x;
	float f = u_ViewInfo.y;
	float z = 2.0 * n * f / (f + n - (2.0 * depth - 1.0) * (f - n));

	return smoothstep(u_Color.z, u_Color.w, z);
}

void main()
{
	vec4 color = texture2D(u_TextureMap, var_Tex1);
	float depth = texture2D(u_ScreenDepthMap, var_Tex1).r;
	float mask = BlurMask(depth);

	vec4 prev = u_ModelMatrix * vec4(var_Tex1 * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
	if (mask <= 0.0 || prev.w <= 0.0)
	{
		gl_FragColor = color;
		return;
	}

	vec2 velocity = (var_Tex1 - (prev.xy / prev.w * 0.5 + 0.5)) * (u_Color.x * mask);
	float len = length(velocity);
	if (len > u_Color.y)
		velocity *= u_Color.y / len;

	// shorter than half a pixel: nothing to do
	if (len * max(1.0 / u_InvTexRes.x, 1.0 / u_InvTexRes.y) < 0.5)
	{
		gl_FragColor = color;
		return;
	}

	// samples along the motion, centred on the pixel; samples on near
	// pixels (the own car) are left out so it does not smear outwards
	vec3 sum = color.rgb;
	float weight = 1.0;
	for (int i = 0; i < SAMPLES; i++)
	{
		float t = (float(i) + 0.5) / float(SAMPLES) - 0.5;
		vec2 tc = clamp(var_Tex1 + velocity * t, u_InvTexRes * 0.5, 1.0 - u_InvTexRes * 0.5);
		float w = BlurMask(texture2D(u_ScreenDepthMap, tc).r);

		sum += texture2D(u_TextureMap, tc).rgb * w;
		weight += w;
	}

	gl_FragColor = vec4(sum / weight, color.a);
}
