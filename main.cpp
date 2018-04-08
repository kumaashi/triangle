#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <windows.h>

#include <vector>
#include <string>
#include <map>

typedef float vtype;

union col32 {
	struct {
		uint8_t b, g, r, a;
	};
	uint32_t rgba;
};

float V_length(float *a, float *b) {
	float t[3] = { a[0] - b[0], a[1] - b[1], a[2] - b[2] };
	return sqrt(t[0] * t[0] + t[1] * t[1] + t[2] * t[2]);
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

struct ShowFps {
	DWORD last = timeGetTime();
	DWORD frames = 0;
	char buf[256] = "";
	bool Update(HWND hWnd = nullptr) {
		bool is_update = false;
		DWORD current = 0;
		current = timeGetTime();
		frames++;
		if(1000 <= current - last) {
			float dt = (float)(current - last) / 1000.0f;
			float fps = (float)frames / dt;
			last = current;
			frames = 0;
			sprintf(buf, "%.02f fps", fps);
			printf("FPS=%s\n", buf);
			is_update = true;
			if(hWnd) {
				SetWindowText(hWnd, buf);
			}
		}
		return is_update;
	}
};

class Window {
	static LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) {
		switch( msg ) {
			case WM_SYSCOMMAND:
				switch( ( wParam & 0xFFF0 ) ) {
					case SC_MONITORPOWER:
					case SC_SCREENSAVE:
						return 0;
						break;
				}
				break;
			case WM_CLOSE:
			case WM_DESTROY:
				PostQuitMessage( 0 );
				return 0;
				break;
			case WM_IME_SETCONTEXT:
				lParam &= ~ISC_SHOWUIALL;
				break;
			case WM_KEYDOWN:
				if(wParam == VK_ESCAPE) PostQuitMessage(0);
				break;
		}
		return DefWindowProc( hWnd, msg, wParam, lParam );
	}
	void *pBits = nullptr;
	HWND hWnd = nullptr;
	HINSTANCE hInst = GetModuleHandle(nullptr);
	DWORD style = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME;
	DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	DWORD Width = -1;
	DWORD Height = -1;
	DWORD windowWidth = -1;
	DWORD windowHeight = -1;
	float scale = 2.5f;
	HBITMAP hDIB = NULL;
	HDC hDIBDC = NULL;
	HDC hdc = NULL;
public:
	bool ProcMsg() {
		bool isActive = true;
		MSG msg;
		while(PeekMessage(&msg , 0 , 0 , 0 , PM_REMOVE)) {
			if(msg.message == WM_QUIT) {
				isActive = false;
				break;
			} else {
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
		}
		return isActive;
	}
	~Window() {
		if(hDIB) DeleteObject(hDIB);
	}
	void Present() {
		HDC hdc = GetDC(hWnd);
		StretchBlt(hdc, 0, 0, windowWidth, windowHeight, hDIBDC, 0, 0, Width, Height, SRCCOPY);
		ReleaseDC(hWnd, hdc);
	}
	void *GetBits() {
		return pBits;
	}
	DWORD GetWidth() {
		return Width;
	}
	DWORD GetHeight() {
		return Height;
	}
	HWND GetWindowHandle() {
		return hWnd;
	}
	Window(const char *szAppName, int BmpX, int BmpY) {
		hInst = GetModuleHandle(NULL);
		windowWidth = BmpX * scale;
		windowHeight = BmpY * scale;
		Width = BmpX;
		Height = BmpY;
		RECT rc = {0, 0, windowWidth, windowHeight};
		WNDCLASSEX twc = {
			sizeof( WNDCLASSEX ), CS_CLASSDC | CS_VREDRAW | CS_HREDRAW, MsgProc, 0L, 0L,
			hInst,  LoadIcon(NULL, IDI_APPLICATION), LoadCursor( NULL , IDC_ARROW ),
			( HBRUSH )GetStockObject( BLACK_BRUSH ), NULL, szAppName, NULL
		};
		RegisterClassEx( &twc );
		AdjustWindowRectEx( &rc, style, FALSE, dwExStyle);
		rc.right -= rc.left;
		rc.bottom -= rc.top;
		hWnd = CreateWindowEx(dwExStyle, szAppName, szAppName, style,
				GetSystemMetrics(SM_CXSCREEN) / 2 - (rc.right / 2),
				GetSystemMetrics(SM_CYSCREEN) / 2 - (rc.bottom / 2), 
				rc.right, rc.bottom, NULL, NULL, hInst, NULL );
		ShowWindow(GetWindowHandle(), SW_SHOW);
		UpdateWindow(GetWindowHandle());

		BITMAPINFO bi = {};
		bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
		bi.bmiHeader.biWidth       = BmpX;
		bi.bmiHeader.biHeight      = -BmpY;
		bi.bmiHeader.biPlanes      = 1;
		bi.bmiHeader.biBitCount    = 32;
		bi.bmiHeader.biCompression = BI_RGB;
		hDIB = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, (void **)&pBits, 0, 0);
		hDIBDC = CreateCompatibleDC(NULL);
		SaveDC(hDIBDC);
		SelectObject(hDIBDC, hDIB);
	}
};

#define BUF_MAX 32768
#define _min(a, b) ((a <= b) ? a : b)
#define _max(a, b) ((a >= b) ? a : b)
#define _clamp(x, a, b) _min(_max(x, a), b)
int buf_index = 0;
float vtxbuf[BUF_MAX];
uint32_t *zbuffer = nullptr;

void ResetCache() {
	buf_index = 0;
	memset(vtxbuf, 0, sizeof(vtxbuf));
}

void UpdateCache(void *bits, int width, int height) {
	static float cube_vertex[] = {
		-1.0f,-1.0f,-1.0f,  -1.0f,-1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
		1.0f, 1.0f,-1.0f,  -1.0f,-1.0f,-1.0f, -1.0f, 1.0f,-1.0f,
		1.0f,-1.0f, 1.0f,  -1.0f,-1.0f,-1.0f,  1.0f,-1.0f,-1.0f,
		1.0f, 1.0f,-1.0f,   1.0f,-1.0f,-1.0f, -1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,  -1.0f, 1.0f, 1.0f, -1.0f, 1.0f,-1.0f,
		1.0f,-1.0f, 1.0f,  -1.0f,-1.0f, 1.0f, -1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,  -1.0f,-1.0f, 1.0f,  1.0f,-1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,   1.0f,-1.0f,-1.0f,  1.0f, 1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,   1.0f, 1.0f, 1.0f,  1.0f,-1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,   1.0f, 1.0f,-1.0f, -1.0f, 1.0f,-1.0f,
		1.0f, 1.0f, 1.0f,  -1.0f, 1.0f,-1.0f, -1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 1.0f,  1.0f,-1.0f, 1.0f,
	};
	static float pos_x = 0.0;
	static float pos_y = 1.0;
	static float pos_z = 20.0;
	static float frame = 0.0;
	frame += 0.01;
	float eyepos[3] = {pos_z * cos(frame), pos_y, pos_z * sin(frame)};
	float center[3] = {0, 0, 0};
	float updir[3]  = {0, 1, 0};
	auto faspect = float(width) / float(height);
	auto fnear = 0.5f;
	auto ffar = 100.0f;

	float proj[16];
	float view[16];
	float projview[16];

	M_perspective(proj, 90.0f, faspect, fnear, ffar);
	M_lookat(view, eyepos, center, updir);
	M_MxM_mult(projview, proj, view);

	Random rnd(1.0);
	for(int count = 0 ; count < 256; count++) {
		const float radius = 10.0f;
		float trans_pos[3] = {
				(rnd.getf() * 2.0 - 1.0) * radius,
				(rnd.getf() * 2.0 - 1.0) * radius,
				(rnd.getf() * 2.0 - 1.0) * radius,
		};
		//float trans_pos[3] = { 0, 0, 0, };
		float *pcube = cube_vertex;
		for(int i = 0 ; i < 36; i++) {
			float temp[4] = { pcube[0], pcube[1], pcube[2], 1.0f };
			temp[0] += trans_pos[0];
			temp[1] += trans_pos[1];
			temp[2] += trans_pos[2];
			float ndcpos[4] = {0};

			V_MxV_mult(ndcpos, projview, temp);

			ndcpos[0] /= ndcpos[3];
			ndcpos[1] /= ndcpos[3];
			ndcpos[2] /= ndcpos[3];

			ndcpos[0] = (ndcpos[0] + 1.0) * width  * 0.5;
			ndcpos[1] = (ndcpos[1] + 1.0) * height * 0.5;

			vtxbuf[buf_index + 0] = ndcpos[0];
			vtxbuf[buf_index + 1] = ndcpos[1];
			vtxbuf[buf_index + 2] = ndcpos[2];

			buf_index += 3;
			pcube += 3;
		}
	}
}

void ClearScreen(void *bits, int width, int height, uint32_t color) {
	uint32_t *dest = (uint32_t *)bits;
	for(int i = 0 ; i < width * height; i++) {
		dest[i] = color;
		zbuffer[i] = 32768.0;
	}
}

void RenderCache(void *bits, int width, int height) {
	static int counter = 0;
	counter++;
	unsigned long *dest = (unsigned long *)bits;

	static auto edgefunc = [&](const float *a, const float *b, const float *c) {
		return (c[1] - a[1]) * (b[0] - a[0]) - (c[0] - a[0]) * (b[1] - a[1]);
	};

	int triangle_count = 0;
	//#pragma omp parallel for
	for(int index = 0; index < buf_index / 3; index += 3) {
		int base_index = index * 3;
		float *v0 = &vtxbuf[base_index + 0 * 3];
		float *v1 = &vtxbuf[base_index + 1 * 3];
		float *v2 = &vtxbuf[base_index + 2 * 3];
		float area = edgefunc(v0, v1, v2);
		if(area < 0) {
			continue;
		}
		area = 1.0 / area;
		int min_x = (int)(_max(0.0       , _min(_min(v0[0], v1[0]), v2[0])));
		int min_y = (int)(_max(0.0       , _min(_min(v0[1], v1[1]), v2[1])));
		int max_x = (int)(_min(width  - 1, _max(_max(v0[0], v1[0]), v2[0])));
		int max_y = (int)(_min(height - 1, _max(_max(v0[1], v1[1]), v2[1])));
		v0[2] = 1.0 / v0[2];
		v1[2] = 1.0 / v1[2];
		v2[2] = 1.0 / v2[2];
		triangle_count++;
		for (int y = min_y; y <= max_y; y++) {
			for (int x = min_x; x <= max_x; x++) {
				float p[2] = {x, y};
				float w0 = edgefunc(v1, v2, p);
				float w1 = edgefunc(v2, v0, p);
				float w2 = edgefunc(v0, v1, p);
				float c0[4] = {1, 0, 0, 1};
				float c1[4] = {0, 1, 0, 1};
				float c2[4] = {0, 0, 1, 1};
				if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
					w0 = (w0 * area);
					w1 = (w1 * area);
					w2 = (w2 * area);
					vtype zd = 1.0 / (w0 * v0[2] + w1 * v1[2] + w2 * v2[2]);
					vtype zd_test = zd * 32768.0;
					int raster_index = x + y * width;
					if(zbuffer[raster_index] > zd_test) {
						zbuffer[raster_index] = zd_test;
						w0 *= 256;
						w1 *= 256;
						w2 *= 256;
						int r = zd * w0 * c0[0] + zd * w1 * c1[0] + zd * w2 * c2[0];
						int g = zd * w0 * c0[1] + zd * w1 * c1[1] + zd * w2 * c2[1];
						int b = zd * w0 * c0[2] + zd * w1 * c1[2] + zd * w2 * c2[2];
						col32 c32;
						c32.r = _clamp(r, 0,  0xFF);
						c32.g = _clamp(g, 0,  0xFF);
						c32.b = _clamp(b, 0,  0xFF);
						dest[raster_index] = c32.rgba;
					}
				}
			}
		}
	}
	//printf("draw triangle_count=%d\n", triangle_count);
}

int main(int argc, char **argb) {
	int Width  = 320;
	int Height = 240;
	int align  = 256;
	std::vector<uint32_t> zbuf(Width * Height + align);
	zbuffer = zbuf.data();
	zbuffer = (uint32_t*)( ((size_t)zbuffer + align) & ~(align - 1));
	float ratio = float(Height) / float(Width);
	Window window("triangle", Width, Height);
	while(window.ProcMsg()) {
		static ShowFps showfps;
		ClearScreen(window.GetBits(), window.GetWidth(), window.GetHeight(), 0xFFFFFFFF);
		ResetCache();
		UpdateCache(window.GetBits(), window.GetWidth(), window.GetHeight());
		RenderCache(window.GetBits(), window.GetWidth(), window.GetHeight());
		window.Present();
		showfps.Update(window.GetWindowHandle());
	}
}

