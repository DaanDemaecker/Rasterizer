#pragma once

#include <cstdint>
#include <vector>
#include <iostream>

#include "Camera.h"
#include "DataTypes.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	class Timer;
	class Scene;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();

		bool SaveBufferToImage() const;

		void ToggleColor() { m_RenderFinalColor = !m_RenderFinalColor; };
		void ToggleBoundingBox() { m_RenderBoundingBox = !m_RenderBoundingBox; };
		void ToggleRotation() { m_RotationEnabled = !m_RotationEnabled; };
		void ToggleNormal() { m_UseNormalMap = !m_UseNormalMap; };
		void CycleShading() { m_ShadingMode = static_cast<ShadingMode>((int(m_ShadingMode) + 1) % 4); PrintShadingMode(); };

		void PrintShadingMode();

		enum class ShadingMode
		{
			ObservedArea,
			Diffuse,
			Specular,
			Combined
		};

	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		Mesh m_Mesh{};

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};

		int m_Width{};
		int m_Height{};

		float m_AspectRatio{};

		bool m_RenderBoundingBox{ false };
		bool m_RenderFinalColor{ true };
		bool m_RotationEnabled{ true };
		bool m_UseNormalMap{ false };
		ShadingMode m_ShadingMode{ ShadingMode::ObservedArea };

		Texture* m_pDiffuseTexture{ nullptr };
		Texture* m_pGlossTexture{ nullptr };
		Texture* m_pSpecularTexture{ nullptr };
		Texture* m_pNormalMap{ nullptr };

		const Vector3 m_LightDirection = Vector3{ .577f, -.577f, .577f }.Normalized();
		float m_LightIntensity{ 7.f };
		float m_Shininess{ 25.f };
		ColorRGB m_Ambient{.025f, .025f, .025f};

		void InitializeMesh();

		//function that returns the bounding box for a triangle
		BoundingBox GetBoundingBox(Vector2 v0, Vector2 v1, Vector2 v2);

		//function that renders a single mesh
		void RenderMesh(Mesh& mesh);

		//function that renders a single triangle
		void RenderTriangle(const Mesh& mesh, std::vector<Vector2> vertices_ScreenSpace, int startIdx, bool flipTriangle = false);

		//Function that transforms the vertices from the mesh from World space to Screen space
		void VertexTransformationFunction(Mesh& mesh) const; //W1 Version

		//Function that shades a single pixel
		ColorRGB PixelShading(Pixel_Out& pixel);

		ColorRGB CalculateSpecular(const Pixel_Out& pixel, const Vector3& sampeledNormal);
	};
}
