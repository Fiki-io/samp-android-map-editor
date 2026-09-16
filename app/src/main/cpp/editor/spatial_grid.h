#pragma once

#include "../core/types.h"
#include <vector>
#include <unordered_map>
#include <cmath>
#include <algorithm>

namespace samp_editor {

template<typename T>
class SpatialGrid2D {
public:
    SpatialGrid2D(float worldMin = -3000.0f, float worldMax = 3000.0f, float cellSize = 250.0f)
        : m_WorldMin(worldMin), m_WorldMax(worldMax), m_CellSize(cellSize) {
        m_GridDim = static_cast<int>(std::ceil((worldMax - worldMin) / cellSize));
        m_Cells.resize(m_GridDim * m_GridDim);
    }

    void Clear() {
        for (auto& cell : m_Cells) {
            cell.clear();
        }
    }

    void Insert(const Vec3& pos, const T& item) {
        int cx = GetCellCoord(pos.x);
        int cy = GetCellCoord(pos.y);
        int idx = cy * m_GridDim + cx;
        m_Cells[idx].push_back(item);
    }

    void QueryRadius(const Vec3& center, float radius, std::vector<T>& outItems) const {
        int minX = GetCellCoord(center.x - radius);
        int maxX = GetCellCoord(center.x + radius);
        int minY = GetCellCoord(center.y - radius);
        int maxY = GetCellCoord(center.y + radius);

        float rSq = radius * radius;

        for (int cy = minY; cy <= maxY; ++cy) {
            for (int cx = minX; cx <= maxX; ++cx) {
                int idx = cy * m_GridDim + cx;
                const auto& cell = m_Cells[idx];
                outItems.insert(outItems.end(), cell.begin(), cell.end());
            }
        }
    }

private:
    int GetCellCoord(float val) const {
        int c = static_cast<int>((val - m_WorldMin) / m_CellSize);
        return std::clamp(c, 0, m_GridDim - 1);
    }

    float m_WorldMin;
    float m_WorldMax;
    float m_CellSize;
    int m_GridDim;
    std::vector<std::vector<T>> m_Cells;
};

} // namespace samp_editor
