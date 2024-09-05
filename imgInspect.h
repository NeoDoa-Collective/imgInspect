// https://github.com/NeoDoa-Collective/imgInspect
//
// The MIT License(MIT)
//
// Copyright(c) 2019 Cedric Guillemet
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
/*
example
Image pickerImage;
ImGui::ImageButton(pickerImage.textureID, ImVec2(pickerImage.mWidth, pickerImage.mHeight));
ImRect rc = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
ImVec2 mouseUVCoord = (io.MousePos - rc.Min) / rc.GetSize();
mouseUVCoord.y = 1.f - mouseUVCoord.y;

if (io.KeyShift && io.MouseDown[0] && mouseUVCoord.x >= 0.f && mouseUVCoord.y >= 0.f)
{
        int width = pickerImage.mWidth;
        int height = pickerImage.mHeight;

        imageInspect(width, height, pickerImage.GetBits(), mouseUVCoord, displayedTextureSize);
}
*/
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces" // Disable -Wmissing-braces
#endif

#pragma once

#include <array>
#include <cmath>
#include <cstdint>

#include "imgui.h"
#include "imgui_internal.h"

namespace ImageInspect {

    inline void histogram(const int width, const int height, const unsigned char* const bits) {

        unsigned int count[4][256] = { 0 };

        const unsigned char* ptrCols = bits;

        ImGui::InvisibleButton("histogram", ImVec2(512, 256));
        for (int l = 0; l < height * width; l++) {
            count[0][*ptrCols++]++;
            count[1][*ptrCols++]++;
            count[2][*ptrCols++]++;
            count[3][*ptrCols++]++;
        }

        unsigned int maxv = count[0][0];
        unsigned int* pCount = &count[0][0];
        for (int i = 0; i < 3 * 256; i++, pCount++) {
            maxv = (maxv > *pCount) ? maxv : *pCount;
        }

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 rmin = ImGui::GetItemRectMin();
        const ImVec2 rmax = ImGui::GetItemRectMax();
        const ImVec2 size = ImGui::GetItemRectSize();
        const float hFactor = size.y / float(maxv);

        for (int i = 0; i <= 10; i++) {
            float ax = rmin.x + (size.x / 10.f) * float(i);
            float ay = rmin.y + (size.y / 10.f) * float(i);
            drawList->AddLine(ImVec2(rmin.x, ay), ImVec2(rmax.x, ay), 0x80808080);
            drawList->AddLine(ImVec2(ax, rmin.y), ImVec2(ax, rmax.y), 0x80808080);
        }

        const float barWidth = (size.x / 256.f);
        for (int j = 0; j < 256; j++) {
            // pixel count << 2 + color index(on 2 bits)
            uint32_t cols[3] = { (count[0][j] << 2), (count[1][j] << 2) + 1, (count[2][j] << 2) + 2 };
            if (cols[0] > cols[1])
                ImSwap(cols[0], cols[1]);
            if (cols[1] > cols[2])
                ImSwap(cols[1], cols[2]);
            if (cols[0] > cols[1])
                ImSwap(cols[0], cols[1]);
            float heights[3];
            uint32_t colors[3];
            uint32_t currentColor = 0xFFFFFFFF;
            for (int i = 0; i < 3; i++) {
                heights[i] = rmax.y - (cols[i] >> 2) * hFactor;
                colors[i] = currentColor;
                currentColor -= 0xFF << ((cols[i] & 3) * 8);
            }

            float currentHeight = rmax.y;
            const float left = rmin.x + barWidth * float(j);
            const float right = left + barWidth;
            for (int i = 0; i < 3; i++) {
                if (heights[i] >= currentHeight) {
                    continue;
                }
                drawList->AddRectFilled(ImVec2(left, currentHeight), ImVec2(right, heights[i]), colors[i]);
                currentHeight = heights[i];
            }
        }
    }

    inline void drawNormal(ImDrawList* draw_list, const ImRect& rc, float x, float y) {
        draw_list->AddCircle(rc.GetCenter(), rc.GetWidth() / 2.f, 0x20AAAAAA, 24, 1.f);
        draw_list->AddCircle(rc.GetCenter(), rc.GetWidth() / 4.f, 0x20AAAAAA, 24, 1.f);
        draw_list->AddLine(
            rc.GetCenter(),
            rc.GetCenter() + ImVec2(x, y) * rc.GetWidth() / 2.f,
            0xFF0000FF,
            2.f
        );
    }

    inline void inspect(const int width, const int height, const unsigned char* const bits, ImVec2 mouseUVCoord, ImVec2 displayedTextureSize, bool drawNormals = false, bool drawHistogram = false) {
        ImGui::BeginTooltip();
        ImGui::BeginGroup();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        static const float zoomRectangleWidth = 200.f;

        // bitmap zoom
        ImGui::InvisibleButton("AnotherInvisibleMan", ImVec2(zoomRectangleWidth, zoomRectangleWidth));
        const ImRect pickRc(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        draw_list->AddRectFilled(pickRc.Min, pickRc.Max, 0xFF000000);
        static int zoomSize = 4;
        const float quadWidth = zoomRectangleWidth / float(zoomSize * 2 + 1);
        const ImVec2 quadSize(quadWidth, quadWidth);
        const int basex = ImClamp(int(mouseUVCoord.x * width), 0, width - 1);
        const int basey = ImClamp(int(mouseUVCoord.y * height), 0, height - 1);
        for (int y = -zoomSize; y <= zoomSize; y++) {
            for (int x = -zoomSize; x <= zoomSize; x++) {
                uint32_t texel;
                if (basex + x < 0 ||
                    basey - y < 0 ||
                    basex + x >= width ||
                    basey - y >= height) {
                    texel = 0;
                } else {
                    texel = ((uint32_t*) bits)[(basey - y) * width + x + basex];
                }
                ImVec2 pos = pickRc.Min + ImVec2(float(x + zoomSize), float(y + zoomSize)) * quadSize;
                draw_list->AddRectFilled(pos, pos + quadSize, texel);
            }
        }
        ImGui::SameLine();

        // center quad
        const ImVec2 pos = pickRc.Min + ImVec2(float(zoomSize), float(zoomSize)) * quadSize;
        draw_list->AddRect(pos, pos + quadSize, 0xFF0000FF, 0.0f, 15, 2.0f);

        if (drawNormals) {
            // normal direction
            ImGui::InvisibleButton("AndOneMore", ImVec2(zoomRectangleWidth, zoomRectangleWidth));
            ImRect normRc(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
            for (int y = -zoomSize; y <= zoomSize; y++) {
                for (int x = -zoomSize; x <= zoomSize; x++) {
                    uint32_t texel;
                    if (basex + x < 0 ||
                        basey - y < 0 ||
                        basex + x >= width ||
                        basey - y >= height) {
                        texel = 0;
                    } else {
                        texel = ((uint32_t*) bits)[(basey - y) * width + x + basex];
                    }
                    const ImVec2 posQuad = normRc.Min + ImVec2(float(x + zoomSize), float(y + zoomSize)) * quadSize;
                    //draw_list->AddRectFilled(pos, pos + quadSize, texel);
                    const float nx = float(texel & 0xFF) / 128.f - 1.f;
                    const float ny = float((texel & 0xFF00) >> 8) / 128.f - 1.f;
                    const ImRect rc(posQuad, posQuad + quadSize);
                    drawNormal(draw_list, rc, nx, ny);
                }
            }
        }

        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();
        uint32_t texel = ((uint32_t*) bits)[basey * width + basex];
        ImVec4 rgbFloat = ImColor(texel); /* [0-1] */
        std::array<int, 4> rgb = { /* [0-255]*/
            static_cast<int>(std::round(rgbFloat.x * 255.0f)),
            static_cast<int>(std::round(rgbFloat.y * 255.0f)),
            static_cast<int>(std::round(rgbFloat.z * 255.0f)),
            static_cast<int>(std::round(rgbFloat.w * 255.0f))
        };
        ImVec4 colHSV;
        ImGui::ColorConvertRGBtoHSV(rgbFloat.x, rgbFloat.y, rgbFloat.z, colHSV.x, colHSV.y, colHSV.z);
        ImGui::Text("U %1.3f V %1.3f", mouseUVCoord.x, mouseUVCoord.y);
        ImGui::Text("Coord %d %d", int(mouseUVCoord.x * width), int(mouseUVCoord.y * height));
        ImGui::Separator();
        ImGui::BeginTable("colors", 4, ImGuiTableFlags_SizingFixedSame);
        ImGui::TableNextColumn(); ImGui::TextColored({ 1.0f, 0.0f, 0.0f, 1.0f }, "R %1.3f", rgbFloat.x);
        ImGui::TableNextColumn(); ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f }, "G %1.3f", rgbFloat.y);
        ImGui::TableNextColumn(); ImGui::TextColored({ 0.2f, 0.2f, 1.0f, 1.0f }, "B %1.3f", rgbFloat.z);
        ImGui::TableNextColumn(); ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1.0f }, "A %1.3f", rgbFloat.w);
        ImGui::TableNextColumn(); ImGui::TextColored({ 1.0f, 0.0f, 0.0f, 1.0f }, "R %d", rgb[0]);
        ImGui::TableNextColumn(); ImGui::TextColored({ 0.0f, 1.0f, 0.0f, 1.0f }, "G %d", rgb[1]);
        ImGui::TableNextColumn(); ImGui::TextColored({ 0.2f, 0.2f, 1.0f, 1.0f }, "B %d", rgb[2]);
        ImGui::TableNextColumn(); ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1.0f }, "A %d", rgb[3]);
        ImGui::EndTable();
        ImGui::Text("HEX: %02x%02x%02x%02x", rgb[0], rgb[1], rgb[2], rgb[3]);
        ImGui::Separator();
        ImGui::Text("H 0x%02x  S 0x%02x  V 0x%02x", int(colHSV.x * 255.f), int(colHSV.y * 255.f), int(colHSV.z * 255.f));
        ImGui::Text("H %1.3f S %1.3f V %1.3f", colHSV.x, colHSV.y, colHSV.z);
        ImGui::Separator();
        ImGui::Text("Size %d, %d", int(displayedTextureSize.x), int(displayedTextureSize.y));
        ImGui::EndGroup();
        if (drawHistogram) { histogram(width, height, bits); }
        ImGui::EndTooltip();
    }
} // namespace ImageInspect

#if defined(__clang__)
#pragma clang diagnostic pop
#endif