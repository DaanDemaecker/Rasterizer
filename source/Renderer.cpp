//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Texture.h"
#include "Utils.h"

#include <algorithm>
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

	//calculate aspect ratio
	m_AspectRatio = static_cast<float>(m_Width) / m_Height;

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f }, m_AspectRatio);

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

	for (Mesh& mesh : meshes_world)
	{
		RenderMesh(mesh);
	}

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void dae::Renderer::RenderMesh(Mesh& mesh)
{
	VertexTransformationFunction(mesh.vertices, mesh.vertices_out);

	std::vector<Vector2> vertices_ScreenSpace{};

	for (const auto& vertex : mesh.vertices_out)
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
			RenderTriangle(mesh, mesh.vertices_out, vertices_ScreenSpace, i);
		}
		break;
	case PrimitiveTopology::TriangleStrip:
		for (int i{}; i < mesh.indices.size() - 2; i++)
		{
			RenderTriangle(mesh, mesh.vertices_out, vertices_ScreenSpace, i, (i % 2) == 1);
		}
		break;
	default:
		break;
	}
}

void dae::Renderer::RenderTriangle(const Mesh& mesh, std::vector<Vertex_Out>& vertices_ndc, std::vector<Vector2> vertices_ScreenSpace, int startIdx, bool flipTriangle)
{
	const uint32_t index0{ mesh.indices[startIdx] };
	const uint32_t index1{ mesh.indices[startIdx + 1 + 1 * flipTriangle] };
	const uint32_t index2{ mesh.indices[startIdx + 1 + 1 * !flipTriangle] };

	if (index0 == index1 || index1 == index2 || index2 == index0)return;

	if (m_Camera.isOutsideFrustum(vertices_ndc[index0].position) ||
		m_Camera.isOutsideFrustum(vertices_ndc[index1].position) ||
		m_Camera.isOutsideFrustum(vertices_ndc[index2].position))
		return;


	const Vector2 v0{ vertices_ScreenSpace[index0] };
	const Vector2 v1{ vertices_ScreenSpace[index1] };
	const Vector2 v2{ vertices_ScreenSpace[index2] };

	BoundingBox boundingBox{ GetBoundingBox(v0, v1, v2) };

	const Vector2 edgeV0V1{ v1 - v0 };
	const Vector2 edgeV1V2{ v2 - v1 };
	const Vector2 edgeV2V0{ v0 - v2 };

	const float triangleArea{ Vector2::Cross(edgeV1V2,edgeV2V0) };

	for (int px{ boundingBox.minX }; px < boundingBox.maxX; ++px)
	{
		for (int py{ boundingBox.minY }; py < boundingBox.maxY; ++py)
		{
			const int pixelIdx{ px + py * m_Width };
			if (pixelIdx < 0 || pixelIdx >= m_Width * m_Height)
				continue;

			const Vector2 point{ static_cast<float>(px), static_cast<float>(py) };

			const Vector2 v0ToPoint{ point - v0 };
			const Vector2 v1ToPoint{ point - v1 };
			const Vector2 v2ToPoint{ point - v2 };

			// Calculate cross product from edge to start to point
			const float edge01PointCross{ Vector2::Cross(edgeV0V1, v0ToPoint) };
			const float edge12PointCross{ Vector2::Cross(edgeV1V2, v1ToPoint) };
			const float edge20PointCross{ Vector2::Cross(edgeV2V0, v2ToPoint) };

			if (!(edge01PointCross > 0 && edge12PointCross > 0 && edge20PointCross > 0)) continue;

			const float weightV0{ edge12PointCross / triangleArea };
			const float weightV1{ edge20PointCross / triangleArea };
			const float weightV2{ edge01PointCross / triangleArea };

			const float depthV0{ vertices_ndc[index0].position.w };
			const float depthV1{ vertices_ndc[index1].position.w };
			const float depthV2{ vertices_ndc[index2].position.w };

			float interpolatedZDepth
			{
				1.0f /
						(weightV0 / vertices_ndc[index0].position.w +
						weightV1 / vertices_ndc[index1].position.w +
						weightV2 / vertices_ndc[index2].position.w)
			};

			if (m_pDepthBufferPixels[pixelIdx] < interpolatedZDepth)
				continue;

			m_pDepthBufferPixels[pixelIdx] = interpolatedZDepth;


			ColorRGB finalColor{};
			if (m_FinalColor)
			{
				const float interpolatedWDepth = 1.0f /
					(weightV0 / vertices_ndc[index0].position.w +
						weightV1 / vertices_ndc[index1].position.w +
						weightV2 / vertices_ndc[index2].position.w);

				Vector2 uvInterpolated = ((weightV0 * mesh.vertices[index0].uv / vertices_ndc[index0].position.w) +
					(weightV1 * mesh.vertices[index1].uv / vertices_ndc[index1].position.w) +
					(weightV2 * mesh.vertices[index2].uv / vertices_ndc[index2].position.w)) * interpolatedWDepth;

				finalColor = m_pTexture->Sample(uvInterpolated);
			}
			else
			{
				const float depthColor{ Remap(interpolatedZDepth, 0.985f, 1.0f) };
				finalColor = { depthColor, depthColor , depthColor };
			}

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}


void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex_Out>& vertices_out) const
{
	vertices_out.reserve(vertices_in.size());

	Matrix worldprojectionMatrix{ m_Camera.viewMatrix * m_Camera.projectionMatrix };

	for (auto& vertex : vertices_in)
	{
		// Tranform the vertex using the inversed view matrix
		Vertex_Out outVertex{ {}, vertex.color, vertex.uv, vertex.normal, vertex.tangent };

		outVertex.position = { worldprojectionMatrix.TransformPoint({vertex.position, 1.f})};

		outVertex.position.x /= outVertex.position.w;
		outVertex.position.y /= outVertex.position.w;
		outVertex.position.z /= outVertex.position.w;

		// Add the new vertex to the list of NDC vertices
		vertices_out.emplace_back(outVertex);
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

BoundingBox Renderer::GetBoundingBox(Vector2 v0, Vector2 v1, Vector2 v2)
{
	BoundingBox box{m_Width, m_Height};

	box.UpdateMin(v0);
	box.UpdateMin(v1);
	box.UpdateMin(v2);

	box.UpdateMax(v0);
	box.UpdateMax(v1);
	box.UpdateMax(v2);

	return box;
}
