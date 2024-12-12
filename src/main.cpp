#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

float rgbToLum(uint32_t pixel) {
    float r = ((pixel) & 0xFF) / 255.0;
    float g = ((pixel >> 8) & 0xFF) / 255.0;
    float b = ((pixel >> 16) & 0xFF) / 255.0;
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

int main() {
    int width, height;
    uint32_t *pixels = (uint32_t*)stbi_load("Broadway_tower.jpg", &width, &height, NULL, 4);
    if (!pixels) {
        std::cerr << "Error loading image" << std::endl;
        return -1;
    }

    int iterations = 500; // Number of iterations to remove seams

    for (int iter = 0; iter < iterations; iter++) {
        float *lumPixels = new float[width * height];
        for (size_t i = 0; i < width * height; i++) {
            lumPixels[i] = rgbToLum(pixels[i]);
        }

        // Sobel operator
        static float gx[3][3] = {
            {-1.0, 0.0, 1.0},
            {-2.0, 0.0, 2.0},
            {-1.0, 0.0, 1.0}
        };
        static float gy[3][3] = {
            {1.0, 2.0, 1.0},
            {0.0, 0.0, 0.0},
            {-1.0, -2.0, -1.0}
        };

        float *sobelMag = new float[width * height];
        for (size_t cy = 0; cy < height; cy++) {
            for (size_t cx = 0; cx < width; cx++) {
                float sx = 0.0, sy = 0.0;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int x = cx + dx;
                        int y = cy + dy;
                        float c;
                        if (x < 0 || x >= width || y >= height || y < 0)
                            c = 0.0;
                        else {
                            c = lumPixels[y * width + x];
                        }
                        sx += c * gx[dy + 1][dx + 1];
                        sy += c * gy[dy + 1][dx + 1];
                    }
                }
                sobelMag[cy * width + cx] = sqrtf(sx * sx + sy * sy);
            }
        }

        // Cumulative energy map
        float *energy = new float[width * height];
        for (size_t i = 0; i < width * height; i++) {
            energy[i] = sobelMag[i];
        }

        for (size_t y = 1; y < height; y++) {
            for (size_t x = 0; x < width; x++) {
                float minEnergy = energy[(y - 1) * width + x];
                if (x > 0)
                    minEnergy = std::min(minEnergy, energy[(y - 1) * width + (x - 1)]);
                if (x < width - 1)
                    minEnergy = std::min(minEnergy, energy[(y - 1) * width + (x + 1)]);

                energy[y * width + x] = sobelMag[y * width + x] + minEnergy;
            }
        }

        // Find the seam
        std::vector<int> seam(height);
        float minEnergy = energy[(height - 1) * width];
        int minIndex = 0;
        for (int x = 1; x < width; x++) {
            if (energy[(height - 1) * width + x] < minEnergy) {
                minEnergy = energy[(height - 1) * width + x];
                minIndex = x;
            }
        }
        seam[height - 1] = minIndex;

        for (int y = height - 2; y >= 0; y--) {
            int prevX = seam[y + 1];
            int bestX = prevX;
            if (prevX > 0 && energy[y * width + (prevX - 1)] < energy[y * width + bestX]) {
                bestX = prevX - 1;
            }
            if (prevX < width - 1 && energy[y * width + (prevX + 1)] < energy[y * width + bestX]) {
                bestX = prevX + 1;
            }
            seam[y] = bestX;
        }

        // Allocate memory for the new image with one less column
        uint32_t *newPixels = new uint32_t[(width - 1) * height];

        // Copy pixels from the original image to the new image, skipping the seam pixels
        for (int y = 0; y < height; y++) {
            int seamX = seam[y];
            for (int x = 0; x < width - 1; x++) {
                if (x < seamX) {
                    newPixels[y * (width - 1) + x] = pixels[y * width + x];
                } else {
                    newPixels[y * (width - 1) + x] = pixels[y * width + (x + 1)];
                }
            }
        }

        // Update the image width and pixels
        width--;
        delete[] pixels;
        pixels = newPixels;

        // Clean up
        delete[] lumPixels;
        delete[] sobelMag;
        delete[] energy;
    }

    // Write the new image to a file
    stbi_write_png("output.png", width, height, 4, pixels, width * sizeof(uint32_t));

    // Clean up
    delete[] pixels;

    return 0;
}