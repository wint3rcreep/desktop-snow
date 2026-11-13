#include <metal_stdlib>
using namespace metal;
// ABI: two float4 records, 32 bytes; uniform float4 is 16 bytes.
struct Particle { float4 positionRadiusAlpha; float4 angleKind; };
struct Output { float4 position [[position]]; float2 local; float alpha; float kind; };
vertex Output snowVertex(uint v [[vertex_id]], uint i [[instance_id]],
                         const device Particle *particles [[buffer(0)]],
                         constant float4 &viewport [[buffer(1)]]) {
    const float2 corners[6] = {float2(-1,-1),float2(1,-1),float2(-1,1),float2(-1,1),float2(1,-1),float2(1,1)};
    Particle p=particles[i];
    float2 local=corners[v]*1.6;
    float angle=p.angleKind.x;
    float2 rotated=float2(cos(angle)*local.x-sin(angle)*local.y,sin(angle)*local.x+cos(angle)*local.y);
    float2 point=p.positionRadiusAlpha.xy+rotated*p.positionRadiusAlpha.z;
    Output o;
    o.position=float4(point.x/viewport.x*2-1,1-point.y/viewport.y*2,0,1);
    o.local=local; o.alpha=p.positionRadiusAlpha.w; o.kind=p.angleKind.y;
    return o;
}
fragment float4 snowFragment(Output in [[stage_in]]) {
    float2 q=in.local;
    float radius=length(q);
    float boundary=0.94+0.06*cos(6*atan2(q.y,q.x)); // visible, gently rotating flake silhouette
    if(in.kind>0.5) {
        q.y/=0.8; radius=length(q);
        boundary=0.87+0.08*cos(6*atan2(q.y,q.x))+0.04*cos(3*atan2(q.y,q.x));
    }
    // Derivatives express coverage in actual drawable pixels, including Retina.
    float aa=max(fwidth(radius),0.001);
    float coverage=1-smoothstep(boundary-aa,boundary+aa,radius);
    float skirt=0.12*(1-smoothstep(boundary,boundary+0.35,radius));
    float alpha=clamp(in.alpha*(coverage+skirt*(1-coverage)),0.0,1.0);
    return float4(float3(0.96,0.985,1.0)*alpha,alpha); // premultiplied
}
