#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <windows.h>

#include <vector>
#include <string>
#include <map>

struct random {
	unsigned long a = 19842356, b = 225254, c = 3456789;
	float scale = 1.0f;
	random(float s = 1.0f) : scale(s) {}
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
public:
	struct Data {
		HWND hWnd;
		int width, height, bpp;
		void *pbits;
	};
	Window(const char *szAppName, int BmpX, int BmpY, float screenratio, void (*update)(Data *)) {
		printf("screenratio = %f\n", screenratio);
		HWND         hWnd        = NULL;
		HINSTANCE    hInst       = GetModuleHandle(NULL);
		DWORD        style       = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME;
		DWORD        dwExStyle   = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		DWORD        Width       = BmpX;
		DWORD        Height      = BmpY;
		POINT        point       = {(GetSystemMetrics( SM_CXSCREEN ) - Width ) / 2, (GetSystemMetrics( SM_CYSCREEN ) - Height) / 2};
		RECT         rc          = {0, 0, Width, Height};
		BITMAPINFO   bi          = {};
		HBITMAP      hDIB        = NULL;
		HDC          hDIBDC      = NULL;
		void       *pBits        = NULL;
		WNDCLASSEX twc = {
			sizeof( WNDCLASSEX ), CS_CLASSDC | CS_VREDRAW | CS_HREDRAW, MsgProc, 0L, 0L,
			hInst,  LoadIcon(NULL, IDI_APPLICATION), LoadCursor( NULL , IDC_ARROW ),
			( HBRUSH )GetStockObject( BLACK_BRUSH ), NULL, szAppName, NULL
		};
		RegisterClassEx( &twc );
		AdjustWindowRectEx( &rc, style, FALSE, dwExStyle);
		hWnd = CreateWindowEx(
			dwExStyle, szAppName, szAppName, style,
			point.x, point.y, rc.right - rc.left, rc.bottom - rc.top,
			NULL, NULL, hInst, NULL );
		ShowWindow(hWnd, SW_SHOW);
		UpdateWindow(hWnd);

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
		timeBeginPeriod(1);
		HDC hdc = GetDC(hWnd);
		Data data;
		data.hWnd   = hWnd;
		data.width  = BmpX;
		data.height = BmpY;
		data.pbits  = pBits;
		data.bpp    = bi.bmiHeader.biBitCount;
		MSG msg;
		BOOL bActive     = TRUE;
		while(bActive) {
			while(PeekMessage(&msg , 0 , 0 , 0 , PM_REMOVE)) {
				if(msg.message == WM_QUIT) {
					bActive = FALSE;
					break;
				} else {
					TranslateMessage( &msg );
					DispatchMessage( &msg );
				}
			}
			if(GetAsyncKeyState(VK_ESCAPE) & 0x8000) bActive = FALSE;
			update(&data);
			StretchBlt(hdc, 0, 0, Width, Height, hDIBDC, 0, 0, BmpX, BmpY, SRCCOPY);
		}
		ReleaseDC(hWnd, hdc);
		timeEndPeriod(1);
		if(hDIB) DeleteObject(hDIB);
	}
};

typedef int32_t vtype;
typedef vtype Vec2[2];
typedef vtype Vec3[3];
typedef vtype Vec4[4];

#define BUF_MAX 1024

int buf_index = 0;
__declspec(align(256)) Vec4 v0buf[BUF_MAX];
__declspec(align(256)) Vec4 v1buf[BUF_MAX];
__declspec(align(256)) Vec4 v2buf[BUF_MAX];
__declspec(align(256)) int  *zbuffer = nullptr;

#define _min(a, b) ((a <= b) ? a : b)
#define _max(a, b) ((a >= b) ? a : b)

void UpdateCache(Window::Data *data) {
	buf_index = 0;

	random rnd(data->height);
	static float phase = 0.0;
	__declspec(align(256)) Vec4 offset;
	phase += 0.001;
	for(int i = 0 ; i < BUF_MAX; i++) {
		offset[0]   = rnd.getf() + (cos(phase) * data->height * 0.15);
		offset[1]   = rnd.getf() + (sin(phase) * data->height * 0.15);
		v0buf[i][0] = vtype(rnd.getf() * 2.0)        * 0.03;//   cos(0.25)  * 5.0;//
		v1buf[i][0] = vtype(rnd.getf() * 2.0  - 1.0) * 0.03;//   cos(1.25)  * 5.0;//
		v2buf[i][0] = vtype(rnd.getf() * 2.0)        * 0.03;//   cos(3.25)  * 5.0;//
		v0buf[i][1] = vtype(rnd.getf() * 2.0  - 1.0) * 0.03;//  -sin(0.25)  * 5.0;//
		v1buf[i][1] = vtype(rnd.getf() * 2.0)        * 0.03;//  -sin(1.25)  * 5.0;//
		v2buf[i][1] = vtype(rnd.getf() * 2.0  - 1.0) * 0.03;//  -sin(3.25)  * 5.0;//

		v0buf[i][0] += offset[0];
		v1buf[i][0] += offset[0];
		v2buf[i][0] += offset[0];
		v0buf[i][1] += offset[1];
		v1buf[i][1] += offset[1];
		v2buf[i][1] += offset[1];
		v0buf[i][2] += offset[2];
		v1buf[i][2] += offset[2];
		v2buf[i][2] += offset[2];
		v0buf[i][3] += offset[3];
		v1buf[i][3] += offset[3];
		v2buf[i][3] += offset[3];
		buf_index++;
	}
}

void RenderCache(Window::Data *data) {
	int width  = data->width;
	int height = data->height;
	int bytes  = data->bpp >> 8;
	unsigned long *dest = (unsigned long *)data->pbits;
	#pragma omp parallel for
	for(int i = 0 ; i < width * height; i++) {
		dest[i] = 0xFF005588;
		zbuffer[i] = 0;
	}

	static auto edgefunc = [&](const Vec4 &a, const Vec4 &b, const Vec4 &c) {
		return (c[0] - a[0]) * (b[1] - a[1]) - (c[1] - a[1]) * (b[0] - a[0]);
	};

	#pragma omp parallel for
	for(int index = 0; index < buf_index; index++) {
		__declspec(align(256)) Vec4 v0 = {v0buf[index][0], v0buf[index][1], 0, 0};
		__declspec(align(256)) Vec4 v1 = {v1buf[index][0], v1buf[index][1], 0, 0};
		__declspec(align(256)) Vec4 v2 = {v2buf[index][0], v2buf[index][1], 0, 0};
		vtype area = edgefunc(v0, v1, v2);
		if(area < 8) {
			continue;
		}
		int min_x = (int)(_max(0.0, _min(_min(v0[0], v1[0]), v2[0]))     );
		int min_y = (int)(_max(0.0, _min(_min(v0[1], v1[1]), v2[1]))     );
		int max_x = (int)(_min((vtype)width, _max(_max(v0[0], v1[0]), v2[0])));
		int max_y = (int)(_min((vtype)height, _max(_max(v0[1], v1[1]), v2[1])));
		for (int y = min_y; y < max_y; y++) {
			for (int x = min_x; x < max_x; x++) {
				int raster_index = x + y * width;
				__declspec(align(256)) Vec4 p = {x, y, x, y};
				vtype w0 = edgefunc(v1, v2, p) * 256;
				vtype w1 = edgefunc(v2, v0, p) * 256;
				vtype w2 = edgefunc(v0, v1, p) * 256;
				if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
					w0 = (w0 / area);
					w1 = (w1 / area);
					w2 = (w2 / area);
					Vec4 c0 = {1, 0, 0, 1};
					Vec4 c1 = {0, 1, 0, 1};
					Vec4 c2 = {0, 0, 1, 1};
					unsigned int r = w0 * c0[0] + w1 * c1[0] + w2 * c2[0];
					unsigned int g = w0 * c0[1] + w1 * c1[1] + w2 * c2[1];
					unsigned int b = w0 * c0[2] + w1 * c1[2] + w2 * c2[2];
					unsigned long color = 0;
					color |= (unsigned char)(r) << 8 * 2;
					color |= (unsigned char)(g) << 8 * 1;
					color |= (unsigned char)(b) << 8 * 0;
					if(zbuffer[raster_index] < index + 1) {
						dest[raster_index] = color;
						zbuffer[raster_index] = index + 1;
					}
				}
			}
		}
	}
}

void Update(Window::Data *data) {
	static ShowFps showfps;
	UpdateCache(data);
	RenderCache(data);
	showfps.Update(data->hWnd);
}

int main(int argc, char **argb) {
	int Width  = 640;
	int Height = 480;
	int align  = 256;
	std::vector<int> zbuf(Width * Height + align);
	zbuffer = zbuf.data();
	zbuffer = (int*)( ((size_t)zbuffer + align) & ~(align - 1));
	float ratio = float(Height) / float(Width);
	Window window("triangle", Width, Height, float(Height) / float(Width), Update);
}

