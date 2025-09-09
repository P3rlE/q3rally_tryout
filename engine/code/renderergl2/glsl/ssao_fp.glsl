uniform sampler2D u_ScreenDepthMap;
uniform sampler2D u_LevelsMap;
uniform vec4   u_ViewInfo; // zfar / znear, zfar, 1/width, 1/height

varying vec2   var_ScreenTex;

const int NUM_SAMPLES = 16;
const vec3 sampleKernel[16] = vec3[16](
    vec3(0.5381, 0.1856, 0.4319),
    vec3(0.1379, 0.2486, 0.4430),
    vec3(0.3371, 0.5679, 0.0057),
    vec3(-0.6999, -0.0451, -0.0019),
    vec3(0.0689, -0.1598, -0.8547),
    vec3(0.0560, 0.0069, -0.1843),
    vec3(-0.0146, 0.1402, 0.0762),
    vec3(0.0100, -0.1924, 0.2779),
    vec3(-0.3577, -0.5301, -0.4358),
    vec3(-0.3169, 0.1063, 0.0158),
    vec3(0.0103, -0.5869, -0.0046),
    vec3(-0.0897, -0.4940, 0.3287),
    vec3(0.7119, -0.0154, -0.0918),
    vec3(-0.0533, 0.0596, -0.5411),
    vec3(0.0352, 0.0631, 0.5460),
    vec3(-0.4776, 0.2847, -0.0271)
);

float getDepth(vec2 texCoord) {
    return texture2D(u_ScreenDepthMap, texCoord).r;
}

void main()
{
    float depth = getDepth(var_ScreenTex);
    vec3 randomVec = texture2D(u_LevelsMap, var_ScreenTex * 4.0).xyz * 2.0 - 1.0;
    float occ = 0.0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        vec3 sampleDir = reflect(sampleKernel[i], randomVec);
        vec2 offset = sampleDir.xy * 4.0 * u_ViewInfo.zw;
        float sampleDepth = getDepth(var_ScreenTex + offset);
        occ += sampleDepth >= depth + sampleDir.z ? 1.0 : 0.0;
    }
    occ = 1.0 - (occ / float(NUM_SAMPLES));
    gl_FragColor = vec4(vec3(occ), 1.0);
}
