#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>
#include <iostream>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{
	}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		//TODO
		//Load SDL_Surface using IMG_LOAD
		//Create & Return a new Texture Object (using SDL_Surface)

		return new Texture{ IMG_Load(path.c_str()) };
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		//TODO
		//Sample the correct texel for the given uv

		//calculate the x and y coordinates on the uv map
		const int x{ static_cast<int>(uv.x * m_pSurface->w) };
		const int y{ static_cast<int>(uv.y * m_pSurface->h) };

		//getht the index of the pixel in the list
		const uint32_t pixelIdx{m_pSurfacePixels[x + y * m_pSurface->w]};

		//initialize the rgb values in the [0, 255] range
		Uint8 r{};
		Uint8 g{};
		Uint8 b{};

		//Get correct values from the texture
		SDL_GetRGB(pixelIdx, m_pSurface->format, &r, &g, &b);

		//return color divided by 255 to get colors in [0, 1] range
		return ColorRGB{r/255.0f, g/255.0f, b/255.0f};
	}
}