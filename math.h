#pragma once

float V_length(float *t) {
	return sqrt(t[0] * t[0] + t[1] * t[1] + t[2] * t[2]);
}

float V_distance(float *a, float *b) {
	float t[3] = { a[0] - b[0], a[1] - b[1], a[2] - b[2] };
	return V_length(t);
}

void M_MxM_mult(float *dest, float *src1, float *src2) {
	for(int y = 0; y < 4; y++) {
		for(int x = 0; x < 4; x++) {
			dest[y * 4 + x] =
				src2[y * 4 + 0] * src1[x +  0] +
				src2[y * 4 + 1] * src1[x +  4] +
				src2[y * 4 + 2] * src1[x +  8] +
				src2[y * 4 + 3] * src1[x + 12];
		}
	}
}

void V_MxV_mult(float *out, float *M, float *in) {
	out[0] = in[0] * M[4 * 0 + 0] + in[1] * M[4 * 1 + 0] + in[2] * M[4 * 2 + 0] + in[3] * M[4 * 3 + 0];
	out[1] = in[0] * M[4 * 0 + 1] + in[1] * M[4 * 1 + 1] + in[2] * M[4 * 2 + 1] + in[3] * M[4 * 3 + 1];
	out[2] = in[0] * M[4 * 0 + 2] + in[1] * M[4 * 1 + 2] + in[2] * M[4 * 2 + 2] + in[3] * M[4 * 3 + 2];
	out[3] = in[0] * M[4 * 0 + 3] + in[1] * M[4 * 1 + 3] + in[2] * M[4 * 2 + 3] + in[3] * M[4 * 3 + 3];
}


void V_normalize(float* v) {
	float m = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	if(m > 0.0f) {
		m = 1.0f / m;
	} else {
		m = 0.0f;
	}
	v[0] *= m;
	v[1] *= m;
	v[2] *= m;
}

void V_cross(float* dst, float* src1, float* src2) {
	dst[0] = src1[1] * src2[2] - src1[2] * src2[1];
	dst[1] = src1[2] * src2[0] - src1[0] * src2[2];
	dst[2] = src1[0] * src2[1] - src1[1] * src2[0];
}

void M_lookat(float *dest, float *eye, float *center, float *up) {
	float forward[3], side[3], upres[3];
	for(int i = 0; i < 3; i++) {
		forward[i] = center[i] - eye[i];
		upres[i] = up[i];
	}
	V_normalize(forward);
	V_cross(side, forward, up);
	V_normalize(side);
	V_cross(up, side, forward);
	float m[]= {
		side[0], upres[0], -forward[0], 0.0,
		side[1], upres[1], -forward[1], 0.0,
		side[2], upres[2], -forward[2], 0.0,
		0.0, 0.0, 0.0, 1.0
	};
	float mov[] = {
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		-eye[0], -eye[1], -eye[2], 1
	};
	M_MxM_mult(dest, m, mov);
}

void M_perspective(float *dest, float fovy,  float aspect, float znear, float zfar) {
	const float PI = 3.14159265358979f;
	float radian = 2.0f * PI * fovy / 360.0f;
	float t = (float)(1.0 / tan(radian / 2));
	memset(dest, 0, sizeof(float) * 16);
	dest[0] = t / aspect;
	dest[5] = t;
	dest[10] = (zfar + znear) / (znear - zfar);
	dest[11] = -1.0f;
	dest[14] = (2 * zfar * znear) / (znear - zfar);
}

struct Random {
	unsigned long a = 19842356, b = 225254, c = 3456789;
	float scale = 1.0f;
	Random(float s = 1.0f) : scale(s) {}
	unsigned long get() {
		a += b;
		b += c;
		c += a;
		return (a >> 16);
	}
	float getf() {
		return scale * float(get()) / float(0xFFFF);
	}
};

