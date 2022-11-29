#pragma once
#include "Math.h"
#include "vector"

namespace dae
{
	struct Vertex
	{
		Vector3 position{};
		ColorRGB color{colors::White};
		Vector2 uv{};
		Vector3 normal{}; //W4
		Vector3 tangent{}; //W4
		//Vector3 viewDirection{}; //W4
	};

	struct Vertex_Out
	{
		Vector4 position{};
		ColorRGB color{ colors::White };
		Vector2 uv{};
		Vector3 normal{};
		Vector3 tangent{};
		//Vector3 viewDirection{};
	};

	struct BoundingBox
	{
		BoundingBox(int screenWidth, int screenHeight)
		{
			clampX = screenWidth + 1;
			clampY = screenHeight + 1;
		}

		int minX{ INT_MAX };
		int minY{ INT_MAX };

		int maxX{ -INT_MAX };
		int maxY{ -INT_MAX };

		int margin{ 1 };

		int clampX{};
		int clampY{};

		void UpdateMin(Vector2 point)
		{
			minX = Clamp(std::min(minX, int(point.x)) - margin, -1, clampX);
			minY = Clamp(std::min(minY, int(point.y)) - margin, -1, clampY);
		}

		void UpdateMax(Vector2 point)
		{
			maxX = Clamp(std::max(maxX, int(point.x)) + margin, -1, clampX);
			maxY = Clamp(std::max(maxY, int(point.y)) + margin, -1, clampY);
		}
	};

	enum class PrimitiveTopology
	{
		TriangeList,
		TriangleStrip
	};

	struct Mesh
	{
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};
		PrimitiveTopology primitiveTopology{ PrimitiveTopology::TriangleStrip };

		std::vector<Vertex_Out> vertices_out{};
		Matrix worldMatrix{};
	};
}