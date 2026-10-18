// Q3Rally: wet ground, see RB_WetGroundPass in tr_shade.c
//
// Drawn over a world surface after its normal stages (depth equal) with
// blendFunc GL_ONE GL_SRC_ALPHA: the alpha darkens what is already there,
// the colour adds the reflection.
//
// The reflection is a screen space ray march: the reflected view ray is
// stepped through the depth of this frame (u_ScreenDepthMap, copied after
// the depth prepass), and the colour at the hit is read from the previous
// frame (u_TextureMap, half size with mip maps), reprojected with
// u_PrevViewProj. Rays that leave the screen or find nothing fade out.

uniform sampler2D u_ScreenDepthMap;
uniform sampler2D u_TextureMap;

uniform mat4   u_ModelViewProjectionMatrix;
uniform mat4   u_PrevViewProj;
uniform vec3   u_ViewOrigin;
uniform vec4   u_ViewInfo;		// x = zNear, y = zFar, z = steps, w = highest mip level
uniform vec4   u_WetParams;		// x = wetness, y = reflection, z = darkening, w = puddles

varying vec3   var_Position;
varying vec3   var_Normal;

#define MAX_STEPS 40

float Hash(vec2 p)
{
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float Noise(vec2 p)
{
	vec2 i = floor(p);
	vec2 f = fract(p);
	vec2 u = f * f * (3.0 - 2.0 * f);

	return mix(mix(Hash(i), Hash(i + vec2(1.0, 0.0)), u.x),
	           mix(Hash(i + vec2(0.0, 1.0)), Hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

float LinearZ(float depth)
{
	float n = u_ViewInfo.x;
	float f = u_ViewInfo.y;
	return 2.0 * n * f / (f + n - (2.0 * depth - 1.0) * (f - n));
}

vec3 Project(vec3 p)
{
	vec4 c = u_ModelViewProjectionMatrix * vec4(p, 1.0);
	if (c.w <= 0.001)
		return vec3(-1.0);
	return c.xyz / c.w * 0.5 + 0.5;
}

float EdgeFade(vec2 uv)
{
	vec2 e = smoothstep(vec2(0.0), vec2(0.06), uv) * smoothstep(vec2(0.0), vec2(0.06), 1.0 - uv);
	return e.x * e.y;
}

vec3 SampleScene(vec2 uv, float mip)
{
#if __VERSION__ >= 130
	return textureLod(u_TextureMap, uv, mip).rgb;
#else
	return texture2D(u_TextureMap, uv, mip).rgb;
#endif
}

void main()
{
	float wet = u_WetParams.x;
	vec3  N = normalize(var_Normal);
	vec3  V = normalize(var_Position - u_ViewOrigin);

	// water gathers on the flat parts: walls get a bit darker, no mirror
	float up = smoothstep(0.55, 0.85, N.z);

	// puddles, more of them the wetter it is
	float n = Noise(var_Position.xy / 230.0) * 0.65 + Noise(var_Position.xy / 61.0) * 0.35;
	float puddle = smoothstep(0.66 - 0.14 * wet, 0.72 - 0.14 * wet, n) * smoothstep(0.45, 1.0, wet) * u_WetParams.w * up;

	// wet asphalt is rough: the normal wobbles a little outside the puddles
	vec2 wobble = vec2(Noise(var_Position.xy / 9.0), Noise(var_Position.yx / 9.0 + 17.0)) - 0.5;
	N = normalize(N + vec3(wobble * 0.05 * (1.0 - puddle), 0.0));

	float cosT = clamp(-dot(V, N), 0.0, 1.0);
	float fresnel = 0.03 + 0.97 * pow(1.0 - cosT, 5.0);
	float reflection = u_WetParams.y * wet * up * mix(0.45, 1.0, puddle) * fresnel;
	float dark = u_WetParams.z * wet * mix(0.5, 1.0, up);

	vec3 color = vec3(0.0);
	float found = 0.0;

	if (reflection > 0.002)
	{
		vec3  R = reflect(V, N);
		vec3  p = var_Position + N * 1.5;
		vec3  last = p;
		float stepLen = 4.0 + LinearZ(gl_FragCoord.z) * 0.01;
		float jitter = Hash(gl_FragCoord.xy);
		float mip = mix(u_ViewInfo.w * 0.6, 0.0, puddle);
		vec2  lastUV = vec2(-1.0);
		bool  hit = false;
		bool  blocked = false;
		int   i;

		p += R * stepLen * jitter;

		for (i = 0; i < MAX_STEPS; i++)
		{
			if (float(i) >= u_ViewInfo.z)
				break;

			last = p;
			p += R * stepLen;
			stepLen *= 1.18;

			vec3 s = Project(p);
			if (s.x < 0.0 || s.x > 1.0 || s.y < 0.0 || s.y > 1.0 || s.z < 0.0 || s.z > 1.0)
				break;
			lastUV = s.xy;

			float sceneDepth = texture2D(u_ScreenDepthMap, s.xy).r;
			if (s.z > sceneDepth)
			{
				// behind something: a hit if it is not far behind it
				if (LinearZ(s.z) - LinearZ(sceneDepth) < stepLen * 2.0 + 16.0)
				{
					vec3 a = last;
					vec3 b = p;
					int j;

					for (j = 0; j < 5; j++)
					{
						vec3 m = (a + b) * 0.5;
						vec3 sm = Project(m);
						if (sm.z > texture2D(u_ScreenDepthMap, sm.xy).r)
							b = m;
						else
							a = m;
					}
					p = b;
					hit = true;
				}
				else
				{
					blocked = true;		// passed behind a thick object
				}
				break;
			}
		}

		if (hit)
		{
			vec4 prev = u_PrevViewProj * vec4(p, 1.0);
			if (prev.w > 0.001)
			{
				vec2 uv = prev.xy / prev.w * 0.5 + 0.5;
				vec3 s = Project(p);

				found = EdgeFade(uv) * EdgeFade(s.xy);
				color = SampleScene(clamp(uv, 0.0, 1.0), mip);
			}
		}
		else if (!blocked && lastUV.x >= 0.0 && R.z > 0.0
			&& (i < MAX_STEPS && float(i) < u_ViewInfo.z || texture2D(u_ScreenDepthMap, lastUV).r > 0.99999))
		{
			// the ray went off the screen or ran into the sky: take the
			// colour where it left (mostly sky), blurred
			color = SampleScene(lastUV, u_ViewInfo.w);
			found = 0.6 * smoothstep(0.0, 0.2, R.z);
		}
	}

	reflection *= found;

	gl_FragColor = vec4(color * reflection, (1.0 - dark) * (1.0 - reflection));
}
