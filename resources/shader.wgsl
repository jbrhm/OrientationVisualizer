struct VertexInput {
	@location(0) position: vec3f,
	@location(1) normal: vec3f, // new attribute
	@location(2) color: vec3f,
};

struct VertexOutput {
	@builtin(position) position: vec4f,
	@location(0) color: vec3f,
	@location(1) normal: vec3f, // <--- Add a normal output
};

/**
 * A structure holding the value of our uniforms
 */
struct MyUniforms {
    projectionMatrix: mat4x4f,
    viewMatrix: mat4x4f,
    modelMatrix: mat4x4f,
	rotation: mat4x4f,
    color: vec4f,
    zScalar: f32,
};

// Instead of the simple uTime variable, our uniform variable is a struct
@group(0) @binding(0) var<uniform> uMyUniforms: MyUniforms;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
	var pos: vec3f;
	pos = in.position;
	pos.z = pos.z * uMyUniforms.zScalar;
	var out: VertexOutput;
	out.position = uMyUniforms.projectionMatrix * uMyUniforms.viewMatrix * uMyUniforms.modelMatrix * uMyUniforms.rotation * vec4f(pos, 1.0);
	// Forward the normal
    out.normal = (uMyUniforms.modelMatrix * uMyUniforms.rotation * vec4f(in.normal, 0.0)).xyz;
	out.color = in.color;
	return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
	let normal = normalize(in.normal);

	let scalar = 1.9;

	let lightColor1 = scalar * vec3f(1.0, 1.0, 1.0);
	let lightColor2 = 0.3 * scalar * vec3f(0.3, 0.3, 0.3);
	let lightDirection1 = vec3f(0.25, 0.45, 0.05);
	let lightDirection2 = vec3f(-0.25, -0.5, 1.5);
	let shading1 = max(0.0, dot(lightDirection1, normal));
	let shading2 = max(0.0, dot(lightDirection2, normal));
	let shading = shading1 * lightColor1 + shading2 * lightColor2;
	let color = in.color * shading;

	// Gamma-correction
	let corrected_color = pow(color, vec3f(2.2));
	return vec4f(corrected_color, uMyUniforms.color.a);
}
