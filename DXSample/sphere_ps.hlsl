struct VSOut {
  float4 pos : SV_Position;
  float3 nrm : NORMAL;
  float3 vpos : TEXCOORD1;
  float2 uv : TEXCOORD0;
};

// ───── テクスチャ (bindlessでも可) ────────────────────
Texture2D gAlbedo : register(t0);
SamplerState gSampler : register(s0);

// ───── 常量バッファ 1 (b1) ───────────────────────────
cbuffer CB_Light : register(b1) {
  float3 lightDir;  // view-space, normalized
  float lightInt;   // intensity
  float3 camPosVS;  // camera position (view-space, normally 0)
  float roughness;  // 0 → 鏡面
  float3 lightColor;
  float metallic;
};

// ───── 補助関数 (Schlick-Fresnel) ───────────────────
float3 fresnelSchlick(float cosTheta, float3 F0) {
  return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// ───── PSMain ───────────────────────────────────────
float4 PSMain(VSOut i) : SV_Target {
  float3 N = normalize(i.nrm);
  float3 V = normalize(camPosVS - i.vpos);
  float3 L = normalize(-lightDir);
  float3 H = normalize(L + V);

  // Disney metallic workflow
  float3 albedo = gAlbedo.Sample(gSampler, i.uv).rgb;
  float3 F0 = lerp(0.04, albedo, metallic);

  // Diffuse (Lambert)
  float NdotL = saturate(dot(N, L));
  float3 diff = albedo * NdotL;

  // Specular (Cook-Torrance GGX - simplified)
  float NdotH = saturate(dot(N, H));
  float VdotH = saturate(dot(V, H));
  float D =
      pow(roughness * roughness /
              (max(1e-4, (NdotH * NdotH) * (roughness * roughness - 1) + 1)),
          2);
  float3 F = fresnelSchlick(VdotH, F0);
  float k = pow(roughness + 1, 2) / 8;
  float G =
      NdotL / (NdotL * (1 - k) + k) * (dot(N, V) / (dot(N, V) * (1 - k) + k));

  float3 spec = F * D * G / max(4 * NdotL * dot(N, V), 1e-4);

  float3 color = (diff + spec) * lightColor * lightInt;

  return float4(color, 1);
}
