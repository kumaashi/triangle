#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include <vector>
#include <string>
#include <algorithm>
#include <map>

#include "win.h"
#include "math.h"

union col32 {
	struct {
		uint8_t b, g, r, a;
	};
	uint32_t rgba;
};

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

#define WIDTH     320
#define HEIGHT    240
#define TILE_DIV  10
#define INDEX_MAX 64
#define BUF_MAX   16384
#define TILE_X_SIZE (WIDTH / TILE_DIV)
#define TILE_Y_SIZE (HEIGHT/ TILE_DIV)
#define TILE_X_MAX  (WIDTH  / TILE_X_SIZE)
#define TILE_Y_MAX  (HEIGHT / TILE_Y_SIZE)
#define _min(a, b) ((a <= b) ? a : b)
#define _max(a, b) ((a >= b) ? a : b)
#define _clamp(x, a, b) _min(_max(x, a), b)
int buf_index = 0;
float vtxbuf[BUF_MAX];
float zbuffer[WIDTH * HEIGHT];
struct tileinfo {
	int index;
	int min_x;
	int min_y;
	int max_x;
	int max_y;
	uint8_t mark;
};
tileinfo tileindex[TILE_X_MAX * TILE_Y_MAX][INDEX_MAX];

static auto edgefunc = [&](const float *a, const float *b, const float *c) {
	return (c[1] - a[1]) * (b[0] - a[0]) - (c[0] - a[0]) * (b[1] - a[1]);
};

void ResetCache() {
	buf_index = 0;
	memset(vtxbuf, 0, sizeof(vtxbuf));
	memset(tileindex, 0xFF, sizeof(tileindex));
}

struct SortInfo {
	float dist;
	int index;
};

int SortCmp(const void *a, const void *b) {
	return ((SortInfo *)a)->dist < ((SortInfo *)b)->dist;
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

	static std::vector<float> vtpos;
	vtpos.clear();
	Random rnd(1.0);
	for(int count = 0 ; count < 145; count++) {
		const float radius = 10.0f;
		float trans_pos[3] = {
				(rnd.getf() * 2.0 - 1.0) * radius,
				(rnd.getf() * 2.0 - 1.0) * radius,
				(rnd.getf() * 2.0 - 1.0) * radius,
		};
		float *pcube = cube_vertex;
		for(int i = 0 ; i < 36; i++) {
			float temp[4] = { pcube[0], pcube[1], pcube[2], 1.0f };
			temp[0] += trans_pos[0];
			temp[1] += trans_pos[1];
			temp[2] += trans_pos[2];
			vtpos.push_back(temp[0]);
			vtpos.push_back(temp[1]);
			vtpos.push_back(temp[2]);
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

	static std::vector<SortInfo> vsortinfo;
	vsortinfo.clear();
	int sindex = 0;
	for(int i = 0 ; i < buf_index; i += 9) {
		float v0[3] = { vtpos[i + 0], vtpos[i + 1], vtpos[i + 2]};
		float v1[3] = { vtpos[i + 3], vtpos[i + 4], vtpos[i + 5]};
		float v2[3] = { vtpos[i + 6], vtpos[i + 7], vtpos[i + 8]};
		float bc[3] = {
			(v0[0] + v1[0] + v2[0]) / 3.0f,
			(v0[1] + v1[1] + v2[1]) / 3.0f,
			(v0[2] + v1[2] + v2[2]) / 3.0f,
		};
		float dist = V_distance(eyepos, bc);
		SortInfo info;
		info.dist  = dist;
		info.index = i;
		vsortinfo.push_back(info);
		sindex++;
	}

	std::sort(vsortinfo.begin(), vsortinfo.end(),
		[](const SortInfo &a, const SortInfo &b) {
			return a.dist < b.dist;
		});


	static auto intersectRect = [](float *r1, float *r2) {
		return
			(r1[0] < r2[2]) &&
			(r1[2] > r2[0]) &&
			(r1[1] < r2[3]) &&
			(r1[3] > r2[1]);
	};
	#pragma omp parallel for
	for(int y = 0; y < TILE_Y_MAX; y++) {
		for(int x = 0; x < TILE_X_MAX; x++) {
			float rect[4];
			rect[0] = x * TILE_X_SIZE;
			rect[1] = y * TILE_Y_SIZE;
			rect[2] = rect[0] + TILE_X_SIZE;
			rect[3] = rect[1] + TILE_Y_SIZE;
			int tile_top = 0;
			for(int i = 0; i < sindex && tile_top < (INDEX_MAX - 1); i++) {
				int vidx = vsortinfo[i].index;
				float *v0 = &vtxbuf[vidx + 0];
				float *v1 = &vtxbuf[vidx + 3];
				float *v2 = &vtxbuf[vidx + 6];
				float trirect[4] = {
					_min(_min(v0[0], v1[0]), v2[0]),
					_min(_min(v0[1], v1[1]), v2[1]),
					_max(_max(v0[0], v1[0]), v2[0]),
					_max(_max(v0[1], v1[1]), v2[1]),
				};
				bool hit = intersectRect(trirect, rect);
				if(hit) {
					float area = edgefunc(v0, v1, v2);
					if(area < 0) {
						continue;
					}
					tileinfo info;
					info.index = vidx;
					info.mark  = 0;
					info.min_x = _max(0,         _max(rect[0] , _min(_min(v0[0], v1[0]), v2[0])));
					info.min_y = _max(0,         _max(rect[1] , _min(_min(v0[1], v1[1]), v2[1])));
					info.max_x = _min(WIDTH - 1, _min(rect[2] , _max(_max(v0[0], v1[0]), v2[0])));
					info.max_y = _min(HEIGHT- 1, _min(rect[3] , _max(_max(v0[1], v1[1]), v2[1])));
					tileindex[x + y * TILE_X_MAX][tile_top++] = info;
				}
			}
		}
	}
}

void ClearScreen(void *bits, int width, int height, uint32_t color) {
	uint32_t *dest = (uint32_t *)bits;
	for(int i = 0 ; i < width * height; i++) {
		dest[i] = color;
		zbuffer[i] = 1.0;
	}
}

Window window("triangle", WIDTH, HEIGHT);
void RenderCache(void *bits, int width, int height) {
	int triangle_count = 0;
	static int counter = 0;
	counter++;
	unsigned long *dest = (unsigned long *)bits;
	#pragma omp parallel for
	for(int ty = 0; ty < TILE_Y_MAX; ty++) {
		for(int tx = 0; tx < TILE_X_MAX; tx++) {
			int rect[4];
			rect[0] = tx * TILE_X_SIZE;
			rect[1] = ty * TILE_Y_SIZE;
			rect[2] = rect[0] + TILE_X_SIZE - 1;
			rect[3] = rect[1] + TILE_Y_SIZE - 1;

			for(int ti = 0; ti < INDEX_MAX; ti++) {
				auto *tile = &tileindex[tx + ty * TILE_X_MAX][ti];
				if(tile->mark != 0) break;
				int index = tile->index;
				float *v0 = &vtxbuf[index + 0];
				float *v1 = &vtxbuf[index + 3];
				float *v2 = &vtxbuf[index + 6];
				
				int min_x = tile->min_x;
				int min_y = tile->min_y;
				int max_x = tile->max_x;
				int max_y = tile->max_y;
				float area = edgefunc(v0, v1, v2);
				if(area < 0) {
					continue;
				}
				area = 1.0 / area;
				triangle_count++;
				for (int y = min_y; y <= max_y; y++) {
					for (int x = min_x; x <= max_x; x++) {
						int raster_index = x + y * width;
						float p[2] = {float(x), float(y)};
						float w0 = edgefunc(v1, v2, p);
						float w1 = edgefunc(v2, v0, p);
						float w2 = edgefunc(v0, v1, p);
						float c0[4] = {1, 0, 0, 1};
						float c1[4] = {0, 1, 0, 1};
						float c2[4] = {0, 0, 1, 1};
						if (w0 >= 0 && w1 >= 0 && w2 >= 0)
						{
							w0 = (w0 * area);
							w1 = (w1 * area);
							w2 = (w2 * area);
							float zd = (w0 * v0[2] + w1 * v1[2] + w2 * v2[2]);
							if(zbuffer[raster_index] > zd) {
								zbuffer[raster_index] = zd;
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
		}
		//window.Present();
		//Sleep(100);
	}
	//printf("draw triangle_count=%d\n", triangle_count);
}

int main(int argc, char **argb) {
	int align  = 256;
	float ratio = float(HEIGHT) / float(WIDTH);
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

