All the code files are in C++ and contain in-line explainations, formulas, and have a comperhensive, user friendly, I/O

# Probe Landing Site Analyzer Info

Analyzes a PNG heightmap to find the optimal vertical landing zone for a probe, scoring terrain by flatness and distance from hazards.

## How It Works

Pixel brightness = elevation. The algorithm runs three steps:

1. **Slope map** — Sobel operator computes gradient magnitude at every pixel. High gradient = steep = bad.
2. **Distance transform** — BFS floods outward from dark pixels (craters/obstacles) and image borders. Each pixel gets its distance to the nearest hazard.
3. **Score** — `score = w_flat × (1 − slope_norm) + w_dist × dist_norm`. Highest score = optimal site.

Output: coordinates printed to console + annotated PNG with jet colormap heatmap and red crosshair.

## Build

Requires `libpng-dev`. Then:

    g++ -O2 -o lander lander.cpp -lpng

## Usage

    ./lander <input.png> [output.png] [w_flat] [w_dist] [obstacle_threshold]

Defaults: output.png, weights 0.5/0.5, obstacle threshold 80 (pixels darker than this count as hazards).

Example — prioritize flatness:

    ./lander terrain.png result.png 0.8 0.2 80
