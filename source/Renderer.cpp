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
	m_pTexture = Texture::LoadFromFile("Resources/tuktuk.png");

	InitializeMesh();
}

void dae::Renderer::InitializeMesh()
{
	Utils::ParseOBJ("Resources/tuktuk.obj", m_Mesh.vertices, m_Mesh.indices);

	m_Mesh.primitiveTopology = PrimitiveTopology::TriangeList;

	const Vector3 position{ m_Camera.origin + Vector3{0.0f, -3.0f, 15.0f} };
	const Vector3 rotation{};
	const Vector3 scale{ Vector3{0.5f, 0.5f, 0.5f} };
	m_Mesh.worldMatrix = Matrix::CreateScale(scale) * Matrix::CreateRotation(rotation) * Matrix::CreateTranslation(position);
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

	const float rotationSpeed = 50.f * TO_RADIANS;

	m_Mesh.worldMatrix = Matrix::CreateRotationY(rotationSpeed * pTimer->GetElapsed()) * m_Mesh.worldMatrix;
}

void Renderer::Render()
{
	//@START
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	RenderMesh(m_Mesh);

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void dae::Renderer::RenderMesh(Mesh& mesh)
{
	VertexTransformationFunction(mesh);

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
				RenderTriangle(mesh, vertices_ScreenSpace, i);
		}
		break;
	case PrimitiveTopology::TriangleStrip:
		for (int i{}; i < mesh.indices.size() - 2; i++)
		{
			RenderTriangle(mesh, vertices_ScreenSpace, i, (i%2) == 1);
		}
		break;
	default:
		break;
	}
}

void dae::Renderer::RenderTriangle(const Mesh& mesh, std::vector<Vector2> vertices_ScreenSpace, int startIdx, bool flipTriangle)
{
	const uint32_t index0{ mesh.indices[startIdx] };
	const uint32_t index1{ mesh.indices[startIdx + 1 + 1 * flipTriangle] };
	const uint32_t index2{ mesh.indices[startIdx + 1 + 1 * !flipTriangle] };

	if (index0 == index1 || index1 == index2 || index2 == index0)return;

	const Vector2 v0{ vertices_ScreenSpace[index0]};
	const Vector2 v1{ vertices_ScreenSpace[index1]};
	const Vector2 v2{ vertices_ScreenSpace[index2]};

	const Vertex_Out ndc0{ mesh.vertices_out[index0] };
	const Vertex_Out ndc1{ mesh.vertices_out[index1] };
	const Vertex_Out ndc2{ mesh.vertices_out[index2] };


	if (m_Camera.isOutsideFrustum(ndc0.position) ||
		m_Camera.isOutsideFrustum(ndc1.position) ||
		m_Camera.isOutsideFrustum(ndc2.position))
	{
		return;
	}

	BoundingBox boundingBox{ GetBoundingBox(v0, v1, v2) };

	const Vector2 edgeV0V1{ v1 - v0 };
	const Vector2 edgeV1V2{ v2 - v1 };
	const Vector2 edgeV2V0{ v0 - v2 };

	const float triangleArea{ Vector2::Cross(edgeV1V2,edgeV2V0) };

	ColorRGB finalColor{};

	for (int px{ boundingBox.minX }; px < boundingBox.maxX; ++px)
	{
		for (int py{ boundingBox.minY }; py < boundingBox.maxY; ++py)
		{
			if (!m_RenderBoundingBox)
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

				float interpolatedZDepth
				{
					1.0f /
							(weightV0 / ndc0.position.z +
							weightV1 / ndc1.position.z +
							weightV2 / ndc2.position.z)
				};

				if (interpolatedZDepth < 0.0f || interpolatedZDepth > 1.0f ||
					m_pDepthBufferPixels[pixelIdx] < interpolatedZDepth)
					continue;

				m_pDepthBufferPixels[pixelIdx] = interpolatedZDepth;


				
				if (m_RenderFinalColor)
				{
					const float interpolatedWDepth = 1.0f /
						(weightV0 / ndc0.position.w +
							weightV1 / ndc1.position.w +
							weightV2 / ndc2.position.w);

					Vector2 uvInterpolated = ((weightV0 * ndc0.uv / ndc0.position.w) +
						(weightV1 * ndc1.uv / ndc1.position.w) +
						(weightV2 * ndc2.uv / ndc2.position.w)) * interpolatedWDepth;

					finalColor = m_pTexture->Sample(uvInterpolated);
				}
				else
				{
					const float depthColor{ Remap(interpolatedZDepth, 0.985f, 1.0f) };
					finalColor = { depthColor, depthColor , depthColor };
				}
			}
			else
			{
				finalColor = { 1, 0, 0 };
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


void Renderer::VertexTransformationFunction(Mesh& mesh) const
{
	mesh.vertices_out.clear();
	mesh.vertices_out.reserve(mesh.vertices.size());

	Matrix worldprojectionMatrix{ mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix };

	for (auto& vertex : mesh.vertices)
	{
		// Tranform the vertex using the inversed view matrix
		Vertex_Out outVertex{ {}, vertex.color, vertex.uv, vertex.normal, vertex.tangent };

		outVertex.position = { worldprojectionMatrix.TransformPoint({vertex.position, 1.f})};

		outVertex.position.x /= outVertex.position.w;
		outVertex.position.y /= outVertex.position.w;
		outVertex.position.z /= outVertex.position.w;

		// Add the new vertex to the list of NDC vertices
		mesh.vertices_out.emplace_back(outVertex);
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
