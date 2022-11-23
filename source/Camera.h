#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle, float _aspectRatio):
			origin{_origin},
			fovAngle{_fovAngle},
			aspectRatio{_aspectRatio}
		{
		}


		Vector3 origin{};
		float fovAngle{90.f};
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };
		float aspectRatio{ 4/3.f };

		float nearPlane{ 0.01f };
		float farPlane{ 100.f };

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float cameraSpeed{ 10.f };
		float angularSpeed{ 2.5f };

		float totalPitch{};
		float totalYaw{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};

		Matrix projectionMatrix{};

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f}, float _aspectRatio = 4/3.f)
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;

			aspectRatio = _aspectRatio;
		}

		void CalculateViewMatrix()
		{
			//TODO W1
			//ONB => invViewMatrix
			//Inverse(ONB) => ViewMatrix
			right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
			up = Vector3::Cross(forward, right).Normalized();

			
			invViewMatrix = Matrix
			{
				right,
				up,
				forward,
				origin
			};

			viewMatrix = invViewMatrix.Inverse();

			//ViewMatrix => Matrix::CreateLookAtLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
		}

		void CalculateProjectionMatrix()
		{
			//TODO W2

			//ProjectionMatrix => Matrix::CreatePerspectiveFovLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh

			projectionMatrix = Matrix::CreatePerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);


			origin += (pKeyboardState[SDL_SCANCODE_W] || pKeyboardState[SDL_SCANCODE_Z] || pKeyboardState[SDL_SCANCODE_UP]) * (forward * cameraSpeed * deltaTime);
			origin -= (pKeyboardState[SDL_SCANCODE_S] || pKeyboardState[SDL_SCANCODE_DOWN]) * (forward * cameraSpeed * deltaTime);
			origin -= (pKeyboardState[SDL_SCANCODE_Q] || pKeyboardState[SDL_SCANCODE_A] || pKeyboardState[SDL_SCANCODE_LEFT]) * (right * cameraSpeed * deltaTime);
			origin += (pKeyboardState[SDL_SCANCODE_D] || pKeyboardState[SDL_SCANCODE_RIGHT]) * (right * cameraSpeed * deltaTime);

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			switch (mouseState)
			{
			case SDL_BUTTON_LEFT: //right click
				origin += forward * float(mouseY) * deltaTime;
				totalYaw += float(mouseX) * deltaTime * angularSpeed;
				break;
			case SDL_BUTTON_X1: //left click
				totalPitch -= float(mouseY) * deltaTime * angularSpeed;
				totalYaw += float(mouseX) * deltaTime * angularSpeed;
				break;
			case SDL_BUTTON_X2: //both click
				origin.y += float(mouseY) * deltaTime;
			default:
				break;
			}


			Matrix rotation = Matrix::CreateRotationX(totalPitch) * Matrix::CreateRotationY(totalYaw);
			forward = rotation.TransformVector(Vector3::UnitZ);

			//Update Matrices
			CalculateViewMatrix();
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
		}


		bool isOutsideFrustum(const Vector4& vertex)const
		{
			return (vertex.x < -1.0f || vertex.x > 1.0f || vertex.y < -1.0f || vertex.y > 1.0f);
		}

	};
}
