//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Texture.h"
#include "Utils.h"

#include <iostream>

using namespace dae;

#define UseTriangleStrip
//#define UseTriangleList

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });

	//calculate aspect ratio
	m_AspectRatio = static_cast<float>(m_Width) / m_Height;

	//temporary texture
	m_pTexture = Texture::LoadFromFile("Resources/uv_grid_2.png");
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;

	if (m_pTexture != nullptr)
	{
		delete m_pTexture;
		m_pTexture = nullptr;
	}
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

#if defined(UseTriangleStrip)
	std::vector<Mesh> meshes_world
	{
		Mesh{
			{
				Vertex{{ -3.f, 3.f, -2.f }, {0, 0, 0}, {0.0f, 0.0f}},
				Vertex{{ 0.f, 3.f, -2.f }, {0, 0, 0}, { 0.5f, 0.0f }},
				Vertex{{ 3.f, 3.f, -2.f }, {0, 0, 0}, { 1.0f, 0.0f }},
				Vertex{{ -3.f, 0.f, -2.f }, {0, 0, 0}, { 0.0f, 0.5f }},
				Vertex{{ 0.f, 0.f, -2.f }, {0, 0, 0}, { 0.5f, 0.5f }},
				Vertex{{ 3.f, 0.f, -2.f }, {0, 0, 0}, { 1.0f, 0.5f }},
				Vertex{{ -3.f, -3.f, -2.f }, {0, 0, 0}, { 0.0f, 1.0f }},
				Vertex{{ 0.f, -3.f, -2.f }, {0, 0, 0}, { 0.5f, 1.0f }},
				Vertex{{ 3.f, -3.f, -2.f }, {0, 0, 0}, { 1.0f, 1.0f }},
		},
				{
			3, 0, 4, 1, 5, 2,
			2, 6,
			6, 3, 7, 4, 8, 5
		},
		
		PrimitiveTopology::TriangleStrip
		}
	};
#elif defined(UseTriangleList)
	std::vector<Mesh> meshes_world
	{
		Mesh{
				{
				Vertex{{ -3.f, 3.f, -2.f }, {0, 0, 0}, {0.0f, 0.0f}},
				Vertex{{ 0.f, 3.f, -2.f }, {0, 0, 0}, { 0.5f, 0.0f }},
				Vertex{{ 3.f, 3.f, -2.f }, {0, 0, 0}, { 1.0f, 0.0f }},
				Vertex{{ -3.f, 0.f, -2.f }, {0, 0, 0}, { 0.0f, 0.5f }},
				Vertex{{ 0.f, 0.f, -2.f }, {0, 0, 0}, { 0.5f, 0.5f }},
				Vertex{{ 3.f, 0.f, -2.f }, {0, 0, 0}, { 1.0f, 0.5f }},
				Vertex{{ -3.f, -3.f, -2.f }, {0, 0, 0}, { 0.0f, 1.0f }},
				Vertex{{ 0.f, -3.f, -2.f }, {0, 0, 0}, { 0.5f, 1.0f }},
				Vertex{{ 3.f, -3.f, -2.f }, {0, 0, 0}, { 1.0f, 1.0f }},
				},
		{
			3, 0, 1,	1, 4, 3,	4, 1, 2,
			2, 5, 4,	6, 3, 4,	4, 7, 6,
			7, 4, 5,	5, 8, 7
			},
		PrimitiveTopology::TriangeList
		}
	};
#endif // PrimitiveTopology

	for (const Mesh& mesh : meshes_world)
	{
		std::vector<Vertex>vertices_ndc;

		VertexTransformationFunction(mesh.vertices, vertices_ndc);

		std::vector<Vector2> vertices_ScreenSpace{};

		for (const auto& vertex : vertices_ndc)
		{
			vertices_ScreenSpace.push_back(
				{
					(vertex.position.x + 1) / 2.0f * m_Width,
					(1.0f - vertex.position.y) / 2.0f * m_Height
				});
		}

		switch (mesh.primitiveTopology)
		{
		case PrimitiveTopology::TriangeList:
			for (int i{}; i < mesh.indices.size(); i += 3)
			{
				RenderTriangle(mesh, vertices_ndc, vertices_ScreenSpace, i);
			}
			break;
		case PrimitiveTopology::TriangleStrip:
			for (int i{}; i < mesh.indices.size() - 2; i++)
			{
				RenderTriangle(mesh, vertices_ndc, vertices_ScreenSpace, i, (i% 2) == 1);
			}
			break;
		default:
			break;
		}
	}

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void dae::Renderer::RenderTriangle(const Mesh& mesh, std::vector<Vertex>& vertices_ndc, std::vector<Vector2> vertices_ScreenSpace, int startIdx, bool flipTriangle)
{
	const uint32_t index0{ mesh.indices[startIdx] };
	const uint32_t index1{ mesh.indices[startIdx + 1 + 1 * flipTriangle] };
	const uint32_t index2{ mesh.indices[startIdx + 1 + 1 * !flipTriangle] };

	if (index0 == index1 || index1 == index2 || index2 == index0)return;


	const Vector2 v0{ vertices_ScreenSpace[index0] };
	const Vector2 v1{ vertices_ScreenSpace[index1] };
	const Vector2 v2{ vertices_ScreenSpace[index2] };

	BoundingBox boundingBox{ GetBoundingBox(v0, v1, v2) };

	const Vector2 edgeV0V1{ v1 - v0 };
	const Vector2 edgeV1V2{ v2 - v1 };
	const Vector2 edgeV2V0{ v0 - v2 };

	const float triangleArea{ Vector2::Cross(edgeV1V2,edgeV2V0) };

	for (int px{boundingBox.minX}; px < boundingBox.maxX; ++px)
	{
		for (int py{boundingBox.minY}; py < boundingBox.maxY; ++py)
		{
			const int pixelIdx{ py * m_Width + px };
			const Vector2 point{ float(px), float(py) };

			const Vector2 pointToEdgeSide0{ point - v0 };
			float weightV0{ Vector2::Cross(edgeV0V1, pointToEdgeSide0) };
			if (weightV0 < 0) continue;

			const Vector2 pointToEdgeSide1{ point - v1 };
			float weightV1{ Vector2::Cross(edgeV1V2, pointToEdgeSide1) };
			if (weightV1 < 0) continue;

			const Vector2 pointToEdgeSide2{ point - v2 };
			float weightV2{ Vector2::Cross(edgeV2V0, pointToEdgeSide2) };
			if (weightV2 < 0) continue;

			weightV0 =  weightV0 / triangleArea;
			weightV1 = weightV1 / triangleArea;
			weightV2 = weightV2 / triangleArea;

			const float depthV0{ vertices_ndc[index0].position.z };
			const float depthV1{ vertices_ndc[index1].position.z };
			const float depthV2{ vertices_ndc[index2].position.z };

			float interpolatedDepth
			{
				1.0f /
						(weightV0 / depthV2 +
						weightV1 / depthV0 +
						weightV2 / depthV1)
			};

			if (m_pDepthBufferPixels[pixelIdx] < interpolatedDepth)
				continue;

			m_pDepthBufferPixels[pixelIdx] = interpolatedDepth;

			Vector2 uvInterpolated = ((weightV0 * mesh.vertices[index0].uv / depthV0) +
									(weightV1 * mesh.vertices[index1].uv /depthV1) +
									(weightV2 * mesh.vertices[index2].uv / depthV2)) * interpolatedDepth;

			ColorRGB finalColor{m_pTexture->Sample(uvInterpolated)};

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}


void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	vertices_out.reserve(vertices_in.size());

	for (Vertex vertex : vertices_in)
	{
		vertex.position = m_Camera.invViewMatrix.TransformPoint(vertex.position);

		vertex.position.x = vertex.position.x/(m_AspectRatio * m_Camera.fov)/ vertex.position.z;
		vertex.position.y = vertex.position.y / (m_Camera.fov)/ vertex.position.z;

		vertices_out.emplace_back(vertex);
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

BoundingBox Renderer::GetBoundingBox(Vector2 v0, Vector2 v1, Vector2 v2)
{
	BoundingBox box{m_Width, m_Height};

	box.Min(v0);
	box.Min(v1);
	box.Min(v2);

	box.Max(v0);
	box.Max(v1);
	box.Max(v2);

	return box;
}
