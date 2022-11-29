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
		bool m_RenderFinalColor{true};

		Texture* m_pTexture{nullptr};

		void InitializeMesh();

		//function that returns the bounding box for a triangle
		BoundingBox GetBoundingBox(Vector2 v0, Vector2 v1, Vector2 v2);

		//function that renders a single mesh
		void RenderMesh(Mesh& mesh);

		//function that renders a single triangle
		void RenderTriangle(const Mesh& mesh, std::vector<Vector2> vertices_ScreenSpace, int startIdx, bool flipTriangle = false);

		//Function that transforms the vertices from the mesh from World space to Screen space
		void VertexTransformationFunction(Mesh& mesh) const; //W1 Version
	};
}
