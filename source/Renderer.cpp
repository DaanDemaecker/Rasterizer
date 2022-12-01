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
	m_Camera.Initialize(45.f, { .0f,.0f, 0.f }, m_AspectRatio);

	//temporary texture
	m_pDiffuseTexture = Texture::LoadFromFile("Resources/vehicle_diffuse.png");
	m_pGlossTexture = Texture::LoadFromFile("Resources/vehicle_gloss.png");
	m_pSpecularTexture = Texture::LoadFromFile("Resources/vehicle_specular.png");
	m_pNormalMap = Texture::LoadFromFile("Resources/vehicle_normal.png");

	InitializeMesh();
}

void dae::Renderer::InitializeMesh()
{
	Utils::ParseOBJ("Resources/vehicle.obj", m_Mesh.vertices, m_Mesh.indices);

	m_Mesh.primitiveTopology = PrimitiveTopology::TriangeList;

	const Vector3 position{ Vector3{0.0f, 0.0f, 50.0f} };
	m_Mesh.worldMatrix = Matrix::CreateTranslation(position);
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;


	delete m_pDiffuseTexture;
	m_pDiffuseTexture = nullptr;

	delete m_pGlossTexture;
	m_pGlossTexture = nullptr;

	delete m_pSpecularTexture;
	m_pSpecularTexture = nullptr;

	delete m_pNormalMap;
	m_pNormalMap = nullptr;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	const float rotationSpeed = 1.f;

	if(m_RotationEnabled)
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

	const Vector2 v0{ vertices_ScreenSpace[index0] };
	const Vector2 v1{ vertices_ScreenSpace[index1] };
	const Vector2 v2{ vertices_ScreenSpace[index2] };

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

	const float inverseTriangleArea{ 1.f / Vector2::Cross(edgeV1V2,edgeV2V0) };

	ColorRGB finalColor{};

	for (int px{ boundingBox.minX }; px < boundingBox.maxX; ++px)
	{
		for (int py{ boundingBox.minY }; py < boundingBox.maxY; ++py)
		{
			const int pixelIdx{ px + py * m_Width };

			if (m_RenderBoundingBox)
			{
				finalColor = ColorRGB{ 1, 1, 1 };

				m_pBackBufferPixels[pixelIdx] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));

				continue;
			}

			const Vector2 point{ static_cast<float>(px), static_cast<float>(py) };

			const Vector2 v0ToPoint{ point - v0 };
			const Vector2 v1ToPoint{ point - v1 };
			const Vector2 v2ToPoint{ point - v2 };

			// Calculate cross product from edge to start to point
			const float edge01PointCross{ Vector2::Cross(edgeV0V1, v0ToPoint) };
			const float edge12PointCross{ Vector2::Cross(edgeV1V2, v1ToPoint) };
			const float edge20PointCross{ Vector2::Cross(edgeV2V0, v2ToPoint) };

			if (!(edge01PointCross > 0 && edge12PointCross > 0 && edge20PointCross > 0)) continue;

			const float weightV0{ edge12PointCross * inverseTriangleArea };
			const float weightV1{ edge20PointCross * inverseTriangleArea };
			const float weightV2{ edge01PointCross * inverseTriangleArea };

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

				Pixel_Out pixelOut{ Vector4{float(px), float(py), interpolatedZDepth, interpolatedWDepth} };

				pixelOut.uv = ((weightV0 * ndc0.uv / ndc0.position.w) +
					(weightV1 * ndc1.uv / ndc1.position.w) +
					(weightV2 * ndc2.uv / ndc2.position.w)) * interpolatedWDepth;

				pixelOut.normal =
				{
					(((weightV0 * ndc0.normal / ndc0.position.w) +
					(weightV1 * ndc1.normal / ndc1.position.w) +
					(weightV2 * ndc2.normal / ndc2.position.w)) * interpolatedWDepth)
				};
				pixelOut.normal.Normalize();

				pixelOut.tangent =
				{
					(((weightV0 * ndc0.tangent / ndc0.position.w) +
					(weightV1 * ndc1.tangent / ndc1.position.w) +
					(weightV2 * ndc2.tangent / ndc2.position.w)) * interpolatedWDepth)
				};

				pixelOut.viewDirection =
				{
					(((weightV0 * ndc0.viewDirection / ndc0.position.w) +
					(weightV1 * ndc1.viewDirection / ndc1.position.w) +
					(weightV2 * ndc2.viewDirection / ndc2.position.w)) * interpolatedWDepth)
				};

				finalColor = PixelShading(pixelOut);
			}
			else
			{
				const float depthColor{ Remap(interpolatedZDepth, 0.997f, 1.0f) };

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


void Renderer::VertexTransformationFunction(Mesh& mesh) const
{
	mesh.vertices_out.clear();
	mesh.vertices_out.reserve(mesh.vertices.size());

	Matrix worldprojectionMatrix{ mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix };

	for (auto& vertex : mesh.vertices)
	{
		// Tranform the vertex using the inversed view matrix
		Vertex_Out outVertex{worldprojectionMatrix.TransformPoint({vertex.position, 1.f})
			, vertex.color, vertex.uv,
			mesh.worldMatrix.TransformVector(vertex.normal).Normalized(),
			mesh.worldMatrix.TransformVector(vertex.tangent).Normalized(),
			worldprojectionMatrix.TransformPoint(vertex.viewDirection).Normalized()
		};

		outVertex.position.x /= outVertex.position.w;
		outVertex.position.y /= outVertex.position.w;
		outVertex.position.z /= outVertex.position.w;

		// Add the new vertex to the list of NDC vertices
		mesh.vertices_out.emplace_back(outVertex);
	}
}

ColorRGB dae::Renderer::PixelShading(Pixel_Out& pixel)
{
	Vector3 sampledNormal{pixel.normal};
	if (m_UseNormalMap)
	{
		const Vector3 binormal{ Vector3::Cross(pixel.normal, pixel.tangent) };
		const Matrix tangentSpaceAxis{ pixel.tangent, binormal.Normalized(), pixel.normal, {0.f, 0.f, 0.f} };

		const ColorRGB normalColor{ m_pNormalMap->Sample(pixel.uv) };
		sampledNormal = { normalColor.r, normalColor.g, normalColor.b };

		sampledNormal = 2 * sampledNormal - Vector3{ 1.f, 1.f, 1.f };
		sampledNormal = tangentSpaceAxis.TransformVector(sampledNormal);
	}
	sampledNormal.Normalize();

	const float observedArea{std::max(0.f, Vector3::Dot(sampledNormal, -m_LightDirection))};
	const float kd{ .5f };

	ColorRGB finalColor{};

	switch (m_ShadingMode)
	{
	case dae::Renderer::ShadingMode::ObservedArea:
		finalColor = ColorRGB{ 1, 1, 1 } * observedArea;
		break;
	case dae::Renderer::ShadingMode::Diffuse:
	{
		ColorRGB diffuse{ (m_pDiffuseTexture->Sample(pixel.uv) * kd) / PI * m_LightIntensity };
		finalColor = diffuse * observedArea;
	}
		break;
	case dae::Renderer::ShadingMode::Specular:
	{
		finalColor = CalculateSpecular(pixel, sampledNormal) * observedArea;
	}
		break;
	case dae::Renderer::ShadingMode::Combined:
		ColorRGB diffuse{ (m_pDiffuseTexture->Sample(pixel.uv) * kd) / PI * m_LightIntensity };

		finalColor = (diffuse *  observedArea) + CalculateSpecular(pixel, sampledNormal);
		break;
	}

	return finalColor;
}

ColorRGB dae::Renderer::CalculateSpecular(const Pixel_Out& pixel, const Vector3& sampledNormal)
{
	const Vector3 reflect{ Vector3::Reflect(m_LightDirection, sampledNormal) };

	const float cosAngle{ std::max(0.f, Vector3::Dot(reflect, -pixel.viewDirection)) };

	const float exp{ m_pGlossTexture->Sample(pixel.uv).r * m_Shininess };

	const float phongSpecular{ powf(cosAngle, exp) };

	return m_pSpecularTexture->Sample(pixel.uv) * phongSpecular;
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}

void dae::Renderer::PrintShadingMode()
{
	std::cout << "Shading mode: ";

	switch (m_ShadingMode)
	{
	case dae::Renderer::ShadingMode::ObservedArea:
		std::cout << "Observed area \n";
		break;
	case dae::Renderer::ShadingMode::Diffuse:
		std::cout << "Diffuse \n";
		break;
	case dae::Renderer::ShadingMode::Specular:
		std::cout << "Specular \n";
		break;
	case dae::Renderer::ShadingMode::Combined:
		std::cout << "Combined \n";
		break;
	default:
		break;
	}
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
