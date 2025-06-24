// ───── Input / Output  ───────────────────────────────
struct VSIn {
  float3 pos : POSITION;
  float3 nrm : NORMAL;
  float2 uv : TEXCOORD0;
};

struct VSOut {
  float4 pos : SV_Position;  // clip-space
  float3 nrm : NORMAL;       // view-space normal
  float3 vpos : TEXCOORD1;   // view-space position
  float2 uv : TEXCOORD0;
};

// ───── 常量バッファ 0 (b0) ───────────────────────────
cbuffer CB_Frame : register(b0) {
  float4x4 uMVP;    // Model × View × Projection
  float4x4 uModel;  // World→Model（normal変換用）
  float4x4 uView;   // World→View
};

// ───── VSMain ───────────────────────────────────────
VSOut VSMain(VSIn v) {
  VSOut o;
  float4 wpos = float4(v.pos, 1.0);
  float3 wnorm = mul((float3x3)uModel, v.nrm);

  o.pos = mul(uMVP, wpos);

  o.vpos = mul(uView, wpos).xyz;  // view-space
  o.nrm = mul((float3x3)uView, wnorm);
  o.uv = v.uv;

  return o;
}
