
#include <png.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <queue>
#include <algorithm>
#include <string>
#include <limits>



struct Image {
    int width, height, channels;
    std::vector<uint8_t> data; 
    uint8_t& at(int x, int y, int c = 0) {
        return data[(y * width + x) * channels + c];
    }
    const uint8_t& at(int x, int y, int c = 0) const {
        return data[(y * width + x) * channels + c];
    }

  
    float gray(int x, int y) const {
        if (channels == 1) return at(x, y, 0);
        return 0.299f * at(x, y, 0)
             + 0.587f * at(x, y, 1)
             + 0.114f * at(x, y, 2);
    }
};

Image load_png(const char* path) {
    Image img{};
    FILE* fp = fopen(path, "rb");
    if (!fp) { fprintf(stderr, "Cannot open %s\n", path); exit(1); }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop   info = png_create_info_struct(png);
    if (setjmp(png_jmpbuf(png))) { fprintf(stderr, "PNG read error\n"); exit(1); }

    png_init_io(png, fp);
    png_read_info(png, info);

    img.width    = png_get_image_width(png, info);
    img.height   = png_get_image_height(png, info);
    int color    = png_get_color_type(png, info);
    int bit      = png_get_bit_depth(png, info);

    if (bit == 16)                         png_set_strip_16(png);
    if (color == PNG_COLOR_TYPE_PALETTE)   png_set_palette_to_rgb(png);
    if (color == PNG_COLOR_TYPE_GRAY && bit < 8) png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
    if (color & PNG_COLOR_MASK_ALPHA)      png_set_strip_alpha(png);
    png_read_update_info(png, info);

    img.channels = (int)png_get_channels(png, info);
    img.data.resize(img.width * img.height * img.channels);

    std::vector<png_bytep> rows(img.height);
    for (int y = 0; y < img.height; ++y)
        rows[y] = img.data.data() + y * img.width * img.channels;

    png_read_image(png, rows.data());
    png_destroy_read_struct(&png, &info, nullptr);
    fclose(fp);
    return img;
}

void save_png(const char* path, const Image& img) {
    FILE* fp = fopen(path, "wb");
    if (!fp) { fprintf(stderr, "Cannot write %s\n", path); exit(1); }

    png_structp png  = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop   info = png_create_info_struct(png);
    if (setjmp(png_jmpbuf(png))) { fprintf(stderr, "PNG write error\n"); exit(1); }

    png_init_io(png, fp);
    png_set_IHDR(png, info, img.width, img.height, 8,
                 img.channels == 1 ? PNG_COLOR_TYPE_GRAY : PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    std::vector<png_bytep> rows(img.height);
    for (int y = 0; y < img.height; ++y)
        rows[y] = const_cast<uint8_t*>(img.data.data()) + y * img.width * img.channels;

    png_write_image(png, rows.data());
    png_write_end(png, nullptr);
    png_destroy_write_struct(&png, &info);
    fclose(fp);
}

// ─────────────────────────────────────────────//

std::vector<float> compute_slope(const Image& img) {
    int W = img.width, H = img.height;
    std::vector<float> slope(W * H, 0.f);

    for (int y = 1; y < H - 1; ++y) {
        for (int x = 1; x < W - 1; ++x) {
            float gx =
                -img.gray(x-1, y-1) + img.gray(x+1, y-1)
                - 2*img.gray(x-1, y) + 2*img.gray(x+1, y)
                - img.gray(x-1, y+1) + img.gray(x+1, y+1);
            float gy =
                -img.gray(x-1, y-1) - 2*img.gray(x, y-1) - img.gray(x+1, y-1)
                + img.gray(x-1, y+1) + 2*img.gray(x, y+1) + img.gray(x+1, y+1);
            slope[y * W + x] = std::sqrt(gx*gx + gy*gy);
        }
    }
    return slope;
}

std::vector<float> compute_distance(const Image& img, float obstacle_thresh) {
    int W = img.width, H = img.height;
    std::vector<float> dist(W * H, std::numeric_limits<float>::max());
    std::queue<int> q;

    auto enqueue = [&](int idx) {
        if (dist[idx] == std::numeric_limits<float>::max()) {
            dist[idx] = 0.f;
            q.push(idx);
        }
    };

    for (int x = 0; x < W; ++x) { enqueue(0 * W + x); enqueue((H-1) * W + x); }
    for (int y = 0; y < H; ++y) { enqueue(y * W + 0); enqueue(y * W + (W-1)); }

   
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
            if (img.gray(x, y) < obstacle_thresh)
                enqueue(y * W + x);

    const int dx[] = {1,-1,0,0};
    const int dy[] = {0,0,1,-1};
    while (!q.empty()) {
        int idx = q.front(); q.pop();
        int cx = idx % W, cy = idx / W;
        for (int d = 0; d < 4; ++d) {
            int nx = cx + dx[d], ny = cy + dy[d];
            if (nx < 0 || nx >= W || ny < 0 || ny >= H) continue;
            int nidx = ny * W + nx;
            if (dist[nidx] == std::numeric_limits<float>::max()) {
                dist[nidx] = dist[idx] + 1.f;
                q.push(nidx);
            }
        }
    }
    return dist;
}

std::vector<float> normalize(const std::vector<float>& v) {
    float mn = *std::min_element(v.begin(), v.end());
    float mx = *std::max_element(v.begin(), v.end());
    float range = mx - mn;
    std::vector<float> out(v.size());
    if (range < 1e-6f) return out; // all same
    for (size_t i = 0; i < v.size(); ++i)
        out[i] = (v[i] - mn) / range;
    return out;
}

//──────────────────────────────────────//

void jet(float t, uint8_t& r, uint8_t& g, uint8_t& b) {
    t = std::max(0.f, std::min(1.f, t));
    r = (uint8_t)(255 * std::min(1.f, std::max(0.f, 1.5f - std::abs(4*t - 3))));
    g = (uint8_t)(255 * std::min(1.f, std::max(0.f, 1.5f - std::abs(4*t - 2))));
    b = (uint8_t)(255 * std::min(1.f, std::max(0.f, 1.5f - std::abs(4*t - 1))));
}

void draw_crosshair(Image& img, int cx, int cy, int radius, uint8_t r, uint8_t g, uint8_t b) {
    int W = img.width, H = img.height;
    // horizontal & vertical lines
    for (int d = -radius; d <= radius; ++d) {
        auto put = [&](int x, int y) {
            if (x < 0 || x >= W || y < 0 || y >= H) return;
            img.at(x, y, 0) = r; img.at(x, y, 1) = g; img.at(x, y, 2) = b;
        };
        put(cx + d, cy);
        put(cx, cy + d);
    }
   
    for (int angle = 0; angle < 360; ++angle) {
        float rad = angle * 3.14159f / 180.f;
        int px = cx + (int)(radius * std::cos(rad));
        int py = cy + (int)(radius * std::sin(rad));
        if (px < 0 || px >= W || py < 0 || py >= H) continue;
        img.at(px, py, 0) = r; img.at(px, py, 1) = g; img.at(px, py, 2) = b;
    }
}

//──────────────────────────────────────────//

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <input.png> [output.png] [w_flat] [w_dist] [obstacle_threshold]\n", argv[0]);
        printf("  w_flat             weight for flatness  (default 0.5)\n");
        printf("  w_dist             weight for distance  (default 0.5)\n");
        printf("  obstacle_threshold pixel brightness below which is an obstacle (default 80)\n");
        return 1;
    }

    const char* input_path  = argv[1];
    const char* output_path = argc >= 3 ? argv[2] : "output.png";
    float w_flat  = argc >= 4 ? atof(argv[3]) : 0.5f;
    float w_dist  = argc >= 5 ? atof(argv[4]) : 0.5f;
    float obs_thr = argc >= 6 ? atof(argv[5]) : 80.f;

    printf("═══════════════════════════════════════\n");
    printf("  Probe Landing Site Analyzer\n");
    printf("═══════════════════════════════════════\n");
    printf("  Input     : %s\n", input_path);
    printf("  Output    : %s\n", output_path);
    printf("  w_flat    : %.2f\n", w_flat);
    printf("  w_dist    : %.2f\n", w_dist);
    printf("  Obstacle  : brightness < %.0f\n", obs_thr);
    printf("───────────────────────────────────────\n");

    // 1. Load
    Image img = load_png(input_path);
    printf("  Image     : %d × %d px, %d ch\n", img.width, img.height, img.channels);

    // 2. Slope (Sobel)
    printf("  Computing slope map (Sobel)...\n");
    std::vector<float> slope_raw = compute_slope(img);
    std::vector<float> slope_n   = normalize(slope_raw);   // 0=flat, 1=steep

    // 3. Distance transform (BFS)
    printf("  Computing distance transform (BFS)...\n");
    std::vector<float> dist_raw = compute_distance(img, obs_thr);
    std::vector<float> dist_n   = normalize(dist_raw);     // 0=near hazard, 1=far

    // 4. Score map
    printf("  Computing score map...\n");
    int W = img.width, H = img.height;
    std::vector<float> score(W * H);
    for (int i = 0; i < W * H; ++i)
        score[i] = w_flat * (1.f - slope_n[i]) + w_dist * dist_n[i];

    // 5. Find best pixel
    int best_idx = (int)(std::max_element(score.begin(), score.end()) - score.begin());
    int best_x   = best_idx % W;
    int best_y   = best_idx / W;
    float best_s = score[best_idx];

    printf("───────────────────────────────────────\n");
    printf("  ✓ Optimal landing site found:\n");
    printf("    X         : %d px\n", best_x);
    printf("    Y         : %d px\n", best_y);
    printf("    Score     : %.4f / %.4f\n", best_s, w_flat + w_dist);
    printf("    Slope     : %.4f (norm)\n", slope_n[best_idx]);
    printf("    Dist      : %.1f px (raw)\n", dist_raw[best_idx]);
    printf("═══════════════════════════════════════\n");

    // 6. RGB output
    printf("  Rendering annotated output...\n");
    Image out{};
    out.width    = W;
    out.height   = H;
    out.channels = 3;
    out.data.resize(W * H * 3);

    std::vector<float> score_n = normalize(score);

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int i = y * W + x;
            float g = img.gray(x, y) / 255.f;
            uint8_t jr, jg, jb;
            jet(score_n[i], jr, jg, jb);
            out.at(x, y, 0) = (uint8_t)(g * 0.6f * 255 + jr * 0.4f);
            out.at(x, y, 1) = (uint8_t)(g * 0.6f * 255 + jg * 0.4f);
            out.at(x, y, 2) = (uint8_t)(g * 0.6f * 255 + jb * 0.4f);
        }
    }
www
    draw_crosshair(out, best_x, best_y, 20, 255, 30, 30);

    save_png(output_path, out);
    printf("  Saved to  : %s\n", output_path);
    printf("═══════════════════════════════════════\n");

    return 0;
}